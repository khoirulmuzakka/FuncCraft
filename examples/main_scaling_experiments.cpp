/**
 * @file main_scaling_experiments.cpp
 * @brief C++ port of run/run_experiments.py + run/scaling_lab.py.
 *
 * Sweeps every (algorithm, problem, dimension) cell over Sphere and
 * FuncCraft's 34 undecorated base functions (no coordinate rotation, no
 * value transform, no composition -- and no MultiSphere: that problem family
 * is intentionally not ported here). Ten algorithms: nine real minion
 * algorithms plus RR_L_BFGS_B, a local meta-algorithm (random-restart
 * L-BFGS-B, ported from scaling_lab.py's _rr_lbfgsb_run -- see
 * run_attempt_rr_lbfgsb()) that minion itself has no notion of. Every
 * problem is evaluated *shifted* so its minimum value is exactly 0 -- Sphere
 * already has f_opt = 0, and each FuncCraft base function subtracts its own
 * f_opt before the value reaches either the optimizer or the hitting-time
 * check. Runs land in
 * ``examples/results/raw_sphere.csv`` and ``examples/results/raw_funccraft.csv``
 * with the same columns run_experiments.py writes: algo, problem, dim, run,
 * hit, success, nfev_spent, final_budget, f_gap_best, seconds.
 *
 * Parallelism is over (algorithm, problem, dimension) cells, one worker
 * thread per cell at a time, matching the process-pool granularity of the
 * Python driver; each cell's ``n_runs`` runs execute sequentially in that
 * worker so the early-failure short-circuit (see run_cell()) still applies.
 * A cell already present in the output CSV is skipped, so an interrupted
 * sweep resumes where it stopped.
 *
 * Usage:
 *   run_scaling_experiments [which] [--workers N] [--runs N] [--cap N]
 *                           [--outdir DIR] [--q Q]
 *     which      positional, "all" (default), "sphere", or "funccraft"
 *     --workers  worker threads (default: hardware concurrency)
 *     --runs     independent runs per cell (default: 100)
 *     --cap      evaluation budget cap per run (default: 10,000,000)
 *     --outdir   output directory (default: examples/results)
 *     --q        prescribed success probability for the early-failure
 *                short-circuit (default: 0.5)
 *
 *   e.g. run_scaling_experiments sphere --workers 1 --runs 5 --cap 20000
 */

#include "funccraft.h"
#include <minion.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {

// =============================================================================
// Defaults, mirroring run/scaling_lab.py
// =============================================================================

constexpr double EPSILON = 1e-6;             // success iff f - f_opt <= EPSILON
constexpr double Q_DEFAULT = 0.5;            // prescribed success probability
constexpr int N_RUNS_DEFAULT = 100;          // independent runs per cell
constexpr long long MAXEVALS_CAP_DEFAULT = 10'000'000LL;
constexpr int BUDGET0_PER_DIM = 100;         // doubling protocol starts at 100*D
constexpr std::uint64_t BASE_SEED = 20260807ULL;

const std::vector<int> SPHERE_DIMS = {2, 5, 10, 20, 40};
const std::vector<int> FUNCCRAFT_DIMS = {2, 5, 10, 20, 40};
constexpr int FUNCCRAFT_FUNCTION_COUNT = 34;

// The algorithm set requested for this sweep. Names are whatever
// canonicalize_algo() (below) accepts case/punctuation-insensitively; the
// canonical form is what ends up in the CSV. Nine are real minion
// algorithms; "RR_lbfgsb" is a local meta-algorithm minion doesn't know
// about (see canonicalize_algo() and run_attempt_rr_lbfgsb()).
const std::vector<std::string> ALGOS = {
    "Lbfgsb", "neldermead", "de", "cmaes", "bipopacmaes",
    "rcmaes", "arrde", "nlshadersp", "rdex", "RR_lbfgsb",
};

// Budget-paced algorithms: their internal schedule (population reduction,
// memory sizing, ...) is keyed to maxevals, so handing them a large budget
// changes their trajectory rather than just capping it. These need the
// doubling protocol below. scaling_lab.py flags ARRDE and NLSHADE_RSP for
// the same reason ("the DE family"); RDEX is the same family (LSHADE-style
// success-history adaptation) and is paced identically here. RR_L_BFGS_B is
// not in this set -- like scaling_lab.py, it treats its own outer budget
// (the eval cap) as a ceiling and paces itself internally by restarting.
const std::unordered_set<std::string> BUDGET_PACED = {"ARRDE", "NLSHADE_RSP", "RDEX"};

