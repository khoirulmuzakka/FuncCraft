/**
 * @file main_scaling_experiments.cpp
 * @brief C++ port of run/run_experiments.py + run/scaling_lab.py.
 *
 * Sweeps every (algorithm, function, dimension) cell over a curated subset
 * of FuncCraft's 34 undecorated base functions -- no coordinate rotation, no
 * value transform, no composition, and no MultiSphere: that problem family
 * is intentionally not ported here. There is no separate Sphere problem
 * either -- FuncCraft base-function index 1 already is Sphere, so it is
 * just one more entry in --functions rather than a second problem family.
 * Ten algorithms: nine real minion algorithms plus RR_L_BFGS_B, a local
 * meta-algorithm (random-restart L-BFGS-B, ported from scaling_lab.py's
 * _rr_lbfgsb_run -- see run_attempt_rr_lbfgsb()) that minion itself has no
 * notion of. Every function is evaluated *shifted* so its minimum value is
 * exactly 0 -- each FuncCraft base function subtracts its own f_opt before
 * the value reaches either the optimizer or the hitting-time check. Runs
 * land in ``examples/results/raw_funccraft.csv`` with the same columns
 * run_experiments.py writes: algo, problem, dim, run, hit, success,
 * nfev_spent, final_budget, f_gap_best, seconds.
 *
 * Parallelism is over (algorithm, function, dimension) cells, one worker
 * thread per cell at a time, matching the process-pool granularity of the
 * Python driver; each cell's ``n_runs`` runs execute sequentially in that
 * worker so the early-failure short-circuit (see run_cell(), and note that
 * --dm turns it off) still applies. A cell already present in the output CSV
 * is skipped, so an interrupted sweep resumes where it stopped.
 *
 * Passing --dm switches the sweep to the dimensional-monotonicity protocol
 * (see draft.tex, Definition "Dimensional monotonicity"), which tests
 *
 *     p^proj(D->d,N,eps)  <=  p(d,N,eps)
 *
 * by projecting every evaluated point of a D-dimensional run onto Omega_d
 * and recording when the projected point reaches the d-dimensional target.
 * The projection pi_{D,d} is plain truncation, which is admissible exactly
 * when it carries the D-dimensional minimizer onto the d-dimensional one
 * (see ProjectionSet). Three differences from the default sweep, each
 * required for the ECDFs the comparison is built from to mean what the
 * definition says:
 *
 *   - hitting times are recorded at every rung of EPS_LADDER, not just at
 *     EPSILON, since DM is quantified over every target accuracy;
 *   - the early-failure short-circuit is off, so a cell's run count is not a
 *     data-dependent stopping time (the stored per-run hitting times are then
 *     a sufficient statistic for every quantile q, not just --q);
 *   - budget-paced algorithms get one budget per cell, chosen by a pilot
 *     phase (see calibrate_budget()) rather than by doubling inside each run,
 *     so every measurement run is an independent draw at one fixed budget N.
 *
 * Output goes to raw_funccraft_dm.csv plus the long-format
 * raw_funccraft_dm_hits.csv, leaving the default sweep's raw_funccraft.csv
 * untouched -- the two protocols are not comparable and must not be mixed in
 * one file.
 *
 * Usage:
 *   run_scaling_experiments [--workers N] [--runs N] [--cap N]
 *                           [--outdir DIR] [--q Q] [--functions I,I,...]
 *                           [--dm]
 *     --workers  worker threads (default: hardware concurrency)
 *     --runs     independent runs per cell (default: 100)
 *     --cap      evaluation budget cap per run (default: 10,000,000)
 *     --outdir   output directory (default: examples/results)
 *     --q        prescribed success probability for the early-failure
 *                short-circuit (default: 0.5); ignored under --dm
 *     --functions  comma-separated 1-based FuncCraft base-function indices
 *                  (1 is Sphere); see FUNCCRAFT_FUNCTIONS_DEFAULT for the
 *                  default set
 *     --dm       record projected hitting times for the dimensional
 *                monotonicity check; rejects the base functions listed in
 *                DM_EXCLUDED_FUNCTIONS
 *
 *   e.g. run_scaling_experiments --workers 1 --runs 5 --cap 20000
 *   e.g. run_scaling_experiments --functions 1,11,13,26
 *   e.g. run_scaling_experiments --dm --runs 30 --cap 200000
 */

#include "funccraft.h"
#include <minion.h>

