#include "benchmark.h"
#include "builder.h"
#include "default_options.h"
#include "minimizer.h"
#include "support.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Config {
    std::string out_dir = "results";
    int dimension = 10;
    int runs = 21;
    int basic_count = 28;
    int composition_count = 30;
    int threads = 128;
    unsigned long long seed = 1;
    std::string algo = "ARRDE";
    double assigned_fmin = 100.0;
    double lower_bound = -100.0;
    double upper_bound = 100.0;
};

struct Treatment {
    std::string experiment;
    std::string label;
    std::vector<FuncCraft::ComposedFunction> functions;
};

Config parse_cli(int argc, char* argv[]) {
    Config config;
    if (argc > 1) config.out_dir = argv[1];
    if (argc > 2) config.dimension = std::max(2, std::atoi(argv[2]));
    if (argc > 3) config.runs = std::max(1, std::atoi(argv[3]));
    if (argc > 4) config.basic_count = std::max(1, std::atoi(argv[4]));
    if (argc > 5) config.composition_count = std::max(1, std::atoi(argv[5]));
    if (argc > 6) config.threads = std::max(1, std::atoi(argv[6]));
    if (argc > 7) config.seed = static_cast<unsigned long long>(std::strtoull(argv[7], nullptr, 10));
    if (argc > 8) config.algo = argv[8];
    return config;
}

std::vector<std::pair<double, double>> bounds_from_domain(const FuncCraft::Domain& domain) {
    std::vector<std::pair<double, double>> bounds;
    bounds.reserve(domain.lower.size());
    for (std::size_t i = 0; i < domain.lower.size(); ++i) {
        bounds.emplace_back(domain.lower[i], domain.upper[i]);
    }
    return bounds;
}

std::vector<std::vector<double>> initial_guesses(const FuncCraft::ComposedFunction& f) {
    std::vector<double> guess(static_cast<std::size_t>(f.dimension()), 0.0);
    for (int i = 0; i < f.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        guess[idx] = f.domain().lower[idx] + 0.63 * (f.domain().upper[idx] - f.domain().lower[idx]);
    }
    return {guess};
}

std::vector<double> evaluate_funcraft_batch(
    const std::vector<std::vector<double>>& xs,
    void* data) {
    const auto* function = static_cast<const FuncCraft::ComposedFunction*>(data);
    if (function == nullptr) {
        throw std::invalid_argument("FuncCraft function pointer must not be null");
    }
    return (*function)(xs);
}

std::vector<double> zero_point(int dimension) {
    return std::vector<double>(static_cast<std::size_t>(dimension), 0.0);
}

std::vector<double> random_point_in_domain(
    const FuncCraft::Domain& domain,
    unsigned long long seed) {
    std::mt19937_64 rng(FuncCraft::detail::mix_seed(seed));
    std::vector<double> point(static_cast<std::size_t>(domain.dimension()), 0.0);
    for (int i = 0; i < domain.dimension(); ++i) {
        std::uniform_real_distribution<double> dist(domain.lower[static_cast<std::size_t>(i)], domain.upper[static_cast<std::size_t>(i)]);
        point[static_cast<std::size_t>(i)] = dist(rng);
    }
    return point;
}

std::string sanitize(std::string value) {
    for (char& ch : value) {
        if (ch == '-' || ch == ';' || ch == '=' || ch == '+' || ch == ' ') {
            ch = '_';
        }
    }
    return value;
}

std::string trim_label(const std::string& label, std::size_t width) {
    if (label.size() <= width) {
        return label;
    }
    if (width <= 3) {
        return label.substr(0, width);
    }
    return label.substr(0, width - 3) + "...";
}