// RR_L_BFGS_B has no minion implementation to canonicalize against; it is
// handled entirely in this file.
constexpr const char* RR_LBFGSB_CANONICAL = "RR_L_BFGS_B";

std::string normalize_algo_name(const std::string& name) {
    std::string norm;
    norm.reserve(name.size());
    for (unsigned char c : name) {
        if (c == '-' || c == '_' || c == ' ') continue;
        norm += static_cast<char>(std::toupper(c));
    }
    return norm;
}

/// Resolves an algorithm name to its canonical form. Real minion algorithms
/// defer to minion::DefaultSettings::canonicalAlgoName(); RR_L_BFGS_B (in
/// any of its usual spellings) is intercepted first since minion has never
/// heard of it.
std::string canonicalize_algo(const std::string& name) {
    const std::string norm = normalize_algo_name(name);
    if (norm == "RRLBFGSB" || norm == "RRLBFGS" || norm == "RANDOMRESTARTLBFGSB") {
        return RR_LBFGSB_CANONICAL;
    }
    return minion::DefaultSettings::canonicalAlgoName(name);
}

// =============================================================================
// Problems: Sphere and FuncCraft's 34 undecorated base functions
// =============================================================================

struct ProblemSpec {
    bool is_sphere = true;
    int function_number = 0; // 1..34, meaningful only when !is_sphere
    int dim = 0;
};

std::string problem_label(const ProblemSpec& spec) {
    if (spec.is_sphere) return "Sphere";
    return "FC-F" + std::to_string(spec.function_number);
}

class Problem {
public:
    virtual ~Problem() = default;
    virtual std::vector<double> evaluate_raw(const std::vector<std::vector<double>>& X) const = 0;
    virtual double f_opt() const = 0;
    virtual int dim() const = 0;
    virtual const std::vector<std::pair<double, double>>& bounds() const = 0;
};

/// f(x) = sum_i x_i^2 on [-R, R]^D; single basin, f_opt = 0 at the origin.
class SphereProblem : public Problem {
public:
    explicit SphereProblem(int dim, double R = 5.0) : dim_(dim) {
        bounds_.assign(static_cast<std::size_t>(dim), std::make_pair(-R, R));
    }

    std::vector<double> evaluate_raw(const std::vector<std::vector<double>>& X) const override {
        std::vector<double> out(X.size());
        for (std::size_t i = 0; i < X.size(); ++i) {
            double s = 0.0;
            for (double v : X[i]) s += v * v;
            out[i] = s;
        }
        return out;
    }

    double f_opt() const override { return 0.0; }
    int dim() const override { return dim_; }
    const std::vector<std::pair<double, double>>& bounds() const override { return bounds_; }

private:
    int dim_;
    std::vector<std::pair<double, double>> bounds_;
};

/// One of FuncCraft's 34 primitive base functions, undecorated: no
/// coordinate transform, no value transform, no composition. Evaluated
/// through Problem::evaluate_raw() at face value; the f_opt() shift that
/// zeroes its minimum happens once, centrally, in Tracker.
class FuncCraftBasicProblem : public Problem {
public:
    FuncCraftBasicProblem(int function_number, int dim)
        : f_(static_cast<FuncCraft::BasicFunctionId>(function_number), dim), dim_(dim) {
        const FuncCraft::Domain domain = f_.default_domain();
        bounds_.reserve(static_cast<std::size_t>(dim));
        for (int d = 0; d < dim; ++d) {
            const auto idx = static_cast<std::size_t>(d);
            bounds_.emplace_back(domain.lower[idx], domain.upper[idx]);
        }
    }

    std::vector<double> evaluate_raw(const std::vector<std::vector<double>>& X) const override {
        return f_(X);
    }

    double f_opt() const override { return f_.f_opt; }
    int dim() const override { return dim_; }
    const std::vector<std::pair<double, double>>& bounds() const override { return bounds_; }

private:
    FuncCraft::BasicF f_;
    int dim_;
    std::vector<std::pair<double, double>> bounds_;
};

