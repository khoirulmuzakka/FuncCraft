#include "funccraft.h"
#include <minion.h>

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct Config {
    std::string out_dir = "scripts/results/paper_experiment";
    int dimension = 10;
    int runs = 21;
    int base_function_count = 34;
    int composition_instances = 30;
    std::string algo = "ARRDE";
    int population_size = 0;
    unsigned long long seed = 1;
    int nthreads = 8;
};

struct Treatment {
    std::string experiment;
    std::string name;
};

struct ExperimentBlock {
    Treatment treatment;
    std::size_t max_evals = 0;
    std::vector<int> indices;
    std::vector<const FuncCraft::BenchmarkFunction*> functions;
    std::vector<std::vector<double>> matrix;
    std::filesystem::path output_path;
};

struct Job {
    std::size_t block_index = 0;
    int run_index = 0;
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

int parse_int_arg(const char* text, const std::string& name) {
    const long long value = parse_integer_arg(text, name);
    if (value < INT_MIN || value > INT_MAX) {
        throw std::out_of_range(name + " is outside the int range");
    }
    return static_cast<int>(value);
}

Config parse_cli(int argc, char* argv[]) {
    Config config;
    if (argc > 2) {
        throw std::invalid_argument("expected at most one argument: Nthreads");
    }
    if (argc == 2) {
        config.nthreads = parse_int_arg(argv[1], "Nthreads");
    }
    if (config.nthreads < 1) {
        throw std::invalid_argument("Nthreads must be at least 1");
    }
    return config;
}

std::uint64_t mix_seed(std::uint64_t seed, std::uint64_t value) {
    seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6U) + (seed >> 2U);
    return seed;
}

std::vector<std::pair<int, std::size_t>> budgets(int dimension) {
    return {
        {100, static_cast<std::size_t>(100 * dimension)},
        {1000, static_cast<std::size_t>(1000 * dimension)},
        {3000, static_cast<std::size_t>(3000 * dimension)},
        {10000, static_cast<std::size_t>(10000 * dimension)},
    };
}

std::vector<int> one_based_range(int first, int count) {
    std::vector<int> indices;
    indices.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        indices.push_back(first + i);
    }
    return indices;
}

std::vector<double> random_start(const FuncCraft::Domain& domain, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<double> x0(static_cast<std::size_t>(domain.dimension()));
    for (int d = 0; d < domain.dimension(); ++d) {
        const auto idx = static_cast<std::size_t>(d);
        std::uniform_real_distribution<double> dist(domain.lower[idx], domain.upper[idx]);
        x0[idx] = dist(rng);
    }
    return x0;
}

double minimize_function(
    const FuncCraft::BenchmarkFunction& function,
    const Config& config,
    std::size_t max_evals,
    int run_index,
    int suite_index) {
    const auto domain = function.domain();
    std::vector<std::pair<double, double>> bounds;
    bounds.reserve(static_cast<std::size_t>(domain.dimension()));
    for (int d = 0; d < domain.dimension(); ++d) {
        const auto idx = static_cast<std::size_t>(d);
        bounds.emplace_back(domain.lower[idx], domain.upper[idx]);
    }

    const std::uint64_t run_seed = mix_seed(
        mix_seed(config.seed, static_cast<std::uint64_t>(run_index + 1)),
        static_cast<std::uint64_t>(suite_index));
    std::vector<double> x0 = random_start(domain, run_seed);

    auto objective = [&function](const std::vector<std::vector<double>>& x, void*) {
        return function(x);
    };

    auto settings = minion::DefaultSettings().getDefaultSettings(config.algo);
    settings["convergence_tol"] = 0.0;
    settings["population_size"] = config.population_size;

    minion::Minimizer optimizer(
        objective,
        bounds,
        x0,
        nullptr,
        nullptr,
        config.algo,
        max_evals,
        static_cast<int>(run_seed % static_cast<std::uint64_t>(std::numeric_limits<int>::max())),
        settings);
    const minion::MinionResult result = optimizer.optimize();
    return result.fun;
}

FuncCraft::SuiteSpec base_suite_spec() {
    FuncCraft::SuiteSpec spec = FuncCraft::suite_collection_spec(2026, 1);
    spec.requested_number_of_functions = 100;
    return spec;
}

FuncCraft::BenchmarkSuite coordinate_suite(FuncCraft::CoordinateTransformKind kind, int dimension) {
    FuncCraft::SuiteSpec spec = base_suite_spec();
    spec.coordinate_transforms = {FuncCraft::make_choice(kind, 1.0)};
    spec.value_transforms = {FuncCraft::make_choice(FuncCraft::ValueTransformKind::None, 1.0)};
    return FuncCraft::make_benchmark_suite(spec, dimension);
}

FuncCraft::BenchmarkSuite value_suite(
    FuncCraft::ValueTransformKind kind,
    std::vector<double> parameters,
    int dimension) {
    FuncCraft::SuiteSpec spec = base_suite_spec();
    spec.coordinate_transforms = {FuncCraft::make_choice(FuncCraft::CoordinateTransformKind::Rotation, 1.0)};
    spec.value_transforms = {FuncCraft::make_choice(kind, 1.0, std::move(parameters))};
    FuncCraft::BenchmarkSuite suite = FuncCraft::make_benchmark_suite(spec, dimension);
    suite.apply_value_transforms_to_basic = true;
    return suite;
}