#include <algorithm>
#include <array>
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

const std::vector<int> FUNCCRAFT_DIMS = {2, 4, 8, 10, 15, 20, 30, 40};
constexpr int FUNCCRAFT_FUNCTION_COUNT = 34;

// =============================================================================
// Dimensional-monotonicity protocol (--dm only)
// =============================================================================

// Target accuracies at which hitting times are recorded, loosest first. DM is
// quantified over every eps, so a violation at 1e-2 falsifies it just as well
// as one at 1e-6; the whole ladder is filled in one pass over the evaluated
// points, at no extra objective evaluations. The tightest rung must be
// EPSILON: it is what success() means, and what retires a projection target
// from further evaluation.
constexpr std::array<double, 5> EPS_LADDER = {1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
constexpr std::size_t EPS_COUNT = EPS_LADDER.size();
constexpr std::size_t EPS_TIGHTEST = EPS_COUNT - 1;
static_assert(EPS_LADDER[EPS_TIGHTEST] == EPSILON,
              "the tightest rung of EPS_LADDER must be the success criterion");

// Pilot phase (see calibrate_budget()): a budget rung is accepted only when
// this many consecutive independent runs all hit.
constexpr int PILOT_RUNS_PER_RUNG = 5;

// Pilot runs draw their seeds from a disjoint region of the run-index space so
// they cannot collide with the measurement runs whose budget they choose.
constexpr int PILOT_RUN_INDEX_BASE = 1'000'000;

// Base functions with no admissible projection, rejected up front under --dm.
// 21/24/25 (LunacekBiRastrigin, Michalewicz, DixonPrice) are the only base
// functions whose x_opt varies across coordinates, so truncation does not
// carry the D-dimensional minimizer onto the d-dimensional one. 19
// (Gallagher21) fails for a stronger reason: its peak layout is seeded with
// the dimension, so f_D and f_d are unrelated landscapes rather than two
// members of one scalable family, and DM is undefined for it.
const std::unordered_set<int> DM_EXCLUDED_FUNCTIONS = {19, 21, 24, 25};

std::string dm_exclusion_reason(int function_number) {
    if (function_number == 19) {
        return "its peak layout is reseeded per dimension, so f_D and f_d are "
               "unrelated landscapes rather than one scalable family";
    }
    return "its x_opt varies across coordinates, so truncation is not an "
           "admissible projection";
}

// FuncCraft base-function indices swept by default (1-based; index 1 is
// Sphere). A curated subset rather than all 34 -- override with --functions.
const std::vector<int> FUNCCRAFT_FUNCTIONS_DEFAULT = {2, 3,16, 23, 26, 30,};  // {1, 9, 10, 12, 19, 34};

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
// Problems: a curated subset of FuncCraft's 34 undecorated base functions
// =============================================================================

struct ProblemSpec {
    int function_number = 1; // 1..34, 1-based; 1 is Sphere
    int dim = 0;
};

std::string problem_label(const ProblemSpec& spec) {
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
    return std::make_unique<FuncCraftBasicProblem>(spec.function_number, spec.dim);
}

// =============================================================================
// Projection targets for the DM check
// =============================================================================

/// The lower-dimensional landscapes a D-dimensional run is projected onto,
/// one per target dimension d < D drawn from FUNCCRAFT_DIMS. Immutable, and
/// built once per cell rather than once per run.
///
/// The admissible projection pi_{D,d} = T_d . P_d . T_D is realised here as
/// the canonical projection P_d alone, with both isometries the identity.
/// That is legitimate only when P_d already carries the D-dimensional
/// minimizer onto the d-dimensional one; for a coordinate-dependent x_opt it
/// does not, and the translated form would be needed instead. Those base
/// functions are rejected by parse_cli() -- the check below is the structural
/// backstop, so a projection can never be silently wrong.
///
/// Because P_d keeps the first d coordinates, the projected point is already
/// the leading prefix of the D-dimensional one: targets are evaluated
/// straight off the original buffer, with no copy and no reordering.
class ProjectionSet {
public:
    struct Target {
        FuncCraft::BasicF f;
        double f_opt = 0.0;
        int dim = 0;
    };

    ProjectionSet(int function_number, int dim_D, const std::vector<int>& candidate_dims) {
        const auto id = static_cast<FuncCraft::BasicFunctionId>(function_number);
        const FuncCraft::BasicF f_D(id, dim_D);
        for (int d : candidate_dims) {
            if (d >= dim_D) continue;
            FuncCraft::BasicF f_d(id, d);
            for (int i = 0; i < d; ++i) {
                const double a = f_D.x_opt[static_cast<std::size_t>(i)];
                const double b = f_d.x_opt[static_cast<std::size_t>(i)];
                if (std::abs(a - b) > 1e-12 * std::max(1.0, std::abs(a))) {
                    throw std::invalid_argument(
                        "FC-F" + std::to_string(function_number) +
                        ": truncation is not an admissible projection from D=" +
                        std::to_string(dim_D) + " to d=" + std::to_string(d) +
                        " (x_opt differs in coordinate " + std::to_string(i) + ")");
                }
            }
            const double f_opt = f_d.f_opt;
            targets_.push_back(Target{std::move(f_d), f_opt, d});
        }
    }

    const std::vector<Target>& targets() const { return targets_; }

private:
    std::vector<Target> targets_;
};

/// First-hit evaluation index at each rung of EPS_LADDER (-1 = never reached)
/// together with the best optimality gap seen, for one target dimension.
///
/// f_best stops advancing once the ladder is complete, because a completed
/// ladder is retired from evaluation. It is therefore the best gap up to
/// retirement, not over the whole run -- exact whenever the tightest rung was
/// never reached (which is the case the diagnostic is for), and merely some
/// value <= EPSILON otherwise. In particular f_best is NOT ordered across
/// target dimensions the way projection dominance orders the gaps
/// pointwise: a low-dimensional target that retires early freezes at a
/// larger value than a higher-dimensional one that kept improving. Order the
/// hitting times, not these.
struct HitLadder {
    std::array<long long, EPS_COUNT> hits{};
    double f_best = std::numeric_limits<double>::infinity();

    HitLadder() { hits.fill(-1); }

    /// Records one observed optimality gap at 1-based evaluation index idx.
    /// Returns true once the tightest rung has been reached, after which
    /// every looser rung is necessarily already filled in and the target can
    /// be retired.
    bool observe(double gap, long long idx) {
        f_best = std::min(f_best, gap);
        if (gap > EPS_LADDER[0]) {
            return hits[EPS_TIGHTEST] >= 0; // too far out to fill any rung
        }
        for (std::size_t j = 0; j < EPS_COUNT; ++j) {
            if (hits[j] < 0 && gap <= EPS_LADDER[j]) hits[j] = idx;
        }
        return hits[EPS_TIGHTEST] >= 0;
    }
};

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
    const std::string repr =
        "funccraft," + std::to_string(spec.function_number) + "," + std::to_string(spec.dim);
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

    // DM instrumentation, inert unless attach_projections() supplied a set.
    // `direct` is the same trajectory as `hit`, but resolved at every rung of
    // EPS_LADDER; `projected[t]` is the trajectory pushed through pi_{D,d}
    // for target t.
    //
    // Note that the optimizer callback stops a run as soon as the
    // D-dimensional target is hit, so the trajectory the projections see is
    // truncated there. That loses nothing *provided the family is
    // dimensionally consistent*: projection dominance gives
    //
    //     f_d(pi(y)) - f_d* <= f_D(y) - f_D*   for every y,
    //
    // so the moment the D-dimensional gap falls to eps, every projected gap
    // is already at or below eps and every target has retired. Where
    // dominance fails the truncation would instead censor the projected
    // hitting times -- which the output makes visible rather than silent: a
    // row whose direct hit is present while a projected hit at the same
    // (D, run, eps) is empty cannot happen under dominance, so it flags a
    // dimensional-consistency failure for that function. Worth asserting
    // over the DM table before reading anything into the ECDFs.
    const ProjectionSet* projections = nullptr;
    HitLadder direct;
    std::vector<HitLadder> projected;
    std::vector<char> projected_active;
    int active_targets = 0;
    bool direct_complete = false;

    void attach_projections(const ProjectionSet* set) {
        projections = set;
        if (set == nullptr) return;
        const std::size_t count = set->targets().size();
        projected.assign(count, HitLadder());
        projected_active.assign(count, 1);
        active_targets = static_cast<int>(count);
    }

    std::vector<double> operator()(const std::vector<std::vector<double>>& X) {
        const std::vector<double> raw = problem->evaluate_raw(X);
        std::vector<double> shifted(raw.size());
        const double fopt = problem->f_opt();
        for (std::size_t i = 0; i < raw.size(); ++i) {
            shifted[i] = raw[i] - fopt;
        }
        for (std::size_t i = 0; i < shifted.size(); ++i) {
            const double gap = shifted[i];
            const long long idx = n + static_cast<long long>(i) + 1;
            if (hit < 0 && gap <= epsilon) hit = idx;
            f_best = std::min(f_best, gap);
            if (projections != nullptr) {
                if (!direct_complete) direct_complete = direct.observe(gap, idx);
                if (active_targets > 0) observe_projected(X[i], idx);
            }
        }
        n += static_cast<long long>(shifted.size());
        return shifted;
    }

    bool success() const { return hit >= 0; }

private:
    /// Evaluates every still-active target on the projection of x. A target
    /// that has reached the tightest rung is retired, so the DM overhead
    /// decays over the run: by projection dominance a projected hit lands no
    /// later than the D-dimensional one, so the low-dimensional targets
    /// typically drop out early.
    void observe_projected(const std::vector<double>& x, long long idx) {
        const std::vector<ProjectionSet::Target>& targets = projections->targets();
        for (std::size_t t = 0; t < targets.size(); ++t) {
            if (projected_active[t] == 0) continue;
            // P_d keeps the leading d coordinates, so x.data() *is* the
            // projected point as far as a d-dimensional BasicF is concerned.
            const double gap = targets[t].f.evaluate(x.data()) - targets[t].f_opt;
            if (projected[t].observe(gap, idx)) {
                projected_active[t] = 0;
                --active_targets;
            }
        }
    }
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
Tracker run_attempt_rr_lbfgsb(const Problem& problem, std::size_t budget, std::uint64_t seed_material,
                              const ProjectionSet* projections) {
    Tracker tracker;
    tracker.problem = &problem;
    tracker.epsilon = EPSILON;
    // One tracker spans every restart, so the projected hitting times are
    // indexed against the run's whole evaluation sequence, not each restart's.
    tracker.attach_projections(projections);

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
                    const std::vector<double>& x0, std::size_t budget, int seed,
                    const ProjectionSet* projections) {
    if (canonical_algo == RR_LBFGSB_CANONICAL) {
        return run_attempt_rr_lbfgsb(problem, budget, static_cast<std::uint64_t>(seed), projections);
    }
    Tracker tracker;
    tracker.problem = &problem;
    tracker.epsilon = EPSILON;
    tracker.attach_projections(projections);
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
    bool budget_censored = false; // --dm only; see calibrate_budget()
};

/// One (target dimension, epsilon) hitting-time record for one run, written
/// to the long-format DM table. Rows with target_d == dim_D carry the direct
/// hitting times that p_{A,f}(d,N,eps) is estimated from; rows with
/// target_d < dim_D carry the projected hitting times behind
/// p^proj(D->d,N,eps). Keeping both in one table makes the DM comparison a
/// self-join rather than a join across two differently shaped files.
///
/// Estimate the probabilities as the fraction of runs with `hit <= N`, which
/// is the definition, and not from the summary table's `success` flag. A
/// population-based optimizer finishes the generation that crosses maxevals,
/// so a run can spend a little past its nominal budget (a couple of percent
/// at most, but nonzero) and can therefore record a hit at an index beyond N.
/// Thresholding on the index keeps the ECDF anchored to the budget the
/// comparison is stated at; the flag does not.
struct DmHitRow {
    std::string algo;
    std::string problem;
    int dim_D = 0;
    int target_d = 0;
    int run = 0;
    double eps = 0.0;
    long long hit = -1;
    double f_gap_best = std::numeric_limits<double>::infinity();
};

/// How one run is to be executed.
struct RunContext {
    long long cap = 0;
    /// > 0: a single attempt at exactly this budget (the --dm protocol).
    /// 0: the default per-run doubling ladder for budget-paced algorithms.
    long long fixed_budget = 0;
    /// Non-null: record projected hitting times for the DM check.
    const ProjectionSet* projections = nullptr;
};

RunResult run_single(const std::string& canonical_algo, const ProblemSpec& spec, int run_index,
                     const RunContext& ctx, std::vector<DmHitRow>* dm_out) {
    const long long cap = ctx.cap;
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

    if (ctx.fixed_budget > 0) {
        // One attempt at one budget: nfev_spent and hit are then on the same
        // clock, which the doubling-inside-a-run branch below cannot offer.
        tracker = run_attempt(canonical_algo, *problem, x0,
                              static_cast<std::size_t>(ctx.fixed_budget), seed, ctx.projections);
        spent = tracker.n;
        final_budget = ctx.fixed_budget;
    } else if (BUDGET_PACED.count(canonical_algo) != 0) {
        long long budget = std::max<long long>(
            static_cast<long long>(BUDGET0_PER_DIM) * problem->dim(), 100);
        while (true) {
            tracker = run_attempt(canonical_algo, *problem, x0,
                                  static_cast<std::size_t>(budget), seed, ctx.projections);
            // Note that spent accumulates over every rung while tracker holds
            // only the last one, so nfev_spent and hit are not on the same
            // clock here. That is why --dm never takes this branch.
            spent += tracker.n;
            if (tracker.success() || budget >= cap) break;
            budget = std::min<long long>(budget * 2, cap);
        }
        final_budget = budget;
    } else {
        tracker = run_attempt(canonical_algo, *problem, x0, static_cast<std::size_t>(cap), seed,
                              ctx.projections);
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

    if (dm_out != nullptr && ctx.projections != nullptr) {
        auto emit = [&](int target_d, const HitLadder& ladder) {
            for (std::size_t j = 0; j < EPS_COUNT; ++j) {
                DmHitRow row;
                row.algo = r.algo;
                row.problem = r.problem;
                row.dim_D = r.dim;
                row.target_d = target_d;
                row.run = r.run;
                row.eps = EPS_LADDER[j];
                row.hit = ladder.hits[j];
                row.f_gap_best = ladder.f_best;
                dm_out->push_back(std::move(row));
            }
        };
        emit(r.dim, tracker.direct);
        const std::vector<ProjectionSet::Target>& targets = ctx.projections->targets();
        for (std::size_t t = 0; t < targets.size(); ++t) {
            emit(targets[t].dim, tracker.projected[t]);
        }
    }
    return r;
}

/// Result of the pilot phase: the one evaluation budget every measurement run
/// of a cell will use.
struct CellBudget {
    long long budget = 0;
    bool censored = false; // the criterion was never met, even at the cap
    int pilot_runs = 0;
};

/// Pilot phase for budget-paced algorithms under --dm. Doubles the budget
/// from 100*D until PILOT_RUNS_PER_RUNG consecutive independent runs all hit,
/// and returns that budget; every measurement run of the cell is then a
/// single attempt at it.
///
/// Two reasons this replaces the doubling-inside-each-run protocol. First,
/// p_{A,f}(D,N,eps) is defined over runs at one fixed budget N, and a ladder
/// executed inside a run is not a run at any single budget -- for these
/// algorithms the budget drives the internal schedule, so each rung is a
/// materially different search. Second, requiring all of them to hit targets
/// a per-run success probability near 1, so the recorded hitting times
/// support reconstructing N_req at any quantile q rather than only at the low
/// q where a first-hit rung happens to land.
///
/// Pilot runs are never reported. The budget is chosen from their outcomes,
/// so including them would condition the sample on its own success; the
/// disjoint seed domain keeps them from recurring as measurement runs. A rung
/// is abandoned at its first failure, since the criterion is then already out
/// of reach.
CellBudget calibrate_budget(const std::string& canonical_algo, const ProblemSpec& spec,
                            long long cap) {
    CellBudget out;
    // Clamped to the cap, so that a cell whose first rung already exceeds it
    // (100*D > cap, which happens at high D under a small --cap) still spends
    // only what the cap allows.
    long long budget = std::min<long long>(
        std::max<long long>(static_cast<long long>(BUDGET0_PER_DIM) * spec.dim, 100), cap);
    int rung = 0;
    while (true) {
        RunContext pilot_ctx;
        pilot_ctx.cap = cap;
        pilot_ctx.fixed_budget = budget;
        pilot_ctx.projections = nullptr; // the pilot only needs hit / no-hit

        bool all_hit = true;
        for (int p = 0; p < PILOT_RUNS_PER_RUNG; ++p) {
            const int pilot_index =
                PILOT_RUN_INDEX_BASE + rung * PILOT_RUNS_PER_RUNG + p;
            const RunResult r = run_single(canonical_algo, spec, pilot_index, pilot_ctx, nullptr);
            ++out.pilot_runs;
            if (!r.success) {
                all_hit = false;
                break;
            }
        }
        if (all_hit) {
            out.budget = budget;
            return out;
        }
        if (budget >= cap) {
            out.budget = cap;
            out.censored = true;
            return out;
        }
        budget = std::min<long long>(budget * 2, cap);
        ++rung;
    }
}

// =============================================================================
// Cell runner: all runs for one (algorithm, problem, dimension) cell
// =============================================================================

int required_rank(double q, int n_runs) {
    return static_cast<int>(std::ceil(q * static_cast<double>(n_runs)));
}

struct CellOutput {
    std::vector<RunResult> rows;
    std::vector<DmHitRow> dm_rows;
    CellBudget budget;
};

/// Terminates early once the target success probability has become
/// unreachable, exactly as scaling_lab.py's run_cell(): with n_runs runs and
/// rank k, n_runs-k+1 observed failures makes fewer than k successes
/// certain, so the result is censored no matter what the remaining runs do.
///
/// Under --dm every run is kept instead. That short-circuit estimates N_req
/// at one prescribed q, but it stops on a rule that depends on the outcomes
/// being measured, so the ECDF over the surviving prefix is not an unbiased
/// estimate of p_{A,f}(., N, eps) -- and the DM comparison is an inequality
/// between two such ECDFs. Keeping every run also leaves q a post-hoc
/// analysis parameter: hitting times per run are a sufficient statistic for
/// every quantile, not just --q.
CellOutput run_cell(const std::string& canonical_algo, const ProblemSpec& spec,
                    int n_runs, double q, long long cap, bool dm) {
    CellOutput out;
    std::unique_ptr<ProjectionSet> projections;

    RunContext ctx;
    ctx.cap = cap;
    if (dm) {
        projections = std::make_unique<ProjectionSet>(spec.function_number, spec.dim,
                                                      FUNCCRAFT_DIMS);
        ctx.projections = projections.get();
        out.budget = (BUDGET_PACED.count(canonical_algo) != 0)
                         ? calibrate_budget(canonical_algo, spec, cap)
                         : CellBudget{cap, false, 0};
        ctx.fixed_budget = out.budget.budget;
    }

    const int k = required_rank(q, n_runs);
    const int max_failures = n_runs - k + 1;
    out.rows.reserve(static_cast<std::size_t>(n_runs));
    int failures = 0;
    for (int r = 0; r < n_runs; ++r) {
        RunResult row = run_single(canonical_algo, spec, r, ctx, dm ? &out.dm_rows : nullptr);
        row.budget_censored = out.budget.censored;
        const bool ok = row.success;
        out.rows.push_back(std::move(row));
        failures += ok ? 0 : 1;
        if (!dm && failures >= max_failures) break;
    }
    return out;
}

// =============================================================================
// CSV output: same columns as run_experiments.py's raw_<which>.csv
// =============================================================================

/// Writes the per-run summary, and under --dm the long-format hitting-time
/// table alongside it. Both files are written under one lock, hits first, so
/// an interrupted sweep can at worst leave orphan hit rows for a cell that
/// resume will re-run -- deduplicate on
/// (algo, problem, dim_D, target_d, run, eps). Ordering them the other way
/// would instead lose hit rows silently, since resume keys off the summary.
class CsvWriter {
public:
    /// dm_path empty selects the default sweep's single-file output.
    CsvWriter(std::filesystem::path path, std::filesystem::path dm_path)
        : path_(std::move(path)), dm_path_(std::move(dm_path)) {
        std::error_code ec;
        header_written_ = std::filesystem::exists(path_, ec) &&
                          std::filesystem::file_size(path_, ec) > 0;
        dm_header_written_ = !dm_path_.empty() && std::filesystem::exists(dm_path_, ec) &&
                             std::filesystem::file_size(dm_path_, ec) > 0;
    }

    void append(const std::vector<RunResult>& rows, const std::vector<DmHitRow>& dm_rows) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!dm_path_.empty()) {
            std::ofstream dm_out(dm_path_, std::ios::app);
            if (!dm_header_written_) {
                dm_out << "algo,problem,dim_D,target_d,run,eps,hit,f_gap_best\n";
                dm_header_written_ = true;
            }
            for (const auto& row : dm_rows) {
                dm_out << row.algo << ',' << row.problem << ',' << row.dim_D << ','
                       << row.target_d << ',' << row.run << ',';
                dm_out << std::scientific << std::setprecision(1) << row.eps << ',';
                if (row.hit >= 0) {
                    dm_out << row.hit;
                }
                dm_out << ',';
                dm_out << std::scientific << std::setprecision(10) << row.f_gap_best << '\n';
            }
        }

        std::ofstream out(path_, std::ios::app);
        if (!header_written_) {
            out << "algo,problem,dim,run,hit,success,nfev_spent,final_budget,f_gap_best,seconds";
            out << (dm_path_.empty() ? "\n" : ",budget_censored\n");
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
            out << std::fixed << std::setprecision(6) << row.seconds;
            if (!dm_path_.empty()) {
                out << ',' << (row.budget_censored ? "True" : "False");
            }
            out << '\n';
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
    std::filesystem::path dm_path_;
    bool header_written_ = false;
    bool dm_header_written_ = false;
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
std::vector<Cell> build_cells(const std::vector<int>& function_indices) {
    const std::vector<std::string> algos = canonical_algos();
    std::vector<Cell> cells;
    for (int fn : function_indices) {
        for (int d : FUNCCRAFT_DIMS) {
            for (const auto& algo : algos) {
                ProblemSpec spec;
                spec.function_number = fn;
                spec.dim = d;
                cells.push_back({algo, spec});
            }
        }
    }
    return cells;
}

struct Config {
    int nthreads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    int n_runs = N_RUNS_DEFAULT;
    long long cap = MAXEVALS_CAP_DEFAULT;
    std::string out_dir = "examples/results";
    double q = Q_DEFAULT;
    std::vector<int> function_indices = FUNCCRAFT_FUNCTIONS_DEFAULT;
    bool dm = false;
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

/// Parses "--functions 1,9,10,19,21" into 1-based FuncCraft base-function
/// indices; index 1 is Sphere.
std::vector<int> parse_function_indices(const std::string& text) {
    std::vector<int> indices;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        indices.push_back(static_cast<int>(parse_integer_arg(token.c_str(), "--functions")));
    }
    if (indices.empty()) {
        throw std::invalid_argument("--functions requires at least one index");
    }
    return indices;
}

/// Flag-style CLI, mirroring run_experiments.py's argparse conventions
/// (--workers, --runs, --cap, --q). No positional arguments: there is only
/// one problem family now that Sphere is FuncCraft base-function index 1.
Config parse_cli(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next_value = [&](const std::string& flag) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(flag + " requires a value");
            return argv[++i];
        };
        if (arg == "--workers" || arg == "--nthreads") {
            config.nthreads = static_cast<int>(parse_integer_arg(next_value(arg).c_str(), arg));
        } else if (arg == "--runs") {
            config.n_runs = static_cast<int>(parse_integer_arg(next_value(arg).c_str(), arg));
        } else if (arg == "--cap") {
            config.cap = parse_integer_arg(next_value(arg).c_str(), arg);
        } else if (arg == "--outdir" || arg == "--out-dir") {
            config.out_dir = next_value(arg);
        } else if (arg == "--q") {
            config.q = parse_double_arg(next_value(arg).c_str(), arg);
        } else if (arg == "--functions") {
            config.function_indices = parse_function_indices(next_value(arg));
        } else if (arg == "--dm") {
            config.dm = true;
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }

    if (config.nthreads < 1) throw std::invalid_argument("--workers must be at least 1");
    if (config.n_runs < 1) throw std::invalid_argument("--runs must be at least 1");
    if (config.cap < 1) throw std::invalid_argument("--cap must be at least 1");
    if (config.q <= 0.0 || config.q > 1.0) throw std::invalid_argument("--q must be in (0, 1]");
    for (int fn : config.function_indices) {
        if (fn < 1 || fn > FUNCCRAFT_FUNCTION_COUNT) {
            throw std::invalid_argument(
                "--functions indices must be in [1, " +
                std::to_string(FUNCCRAFT_FUNCTION_COUNT) + "] (1-based; 1 is Sphere)");
        }
    }
    // Rejected before a single cell runs, rather than part-way through the
    // sweep: for these there is no admissible projection to record against.
    if (config.dm) {
        for (int fn : config.function_indices) {
            if (DM_EXCLUDED_FUNCTIONS.count(fn) != 0) {
                throw std::invalid_argument(
                    "--dm does not support FC-F" + std::to_string(fn) + ": " +
                    dm_exclusion_reason(fn) + ". Drop it from --functions.");
            }
        }
    }
    return config;
}

void run_jobs(std::vector<Cell>& cells, const Config& config,
             CsvWriter& writer, const std::unordered_set<std::string>& done_keys) {
    std::vector<std::size_t> todo;
    todo.reserve(cells.size());
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const Cell& cell = cells[i];
        if (done_keys.count(cell_key(cell.canonical_algo, cell.spec)) == 0) {
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
                CellOutput cell_out = run_cell(cell.canonical_algo, cell.spec,
                                               config.n_runs, config.q, config.cap, config.dm);
                const std::chrono::duration<double> dt = std::chrono::steady_clock::now() - t0;

                writer.append(cell_out.rows, cell_out.dm_rows);

                const std::vector<RunResult>& rows = cell_out.rows;
                const int n_ok = static_cast<int>(std::count_if(
                    rows.begin(), rows.end(), [](const RunResult& r) { return r.success; }));
                const std::size_t done_count = finished.fetch_add(1) + 1;
                const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t_start;

                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "[" << std::setw(5) << done_count << "/" << todo.size() << "] "
                          << std::left << std::setw(10) << problem_label(cell.spec)
                          << "D=" << std::setw(4) << cell.spec.dim
                          << std::setw(14) << cell.canonical_algo
                          << std::right << std::setw(3) << n_ok << "/" << rows.size() << " ok  "
                          << "cell " << std::fixed << std::setprecision(1) << std::setw(7) << dt.count()
                          << "s  elapsed " << std::setw(8) << elapsed.count() << "s";
                if (config.dm) {
                    std::cout << "  N=" << cell_out.budget.budget;
                    if (cell_out.budget.pilot_runs > 0) {
                        std::cout << " (pilot " << cell_out.budget.pilot_runs << ")";
                    }
                    if (cell_out.budget.censored) std::cout << " CENSORED";
                }
                std::cout << "\n" << std::flush;
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

        // The two protocols differ in budget pacing and in run-count
        // censoring, so their rows are not comparable and go to separate
        // files -- resume then also keys off the right one.
        const std::filesystem::path out_root(config.out_dir);
        const std::filesystem::path funccraft_path =
            out_root / (config.dm ? "raw_funccraft_dm.csv" : "raw_funccraft.csv");
        const std::filesystem::path dm_hits_path =
            config.dm ? out_root / "raw_funccraft_dm_hits.csv" : std::filesystem::path();
        CsvWriter writer(funccraft_path, dm_hits_path);
        const std::unordered_set<std::string> done_keys = writer.already_done();

        std::vector<Cell> cells = build_cells(config.function_indices);

        std::cout << "Usage: run_scaling_experiments [--workers N] [--runs N] "
                     "[--cap N] [--outdir DIR] [--q Q] [--functions I,I,...] [--dm]\n";
        std::cout << "nthreads: " << config.nthreads
                  << ", runs: " << config.n_runs
                  << ", cap: " << config.cap
                  << ", q: " << (config.dm ? "n/a (--dm keeps every run)" : std::to_string(config.q))
                  << ", out_dir: " << config.out_dir << "\n";
        if (config.dm) {
            std::cout << "mode: dimensional monotonicity -- eps ladder";
            for (std::size_t j = 0; j < EPS_COUNT; ++j) {
                std::cout << ' ' << EPS_LADDER[j];
            }
            std::cout << ", pilot " << PILOT_RUNS_PER_RUNG << "/" << PILOT_RUNS_PER_RUNG
                      << " per budget rung\n";
        }
        std::cout << "algorithms:";
        for (const auto& a : canonical_algos()) std::cout << ' ' << a;
        std::cout << "\n";
        auto print_dims = [](const std::vector<int>& dims) {
            for (std::size_t i = 0; i < dims.size(); ++i) {
                if (i) std::cout << ',';
                std::cout << dims[i];
            }
        };
        std::cout << "problems: FuncCraft-F{";
        for (std::size_t i = 0; i < config.function_indices.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << config.function_indices[i];
        }
        std::cout << "}(D=";
        print_dims(FUNCCRAFT_DIMS);
        std::cout << ")";
        std::cout << "\ntotal cells: " << cells.size() << "\n\n" << std::flush;

        const auto t_start = std::chrono::steady_clock::now();
        run_jobs(cells, config, writer, done_keys);
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t_start;

        std::cout << "\nDone in " << std::fixed << std::setprecision(1) << elapsed.count()
                  << "s\n";
        std::cout << "  -> " << funccraft_path.string() << "\n";
        if (config.dm) {
            std::cout << "  -> " << dm_hits_path.string() << "\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_scaling_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