std::unique_ptr<Problem> make_problem(const ProblemSpec& spec) {
    if (spec.is_sphere) return std::make_unique<SphereProblem>(spec.dim);
    return std::make_unique<FuncCraftBasicProblem>(spec.function_number, spec.dim);
}

// =============================================================================
// Deterministic per-cell seeding (process-independent, unlike std::hash)
// =============================================================================

std::uint64_t mix_seed(std::uint64_t seed, std::uint64_t value) {
    seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6U) + (seed >> 2U);
    return seed;
}

/// FNV-1a over a textual spec representation -- stable across runs and
/// platforms, unlike std::hash<std::string> (which is not required to be).
std::uint64_t stable_hash(const ProblemSpec& spec) {
    std::uint64_t h = 1469598103934665603ULL; // FNV offset basis
    const std::string repr = spec.is_sphere
        ? ("sphere," + std::to_string(spec.dim))
        : ("funccraft," + std::to_string(spec.function_number) + "," + std::to_string(spec.dim));
    for (unsigned char c : repr) {
        h ^= c;
        h *= 1099511628211ULL; // FNV prime
    }
    return h;
}

// =============================================================================
// Evaluation accounting
// =============================================================================

/// Counts objective evaluations and records the hitting time, mirroring
/// scaling_lab.py's Tracker. Every algorithm reaches the objective through
/// this wrapper. Values returned to the optimizer (and used for the hit
/// check) are already shifted by f_opt, so the target is "value <= EPSILON".
struct Tracker {
    const Problem* problem = nullptr;
    double epsilon = EPSILON;
    long long n = 0;
    long long hit = -1; // -1 means "not yet hit" (Python's None)
    double f_best = std::numeric_limits<double>::infinity();

    std::vector<double> operator()(const std::vector<std::vector<double>>& X) {
        const std::vector<double> raw = problem->evaluate_raw(X);
        std::vector<double> shifted(raw.size());
        const double fopt = problem->f_opt();
        for (std::size_t i = 0; i < raw.size(); ++i) {
            shifted[i] = raw[i] - fopt;
        }
        if (hit < 0) {
            for (std::size_t i = 0; i < shifted.size(); ++i) {
                if (shifted[i] <= epsilon) {
                    hit = n + static_cast<long long>(i) + 1;
                    break;
                }
            }
        }
        for (double v : shifted) {
            f_best = std::min(f_best, v);
        }
        n += static_cast<long long>(shifted.size());
        return shifted;
    }

    bool success() const { return hit >= 0; }
};

// =============================================================================
// Optimizer dispatch
// =============================================================================

/// Random-restart L-BFGS-B: repeatedly fires a fresh single-shot L-BFGS-B
/// from an independent uniform starting point, evaluations accumulating in
/// the shared tracker, until the target is hit or the outer budget is
/// exhausted. Ported from scaling_lab.py's _rr_lbfgsb_run() -- the direct
/// empirical counterpart of the restart toy model
/// N_req = S_loc(D) * (R/r)^D. Each restart gets whatever budget remains,
/// same as the Python version; a restart that makes no progress at all
/// (n unchanged) stops the loop rather than spinning forever.
Tracker run_attempt_rr_lbfgsb(const Problem& problem, std::size_t budget, std::uint64_t seed_material) {
    Tracker tracker;
    tracker.problem = &problem;
    tracker.epsilon = EPSILON;

    std::mt19937_64 rng(seed_material);
    std::uniform_int_distribution<int> seed_dist(1, std::numeric_limits<int>::max() - 1);
    const auto settings = minion::DefaultSettings().getDefaultSettings("L_BFGS_B");

    while (static_cast<std::size_t>(tracker.n) < budget && !tracker.success()) {
        std::vector<double> x0(static_cast<std::size_t>(problem.dim()));
        for (std::size_t d = 0; d < x0.size(); ++d) {
            const auto& b = problem.bounds()[d];
            std::uniform_real_distribution<double> dist(b.first, b.second);
            x0[d] = dist(rng);
        }
        const std::size_t remaining = budget - static_cast<std::size_t>(tracker.n);
        const long long before = tracker.n;
        const int restart_seed = seed_dist(rng);

        try {
            auto objective = [&tracker](const std::vector<std::vector<double>>& X, void*) {
                return tracker(X);
            };
            auto callback = [&tracker](minion::MinionResult*) {
                return tracker.success();
            };
            minion::Minimizer optimizer(
                objective, problem.bounds(), x0, nullptr, callback,
                "L_BFGS_B", std::max<std::size_t>(remaining, 1), restart_seed, settings);
            optimizer.optimize();
        } catch (...) {
            // A failed restart still counts whatever evaluations it spent;
            // fall through to the no-progress check below.
        }

        if (tracker.n == before) break; // no progress -- avoid an infinite spin
    }
    return tracker;
}