std::shared_ptr<FuncCraft::CoordinateTransform> make_coordinate_transform(
    FuncCraft::CoordinateTransformClass cls,
    int dimension,
    const std::vector<double>& x_star,
    std::mt19937_64& rng) {
    using namespace FuncCraft;
    switch (cls) {
    case CoordinateTransformClass::None:
        return std::make_shared<IdentityTransform>(dimension);
    case CoordinateTransformClass::Rotation:
        return std::make_shared<RotationTransform>(detail::random_rotation_matrix(rng, dimension), x_star);
    case CoordinateTransformClass::Affine:
        return std::make_shared<AffineTransform>(
            detail::random_affine_matrix(rng, dimension),
            x_star,
            zero_point(dimension));
    default:
        throw std::invalid_argument("unsupported coordinate treatment");
    }
}

std::shared_ptr<FuncCraft::ValueTransform> make_experiment_value_transform(
    FuncCraft::ValueTransformClass cls) {
    using namespace FuncCraft;
    switch (cls) {
    case ValueTransformClass::None:
        return std::make_shared<IdentityValueTransform>();
    case ValueTransformClass::CosineZero:
        return std::make_shared<CosineZeroValueTransform>(0.10);
    case ValueTransformClass::Oscillatory:
        return std::make_shared<OscillatoryValueTransform>(0.40, 0.03);
    case ValueTransformClass::Power:
        return std::make_shared<PowerValueTransform>(1.0, 2.0);
    default:
        throw std::invalid_argument("unsupported value treatment");
    }
}

std::shared_ptr<FuncCraft::CompositionFunction> make_composition(
    FuncCraft::CompositionClass cls,
    int components,
    const std::vector<std::vector<double>>& centers) {
    using namespace FuncCraft;
    std::vector<double> weights(static_cast<std::size_t>(components), 1.0);
    switch (cls) {
    case CompositionClass::CommonPointWeightedSum:
        return std::make_shared<WeightedSumComposition>(weights);
    case CompositionClass::CommonPointPowerMean:
        return std::make_shared<PowerMeanComposition>(weights, 2.0);
    case CompositionClass::CommonPointLevelWell:
        return std::make_shared<LevelWellComposition>(weights, 0.40, 0.03);
    case CompositionClass::DeceptivePointSoftmax: {
        std::vector<double> offsets(static_cast<std::size_t>(components), 0.0);
        for (int i = 1; i < components; ++i) {
            offsets[static_cast<std::size_t>(i)] = 10.0 * static_cast<double>(i);
        }
        return std::make_shared<DeceptiveSoftmaxComposition>(centers, offsets, 0.03, 1.0);
    }
    default:
        throw std::invalid_argument("unsupported composition treatment");
    }
}

FuncCraft::ComposedFunction make_single_basic_function(
    FuncCraft::BasicFunctionId id,
    FuncCraft::CoordinateTransformClass t,
    FuncCraft::ValueTransformClass p,
    const FuncCraft::Domain& domain,
    unsigned long long seed,
    const std::string& family) {
    const int dimension = domain.dimension();
    std::mt19937_64 rng(FuncCraft::detail::mix_seed(seed));
    const auto x_star = random_point_in_domain(domain, seed ^ 0xabcddcbaull);

    FuncCraft::FunctionBuilder builder(dimension);
    builder.domain(domain)
        .seed(seed)
        .known_global_minimizer(x_star)
        .known_global_value(100.0)
        .parameter("suite_family", family)
        .parameter("assigned_fmin", "100.0")
        .add_component(id, dimension, make_coordinate_transform(t, dimension, x_star, rng), make_experiment_value_transform(p))
        .composition(std::make_shared<FuncCraft::SingleComponentComposition>());
    return builder.build();
}

std::vector<FuncCraft::BasicFunctionId> first_basic_functions(int count) {
    auto ids = FuncCraft::list_basic_functions();
    count = std::min(count, static_cast<int>(ids.size()));
    ids.resize(static_cast<std::size_t>(count));
    return ids;
}