FuncCraft::BenchmarkSuite composition_suite(
    FuncCraft::CompositionKind kind,
    std::vector<double> parameters,
    int dimension) {
    FuncCraft::SuiteSpec spec = base_suite_spec();
    spec.coordinate_transforms = {FuncCraft::make_choice(FuncCraft::CoordinateTransformKind::Rotation, 1.0)};
    spec.value_transforms = {FuncCraft::make_choice(FuncCraft::ValueTransformKind::None, 1.0)};
    spec.compositions = {FuncCraft::make_choice(kind, 1.0, std::move(parameters))};
    return FuncCraft::make_benchmark_suite(spec, dimension);
}

void write_manifest(
    const std::filesystem::path& path,
    const Config& config,
    const std::vector<Treatment>& treatments) {
    std::ofstream out(path);
    out << "suite 2026_v1\n";
    out << "dimension " << config.dimension << '\n';
    out << "runs " << config.runs << '\n';
    out << "assigned_fopt 100\n";
    out << "base_function_indices F1-F" << config.base_function_count << '\n';
    out << "composition_source_indices F41-F" << (40 + config.composition_instances) << '\n';
    out << "treatments\n";
    for (const Treatment& treatment : treatments) {
        out << treatment.experiment << ' ' << treatment.name << '\n';
    }
}

void write_matrix(
    const std::filesystem::path& path,
    const std::vector<std::vector<double>>& values) {
    std::ofstream out(path);
    out << std::scientific << std::setprecision(16);
    for (const auto& row : values) {
        for (std::size_t j = 0; j < row.size(); ++j) {
            if (j > 0) {
                out << ' ';
            }
            out << row[j];
        }
        out << '\n';
    }
}

std::filesystem::path result_path(
    const Config& config,
    const Treatment& treatment,
    std::size_t max_evals) {
    return std::filesystem::path(config.out_dir)
        / (config.algo + "_" + std::to_string(config.dimension) + "D_"
           + std::to_string(max_evals) + "_" + treatment.experiment + "_"
           + treatment.name + ".txt");
}

std::vector<Treatment> experiment_treatments() {
    return {
        {"coord", "NONE"},
        {"coord", "ROT"},
        {"coord", "AFF"},
        {"value", "NONE"},
        {"value", "COSZERO"},
        {"value", "OSC"},
        {"value", "POWER_P01"},
        {"value", "POWER_P10"},
        {"composition", "CPM_SUM"},
        {"composition", "CPM_PMEAN"},
        {"composition", "CPM_LWELL"},
        {"composition", "DPM_SOFTMAX"},
    };
}

FuncCraft::BenchmarkSuite treatment_suite(const Treatment& treatment, int dimension) {
    if (treatment.experiment == "coord" && treatment.name == "NONE") {
        return coordinate_suite(FuncCraft::CoordinateTransformKind::None, dimension);
    }
    if (treatment.experiment == "coord" && treatment.name == "ROT") {
        return coordinate_suite(FuncCraft::CoordinateTransformKind::Rotation, dimension);
    }
    if (treatment.experiment == "coord" && treatment.name == "AFF") {
        return coordinate_suite(FuncCraft::CoordinateTransformKind::Affine, dimension);
    }
    if (treatment.experiment == "value" && treatment.name == "NONE") {
        return value_suite(FuncCraft::ValueTransformKind::None, {}, dimension);
    }
    if (treatment.experiment == "value" && treatment.name == "COSZERO") {
        return value_suite(FuncCraft::ValueTransformKind::CosineZero, {0.10}, dimension);
    }
    if (treatment.experiment == "value" && treatment.name == "OSC") {
        return value_suite(FuncCraft::ValueTransformKind::Oscillatory, {0.40, 0.03}, dimension);
    }
    if (treatment.experiment == "value" && treatment.name == "POWER_P01") {
        return value_suite(FuncCraft::ValueTransformKind::Power, {1.0, 0.1}, dimension);
    }
    if (treatment.experiment == "value" && treatment.name == "POWER_P10") {
        return value_suite(FuncCraft::ValueTransformKind::Power, {1.0, 10.0}, dimension);
    }
    if (treatment.experiment == "composition" && treatment.name == "CPM_SUM") {
        return composition_suite(FuncCraft::CompositionKind::CpmWeightedSum, {}, dimension);
    }
    if (treatment.experiment == "composition" && treatment.name == "CPM_PMEAN") {
        return composition_suite(FuncCraft::CompositionKind::CpmPowerMean, {2.0}, dimension);
    }
    if (treatment.experiment == "composition" && treatment.name == "CPM_LWELL") {
        return composition_suite(FuncCraft::CompositionKind::CpmLevelWell, {0.40, 0.03}, dimension);
    }
    if (treatment.experiment == "composition" && treatment.name == "DPM_SOFTMAX") {
        return composition_suite(FuncCraft::CompositionKind::DpmSoftmax, {0.03}, dimension);
    }
    throw std::logic_error("unknown treatment");
}