Tracker run_attempt(const std::string& canonical_algo, const Problem& problem,
                    const std::vector<double>& x0, std::size_t budget, int seed) {
    if (canonical_algo == RR_LBFGSB_CANONICAL) {
        return run_attempt_rr_lbfgsb(problem, budget, static_cast<std::uint64_t>(seed));
    }
    Tracker tracker;
    tracker.problem = &problem;
    tracker.epsilon = EPSILON;
    try {
        auto objective = [&tracker](const std::vector<std::vector<double>>& X, void*) {
            return tracker(X);
        };
        auto callback = [&tracker](minion::MinionResult*) {
            return tracker.success();
        };
        const auto settings = minion::DefaultSettings().getDefaultSettings(canonical_algo);
        minion::Minimizer optimizer(
            objective, problem.bounds(), x0, nullptr, callback,
            canonical_algo, budget, seed, settings);
        optimizer.optimize();
    } catch (...) {
        // A solver failure is a failed run, not a crashed experiment; the
        // tracker still holds whatever evaluations were spent.
    }
    return tracker;
}

// =============================================================================
// Budget protocol + single run
// =============================================================================

struct RunResult {
    std::string algo;
    std::string problem;
    int dim = 0;
    int run = 0;
    long long hit = -1;
    bool success = false;
    long long nfev_spent = 0;
    long long final_budget = 0;
    double f_gap_best = std::numeric_limits<double>::infinity();
    double seconds = 0.0;
};

RunResult run_single(const std::string& canonical_algo, const ProblemSpec& spec, int run_index,
                     long long cap) {
    const std::unique_ptr<Problem> problem = make_problem(spec);

    std::uint64_t seed_material = BASE_SEED;
    seed_material = mix_seed(seed_material, static_cast<std::uint64_t>(run_index));
    seed_material = mix_seed(seed_material, static_cast<std::uint64_t>(problem->dim()));
    seed_material = mix_seed(seed_material, stable_hash(spec));
    std::mt19937_64 rng(seed_material);

    std::vector<double> x0(static_cast<std::size_t>(problem->dim()));
    for (std::size_t d = 0; d < x0.size(); ++d) {
        const auto& b = problem->bounds()[d];
        std::uniform_real_distribution<double> dist(b.first, b.second);
        x0[d] = dist(rng);
    }
    std::uniform_int_distribution<int> seed_dist(1, std::numeric_limits<int>::max() - 1);
    const int seed = seed_dist(rng);

    const auto t0 = std::chrono::steady_clock::now();
    long long spent = 0;
    long long final_budget = 0;
    Tracker tracker;
    tracker.problem = problem.get();

    if (BUDGET_PACED.count(canonical_algo) != 0) {
        long long budget = std::max<long long>(
            static_cast<long long>(BUDGET0_PER_DIM) * problem->dim(), 100);
        while (true) {
            tracker = run_attempt(canonical_algo, *problem, x0,
                                  static_cast<std::size_t>(budget), seed);
            spent += tracker.n;
            if (tracker.success() || budget >= cap) break;
            budget = std::min<long long>(budget * 2, cap);
        }
        final_budget = budget;
    } else {
        tracker = run_attempt(canonical_algo, *problem, x0, static_cast<std::size_t>(cap), seed);
        spent = tracker.n;
        final_budget = cap;
    }

    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t0;

    RunResult r;
    r.algo = canonical_algo;
    r.problem = problem_label(spec);
    r.dim = problem->dim();
    r.run = run_index;
    r.hit = tracker.hit;
    r.success = tracker.success();
    r.nfev_spent = spent;
    r.final_budget = final_budget;
    r.f_gap_best = tracker.f_best;
    r.seconds = elapsed.count();
    return r;
}

// =============================================================================
// Cell runner: all runs for one (algorithm, problem, dimension) cell
// =============================================================================