std::vector<std::vector<FuncCraft::BasicFunctionId>> fixed_composition_base_sets(
    int set_count,
    int components,
    unsigned long long seed) {
    auto all = FuncCraft::list_basic_functions();
    std::vector<std::vector<FuncCraft::BasicFunctionId>> sets;
    sets.reserve(static_cast<std::size_t>(set_count));

    std::mt19937_64 rng(FuncCraft::detail::mix_seed(seed ^ 0x71a2b3c4ull));
    for (int s = 0; s < set_count; ++s) {
        auto shuffled = all;
        FuncCraft::detail::stable_shuffle(shuffled, rng);
        std::vector<FuncCraft::BasicFunctionId> ids;
        ids.reserve(static_cast<std::size_t>(components));
        for (int i = 0; i < components; ++i) {
            ids.push_back(shuffled[static_cast<std::size_t>(i % static_cast<int>(shuffled.size()))]);
        }
        sets.push_back(std::move(ids));
    }
    return sets;
}

std::vector<std::vector<double>> fixed_deceptive_centers(
    int components,
    int dimension,
    const std::vector<double>& x_star,
    unsigned long long seed) {
    std::vector<std::vector<double>> centers(static_cast<std::size_t>(components), zero_point(dimension));
    centers.front() = x_star;
    std::mt19937_64 rng(FuncCraft::detail::mix_seed(seed ^ 0x66778899ull));
    std::uniform_real_distribution<double> dist(-60.0, 60.0);
    for (int i = 2; i < components; ++i) {
        for (double& value : centers[static_cast<std::size_t>(i)]) {
            value = dist(rng);
        }
    }
    return centers;
}

std::vector<FuncCraft::ComposedFunction> make_composition_functions(
    const std::vector<std::vector<FuncCraft::BasicFunctionId>>& base_sets,
    FuncCraft::CompositionClass c,
    const FuncCraft::Domain& domain,
    unsigned long long seed) {
    constexpr int components = 5;
    const int dimension = domain.dimension();
    std::vector<FuncCraft::ComposedFunction> functions;
    functions.reserve(base_sets.size());

    for (std::size_t idx = 0; idx < base_sets.size(); ++idx) {
        const auto& bases = base_sets[idx];
        const auto x_star = random_point_in_domain(domain, seed + 50021ull * static_cast<unsigned long long>(idx + 1));
        const auto centers = fixed_deceptive_centers(components, dimension, x_star, seed + static_cast<unsigned long long>(idx + 1));

        FuncCraft::FunctionBuilder builder(dimension);
        builder.domain(domain)
            .seed(seed + static_cast<unsigned long long>(idx + 1))
            .known_global_minimizer(x_star)
            .known_global_value(100.0)
            .parameter("suite_family", "composition-m5")
            .parameter("assigned_fmin", "100.0")
            .parameter("components", std::to_string(components));

        for (int i = 0; i < components; ++i) {
            std::mt19937_64 rng(FuncCraft::detail::mix_seed(seed + 1009ull * static_cast<unsigned long long>(idx + 1) + static_cast<unsigned long long>(i)));
            // CPM classes use the same minimizer for every component.  Only the
            // DPM class uses separate prescribed component centers, with one
            // deceptive local basin fixed at the origin.
            const auto& component_center = c == FuncCraft::CompositionClass::DeceptivePointSoftmax
                ? centers[static_cast<std::size_t>(i)]
                : x_star;
            builder.add_component(
                bases[static_cast<std::size_t>(i)],
                dimension,
                std::make_shared<FuncCraft::RotationTransform>(FuncCraft::detail::random_rotation_matrix(rng, dimension), component_center),
                std::make_shared<FuncCraft::IdentityValueTransform>());
        }

        builder.composition(make_composition(c, components, centers));
        functions.push_back(builder.build());
    }

    return functions;
}