std::vector<int> treatment_indices(const Treatment& treatment, const Config& config) {
    return treatment.experiment == "composition"
        ? one_based_range(41, config.composition_instances)
        : one_based_range(1, config.base_function_count);
}

void run_jobs(
    std::vector<ExperimentBlock>& blocks,
    const std::vector<Job>& jobs,
    const Config& config) {
    std::atomic<std::size_t> next_job{0};
    std::atomic<std::size_t> completed_jobs{0};
    std::mutex output_mutex;
    std::mutex error_mutex;
    std::exception_ptr first_error;

    const int worker_count = std::min(config.nthreads, static_cast<int>(jobs.size()));
    const std::size_t progress_interval = std::max<std::size_t>(1, jobs.size() / 20);
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(worker_count));

    for (int worker = 0; worker < worker_count; ++worker) {
        workers.emplace_back([&]() {
            while (true) {
                const std::size_t job_index = next_job.fetch_add(1);
                if (job_index >= jobs.size()) {
                    break;
                }

                const Job& job = jobs[job_index];
                ExperimentBlock& block = blocks[job.block_index];
                try {
                    for (std::size_t column = 0; column < block.functions.size(); ++column) {
                        block.matrix[static_cast<std::size_t>(job.run_index)][column] =
                            minimize_function(
                                *block.functions[column],
                                config,
                                block.max_evals,
                                job.run_index,
                        block.indices[column]);
                    }

                    const std::size_t done = completed_jobs.fetch_add(1) + 1;
                    if (done == jobs.size() || done % progress_interval == 0) {
                        std::lock_guard<std::mutex> lock(output_mutex);
                        std::cout << "Completed " << done << '/' << jobs.size() << " run jobs\n";
                    }
                } catch (...) {
                    std::lock_guard<std::mutex> lock(error_mutex);
                    if (!first_error) {
                        first_error = std::current_exception();
                    }
                }
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const Config config = parse_cli(argc, argv);
        std::filesystem::create_directories(config.out_dir);

        const std::vector<Treatment> treatments = experiment_treatments();
        const std::vector<std::pair<int, std::size_t>> eval_budgets = budgets(config.dimension);
        std::vector<std::unique_ptr<FuncCraft::BenchmarkSuite>> suites;
        std::vector<ExperimentBlock> blocks;
        std::vector<Job> jobs;

        const auto start_time = std::chrono::steady_clock::now();
        write_manifest(std::filesystem::path(config.out_dir) / "experiment_manifest.txt", config, treatments);

        std::cout << "Usage: run_experiments [Nthreads]\n";
        std::cout << "Output directory: " << config.out_dir << "\n";
        std::cout << "Suite: 2026_v1"
                  << ", dimension: " << config.dimension
                  << ", runs: " << config.runs
                  << ", base functions: F1-F" << config.base_function_count
                  << ", composition functions: F41-F" << (40 + config.composition_instances)
                  << ", algo: " << config.algo
                  << ", population_size: " << config.population_size
                  << ", seed: " << config.seed
                  << ", Nthreads: " << config.nthreads << "\n\n";

        for (const Treatment& treatment : treatments) {
            suites.push_back(std::make_unique<FuncCraft::BenchmarkSuite>(treatment_suite(treatment, config.dimension)));
            const FuncCraft::BenchmarkSuite& suite = *suites.back();
            const std::vector<int> indices = treatment_indices(treatment, config);
            std::vector<const FuncCraft::BenchmarkFunction*> functions;
            functions.reserve(indices.size());
            for (int index : indices) {
                functions.push_back(&suite.function(index));
            }

            for (const auto& budget : eval_budgets) {
                const std::size_t max_evals = budget.second;
                ExperimentBlock block;
                block.treatment = treatment;
                block.max_evals = max_evals;
                block.indices = indices;
                block.functions = functions;
                block.matrix.assign(
                    static_cast<std::size_t>(config.runs),
                    std::vector<double>(indices.size(), std::numeric_limits<double>::quiet_NaN()));
                block.output_path = result_path(config, treatment, max_evals);
                blocks.push_back(std::move(block));
                const std::size_t block_index = blocks.size() - 1;
                for (int run = 0; run < config.runs; ++run) {
                    jobs.push_back(Job{block_index, run});
                }
            }
        }

        std::cout << "Prepared " << blocks.size() << " result blocks and "
                  << jobs.size() << " run jobs. Worker threads: "
                  << std::min(config.nthreads, static_cast<int>(jobs.size()))
                  << "\n";

        run_jobs(blocks, jobs, config);

        for (const ExperimentBlock& block : blocks) {
            write_matrix(block.output_path, block.matrix);
            std::cout << "Wrote " << block.output_path.string() << "\n";
        }

        const auto end_time = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "\nTotal elapsed time: "
                  << std::fixed << std::setprecision(3)
                  << elapsed.count() << " seconds\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