int required_rank(double q, int n_runs) {
    return static_cast<int>(std::ceil(q * static_cast<double>(n_runs)));
}

/// Terminates early once the target success probability has become
/// unreachable, exactly as scaling_lab.py's run_cell(): with n_runs runs and
/// rank k, n_runs-k+1 observed failures makes fewer than k successes
/// certain, so the result is censored no matter what the remaining runs do.
std::vector<RunResult> run_cell(const std::string& canonical_algo, const ProblemSpec& spec,
                                int n_runs, double q, long long cap) {
    const int k = required_rank(q, n_runs);
    const int max_failures = n_runs - k + 1;
    std::vector<RunResult> rows;
    rows.reserve(static_cast<std::size_t>(n_runs));
    int failures = 0;
    for (int r = 0; r < n_runs; ++r) {
        RunResult row = run_single(canonical_algo, spec, r, cap);
        const bool ok = row.success;
        rows.push_back(std::move(row));
        failures += ok ? 0 : 1;
        if (failures >= max_failures) break;
    }
    return rows;
}

// =============================================================================
// CSV output: same columns as run_experiments.py's raw_<which>.csv
// =============================================================================

class CsvWriter {
public:
    explicit CsvWriter(std::filesystem::path path) : path_(std::move(path)) {
        std::error_code ec;
        header_written_ = std::filesystem::exists(path_, ec) &&
                          std::filesystem::file_size(path_, ec) > 0;
    }

    void append(const std::vector<RunResult>& rows) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream out(path_, std::ios::app);
        if (!header_written_) {
            out << "algo,problem,dim,run,hit,success,nfev_spent,final_budget,f_gap_best,seconds\n";
            header_written_ = true;
        }
        for (const auto& row : rows) {
            out << row.algo << ',' << row.problem << ',' << row.dim << ',' << row.run << ',';
            if (row.hit >= 0) {
                out << row.hit;
            }
            out << ',' << (row.success ? "True" : "False") << ','
                << row.nfev_spent << ',' << row.final_budget << ',';
            out << std::scientific << std::setprecision(10) << row.f_gap_best << ',';
            out << std::fixed << std::setprecision(6) << row.seconds << '\n';
        }
    }

    /// Keys of ("algo,problem,dim") already present, for resuming a sweep.
    std::unordered_set<std::string> already_done() const {
        std::unordered_set<std::string> done;
        std::ifstream in(path_);
        if (!in) return done;
        std::string line;
        std::getline(in, line); // header
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            const std::size_t p1 = line.find(',');
            const std::size_t p2 = (p1 == std::string::npos) ? std::string::npos : line.find(',', p1 + 1);
            const std::size_t p3 = (p2 == std::string::npos) ? std::string::npos : line.find(',', p2 + 1);
            if (p3 == std::string::npos) continue;
            done.insert(line.substr(0, p3));
        }
        return done;
    }

private:
    std::filesystem::path path_;
    bool header_written_ = false;
    std::mutex mutex_;
};

std::string cell_key(const std::string& canonical_algo, const ProblemSpec& spec) {
    return canonical_algo + "," + problem_label(spec) + "," + std::to_string(spec.dim);
}

// =============================================================================
// Cells and driver
// =============================================================================

struct Cell {
    std::string canonical_algo;
    ProblemSpec spec;
    bool is_sphere_family = true;
};

std::vector<std::string> canonical_algos() {
    std::vector<std::string> out;
    out.reserve(ALGOS.size());
    for (const auto& a : ALGOS) {
        out.push_back(canonicalize_algo(a));
    }
    return out;
}

/// (algo, spec) pairs, cheapest problems first so results arrive early.
std::vector<Cell> build_cells(bool include_sphere, bool include_funccraft) {
    const std::vector<std::string> algos = canonical_algos();
    std::vector<Cell> cells;
    if (include_sphere) {
        for (int d : SPHERE_DIMS) {
            for (const auto& algo : algos) {
                ProblemSpec spec;
                spec.is_sphere = true;
                spec.dim = d;
                cells.push_back({algo, spec, true});
            }
        }
    }
    if (include_funccraft) {
        for (int fn = 1; fn <= FUNCCRAFT_FUNCTION_COUNT; ++fn) {
            for (int d : FUNCCRAFT_DIMS) {
                for (const auto& algo : algos) {
                    ProblemSpec spec;
                    spec.is_sphere = false;
                    spec.function_number = fn;
                    spec.dim = d;
                    cells.push_back({algo, spec, false});
                }
            }
        }
    }
    return cells;
}