std::vector<Treatment> build_treatments(const Config& config) {
    const FuncCraft::Domain domain(config.dimension, config.lower_bound, config.upper_bound);
    const auto basic_ids = first_basic_functions(config.basic_count);
    std::vector<Treatment> treatments;

    const std::vector<FuncCraft::CoordinateTransformClass> coordinate_classes = {
        FuncCraft::CoordinateTransformClass::None,
        FuncCraft::CoordinateTransformClass::Rotation,
        FuncCraft::CoordinateTransformClass::Affine,
    };
    for (auto t : coordinate_classes) {
        Treatment treatment;
        treatment.experiment = "coord";
        treatment.label = FuncCraft::to_string(t);
        for (std::size_t i = 0; i < basic_ids.size(); ++i) {
            treatment.functions.push_back(make_single_basic_function(
                basic_ids[i],
                t,
                FuncCraft::ValueTransformClass::None,
                domain,
                config.seed + static_cast<unsigned long long>(i + 1),
                "coordinate"));
        }
        treatments.push_back(std::move(treatment));
    }

    const std::vector<FuncCraft::ValueTransformClass> value_classes = {
        FuncCraft::ValueTransformClass::None,
        FuncCraft::ValueTransformClass::CosineZero,
        FuncCraft::ValueTransformClass::Oscillatory,
        FuncCraft::ValueTransformClass::Power,
    };
    for (auto p : value_classes) {
        Treatment treatment;
        treatment.experiment = "value";
        treatment.label = FuncCraft::to_string(p);
        for (std::size_t i = 0; i < basic_ids.size(); ++i) {
            treatment.functions.push_back(make_single_basic_function(
                basic_ids[i],
                FuncCraft::CoordinateTransformClass::Rotation,
                p,
                domain,
                config.seed + 10000ull + static_cast<unsigned long long>(i + 1),
                "value"));
        }
        treatments.push_back(std::move(treatment));
    }

    const auto base_sets = fixed_composition_base_sets(config.composition_count, 5, config.seed);
    const std::vector<FuncCraft::CompositionClass> composition_classes = {
        FuncCraft::CompositionClass::CommonPointWeightedSum,
        FuncCraft::CompositionClass::CommonPointPowerMean,
        FuncCraft::CompositionClass::CommonPointLevelWell,
        FuncCraft::CompositionClass::DeceptivePointSoftmax,
    };
    for (auto c : composition_classes) {
        Treatment treatment;
        treatment.experiment = "composition";
        treatment.label = FuncCraft::to_string(c);
        treatment.functions = make_composition_functions(base_sets, c, domain, config.seed + 20000ull);
        treatments.push_back(std::move(treatment));
    }

    return treatments;
}

void write_matrix(
    const std::filesystem::path& path,
    const std::vector<std::vector<double>>& values) {
    std::ofstream out(path);
    out << std::scientific << std::setprecision(8);
    for (const auto& row : values) {
        for (std::size_t j = 0; j < row.size(); ++j) {
            if (j != 0) {
                out << ' ';
            }
            out << row[j];
        }
        out << '\n';
    }
}

void print_result_header() {
    constexpr std::size_t class_width = 78;
    std::cout << std::left
              << std::setw(6) << "run"
              << std::setw(6) << "idx"
              << std::setw(static_cast<int>(class_width)) << "class"
              << std::right
              << std::setw(16) << "f_opt"
              << "\n";
    std::cout << std::string(12 + class_width + 16, '-') << "\n";
}

void print_result_line(
    int run,
    int function_index,
    const FuncCraft::FunctionMetadata& metadata,
    double f_opt) {
    constexpr std::size_t class_width = 78;
    std::cout << std::left
              << std::setw(6) << run
              << std::setw(6) << function_index
              << std::setw(static_cast<int>(class_width)) << trim_label(metadata.parameters.at("class_label"), class_width - 1)
              << std::right
              << std::setw(16) << std::scientific << std::setprecision(8)
              << f_opt
              << "\n";
}

void write_manifest(
    const std::filesystem::path& path,
    const Treatment& treatment) {
    std::ofstream out(path);
    out << "column class_label known_global_value bases\n";
    for (std::size_t i = 0; i < treatment.functions.size(); ++i) {
        const auto& metadata = treatment.functions[i].metadata();
        out << i << ' '
            << metadata.parameters.at("class_label") << ' '
            << std::scientific << std::setprecision(16) << metadata.known_global_value << ' ';
        for (std::size_t j = 0; j < metadata.function_class.base_functions.size(); ++j) {
            if (j != 0) {
                out << '+';
            }
            out << FuncCraft::to_string(metadata.function_class.base_functions[j]);
        }
        out << '\n';
    }
}