struct Config {
    std::string which = "all"; // "all", "sphere", "funccraft"
    int nthreads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    int n_runs = N_RUNS_DEFAULT;
    long long cap = MAXEVALS_CAP_DEFAULT;
    std::string out_dir = "examples/results";
    double q = Q_DEFAULT;
};

long long parse_integer_arg(const char* text, const std::string& name) {
    if (text == nullptr || *text == '\0') {
        throw std::invalid_argument(name + " must be an integer");
    }
    char* end = nullptr;
    errno = 0;
    const long long value = std::strtoll(text, &end, 10);
    if (errno != 0 || end == text || end == nullptr || *end != '\0') {
        throw std::invalid_argument(name + " must be an integer");
    }
    return value;
}

double parse_double_arg(const char* text, const std::string& name) {
    if (text == nullptr || *text == '\0') {
        throw std::invalid_argument(name + " must be a number");
    }
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(text, &end);
    if (errno != 0 || end == text || end == nullptr || *end != '\0') {
        throw std::invalid_argument(name + " must be a number");
    }
    return value;
}

/// Flag-style CLI, mirroring run_experiments.py's argparse conventions
/// (--workers, --runs, --cap, --q); "which" is the one positional argument.
Config parse_cli(int argc, char* argv[]) {
    Config config;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next_value = [&](const std::string& flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(flag + " requires a value");
            return argv[++i];
        };
        if (arg == "--which") {
            config.which = next_value(arg);
        } else if (arg == "--workers" || arg == "--nthreads") {
            config.nthreads = static_cast<int>(parse_integer_arg(next_value(arg).c_str(), arg));
        } else if (arg == "--runs") {
            config.n_runs = static_cast<int>(parse_integer_arg(next_value(arg).c_str(), arg));
        } else if (arg == "--cap") {
            config.cap = parse_integer_arg(next_value(arg).c_str(), arg);
        } else if (arg == "--outdir" || arg == "--out-dir") {
            config.out_dir = next_value(arg);
        } else if (arg == "--q") {
            config.q = parse_double_arg(next_value(arg).c_str(), arg);
        } else if (!arg.empty() && arg[0] == '-' && arg != "-") {
            throw std::invalid_argument("unknown flag: " + arg);
        } else {
            positional.push_back(arg);
        }
    }
    if (positional.size() > 1) {
        throw std::invalid_argument("expected at most one positional argument: which");
    }
    if (!positional.empty()) {
        config.which = positional[0];
    }

    if (config.which != "all" && config.which != "sphere" && config.which != "funccraft") {
        throw std::invalid_argument("which must be one of: all, sphere, funccraft");
    }
    if (config.nthreads < 1) throw std::invalid_argument("--workers must be at least 1");
    if (config.n_runs < 1) throw std::invalid_argument("--runs must be at least 1");
    if (config.cap < 1) throw std::invalid_argument("--cap must be at least 1");
    if (config.q <= 0.0 || config.q > 1.0) throw std::invalid_argument("--q must be in (0, 1]");
    return config;
}

void run_jobs(std::vector<Cell>& cells, const Config& config,
             CsvWriter& sphere_writer, CsvWriter& funccraft_writer,
             const std::unordered_set<std::string>& sphere_done,
             const std::unordered_set<std::string>& funccraft_done) {
    std::vector<std::size_t> todo;
    todo.reserve(cells.size());
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const Cell& cell = cells[i];
        const auto& done = cell.is_sphere_family ? sphere_done : funccraft_done;
        if (done.count(cell_key(cell.canonical_algo, cell.spec)) == 0) {
            todo.push_back(i);
        }
    }

    std::cout << "Cells to run: " << todo.size() << " (" << (cells.size() - todo.size())
              << " already cached), worker threads: "
              << std::min<std::size_t>(static_cast<std::size_t>(config.nthreads), todo.size())
              << "\n" << std::flush;
    if (todo.empty()) return;

    std::atomic<std::size_t> next_index{0};
    std::atomic<std::size_t> finished{0};
    std::mutex output_mutex;
    const auto t_start = std::chrono::steady_clock::now();

    const int worker_count = std::min<int>(config.nthreads, static_cast<int>(todo.size()));
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(worker_count));

    for (int w = 0; w < worker_count; ++w) {
        workers.emplace_back([&]() {
            while (true) {
                const std::size_t job = next_index.fetch_add(1);
                if (job >= todo.size()) break;
                const Cell& cell = cells[todo[job]];

                const auto t0 = std::chrono::steady_clock::now();
                std::vector<RunResult> rows = run_cell(cell.canonical_algo, cell.spec,
                                                       config.n_runs, config.q, config.cap);
                const std::chrono::duration<double> dt = std::chrono::steady_clock::now() - t0;

                if (cell.is_sphere_family) {
                    sphere_writer.append(rows);
                } else {
                    funccraft_writer.append(rows);
                }

                const int n_ok = static_cast<int>(std::count_if(
                    rows.begin(), rows.end(), [](const RunResult& r) { return r.success; }));
                const std::size_t done_count = finished.fetch_add(1) + 1;
                const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t_start;

                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "[" << std::setw(5) << done_count << "/" << todo.size() << "] "
                          << std::left << std::setw(6) << (cell.is_sphere_family ? "sphere" : "funcc")
                          << std::setw(10) << problem_label(cell.spec)
                          << "D=" << std::setw(4) << cell.spec.dim
                          << std::setw(14) << cell.canonical_algo
                          << std::right << std::setw(3) << n_ok << "/" << rows.size() << " ok  "
                          << "cell " << std::fixed << std::setprecision(1) << std::setw(7) << dt.count()
                          << "s  elapsed " << std::setw(8) << elapsed.count() << "s\n" << std::flush;
            }
        });
    }
    for (auto& t : workers) t.join();
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const Config config = parse_cli(argc, argv);
        std::filesystem::create_directories(config.out_dir);

        const std::filesystem::path sphere_path =
            std::filesystem::path(config.out_dir) / "raw_sphere.csv";
        const std::filesystem::path funccraft_path =
            std::filesystem::path(config.out_dir) / "raw_funccraft.csv";
        CsvWriter sphere_writer(sphere_path);
        CsvWriter funccraft_writer(funccraft_path);
        const std::unordered_set<std::string> sphere_done = sphere_writer.already_done();
        const std::unordered_set<std::string> funccraft_done = funccraft_writer.already_done();

        const bool include_sphere = config.which == "all" || config.which == "sphere";
        const bool include_funccraft = config.which == "all" || config.which == "funccraft";
        std::vector<Cell> cells = build_cells(include_sphere, include_funccraft);

        std::cout << "Usage: run_scaling_experiments [which] [--workers N] [--runs N] "
                     "[--cap N] [--outdir DIR] [--q Q]\n";
        std::cout << "which: " << config.which
                  << ", nthreads: " << config.nthreads
                  << ", runs: " << config.n_runs
                  << ", cap: " << config.cap
                  << ", q: " << config.q
                  << ", out_dir: " << config.out_dir << "\n";
        std::cout << "algorithms:";
        for (const auto& a : canonical_algos()) std::cout << ' ' << a;
        std::cout << "\n";
        auto print_dims = [](const std::vector<int>& dims) {
            for (std::size_t i = 0; i < dims.size(); ++i) {
                if (i) std::cout << ',';
                std::cout << dims[i];
            }
        };
        std::cout << "problems: ";
        if (include_sphere) {
            std::cout << "Sphere(D=";
            print_dims(SPHERE_DIMS);
            std::cout << ") ";
        }
        if (include_funccraft) {
            std::cout << "FuncCraft-F1.." << FUNCCRAFT_FUNCTION_COUNT << "(D=";
            print_dims(FUNCCRAFT_DIMS);
            std::cout << ")";
        }
        std::cout << "\ntotal cells: " << cells.size() << "\n\n" << std::flush;

        const auto t_start = std::chrono::steady_clock::now();
        run_jobs(cells, config, sphere_writer, funccraft_writer, sphere_done, funccraft_done);
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t_start;

        std::cout << "\nDone in " << std::fixed << std::setprecision(1) << elapsed.count()
                  << "s\n";
        if (include_sphere) std::cout << "  -> " << sphere_path.string() << "\n";
        if (include_funccraft) std::cout << "  -> " << funccraft_path.string() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_scaling_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