std::vector<std::vector<double>> run_treatment(
    const Config& config,
    const Treatment& treatment,
    int max_evals) {
    const int function_count = static_cast<int>(treatment.functions.size());
    const int task_count = config.runs * function_count;
    std::vector<std::vector<double>> values(
        static_cast<std::size_t>(config.runs),
        std::vector<double>(static_cast<std::size_t>(function_count), 0.0));

    std::atomic<int> next_task{0};
    std::mutex error_mutex;
    std::mutex print_mutex;
    std::exception_ptr first_error = nullptr;

    const int worker_count = std::min(config.threads, task_count);
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(worker_count));

    for (int worker = 0; worker < worker_count; ++worker) {
        workers.emplace_back([&, worker]() {
            try {
                while (true) {
                    const int task_index = next_task.fetch_add(1);
                    if (task_index >= task_count) {
                        break;
                    }

                    const int run = task_index / function_count;
                    const int function_index = task_index % function_count;
                    const auto& function = treatment.functions[static_cast<std::size_t>(function_index)];
                    const auto bounds = bounds_from_domain(function.domain());
                    const auto guesses = initial_guesses(function);

                    auto settings = minion::DefaultSettings().getDefaultSettings(config.algo);
                    settings["population_size"] = 0;

                    const int run_seed = static_cast<int>(
                        config.seed
                        + 1000003ull * static_cast<unsigned long long>(run + 1)
                        + 9176ull * static_cast<unsigned long long>(function_index + 1));

                    minion::Minimizer minimizer(
                        evaluate_funcraft_batch,
                        bounds,
                        guesses,
                        const_cast<FuncCraft::ComposedFunction*>(&function),
                        nullptr,
                        config.algo,
                        static_cast<std::size_t>(max_evals),
                        run_seed,
                        settings);

                    const minion::MinionResult result = minimizer.optimize();
                    values[static_cast<std::size_t>(run)][static_cast<std::size_t>(function_index)] = result.fun;
                    {
                        std::lock_guard<std::mutex> lock(print_mutex);
                        print_result_line(run, function_index, function.metadata(), result.fun);
                    }
                }
            } catch (...) {
                std::lock_guard<std::mutex> lock(error_mutex);
                if (!first_error) {
                    first_error = std::current_exception();
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
    if (first_error) {
        std::rethrow_exception(first_error);
    }
    return values;
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const Config config = parse_cli(argc, argv);
        std::filesystem::create_directories(config.out_dir);

        const std::vector<int> max_eval_multipliers = {100, 1000, 3000, 10000};
        const auto treatments = build_treatments(config);

        std::cout << "Usage: run_experiments [out_dir] [dimension] [runs] [basic_count] [composition_count] [threads] [seed] [algo]\n";
        std::cout << "Running experiments with algo=" << config.algo
                  << ", dimension=" << config.dimension
                  << ", runs=" << config.runs
                  << ", basic_count=" << config.basic_count
                  << ", composition_count=" << config.composition_count
                  << ", threads=" << config.threads
                  << ", seed=" << config.seed << "\n";

        for (const auto& treatment : treatments) {
            const auto manifest_path = std::filesystem::path(config.out_dir) /
                (config.algo + "_" + std::to_string(config.dimension) + "D_manifest_" +
                 treatment.experiment + "_" + sanitize(treatment.label) + ".txt");
            write_manifest(manifest_path, treatment);

            for (int multiplier : max_eval_multipliers) {
                const int max_evals = multiplier * config.dimension;
                const std::string stem = config.algo + "_" + std::to_string(config.dimension) + "D_" +
                    std::to_string(max_evals) + "_" + treatment.experiment + "_" + sanitize(treatment.label);
                const auto output_path = std::filesystem::path(config.out_dir) / (stem + ".txt");

                std::cout << "Running " << treatment.experiment << ":" << treatment.label
                          << ", maxevals=" << max_evals
                          << ", functions=" << treatment.functions.size() << "\n";
                print_result_header();

                const auto values = run_treatment(config, treatment, max_evals);
                write_matrix(output_path, values);
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
