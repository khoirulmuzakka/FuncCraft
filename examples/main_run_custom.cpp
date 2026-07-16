#include "benchmark.h"

#include "default_options.h"
#include "minimizer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct RunConfig {
    std::string algo = "ARRDE";
    int dimension = 10;
    int max_evals = 20000;
    int function_count = FuncCraft::BenchmarkSuite::mandatory_function_count() + 12;
    unsigned long long seed = 1;
    double assigned_fmin = 100.0;
};

RunConfig parse_cli(int argc, char* argv[]) {
    RunConfig config;
    if (argc > 1) config.algo = argv[1];
    if (argc > 2) config.dimension = std::max(2, std::atoi(argv[2]));
    if (argc > 3) config.max_evals = std::max(1, std::atoi(argv[3]));
    if (argc > 4) config.function_count = std::max(FuncCraft::BenchmarkSuite::mandatory_function_count(), std::atoi(argv[4]));
    if (argc > 5) config.seed = static_cast<unsigned long long>(std::strtoull(argv[5], nullptr, 10));
    if (argc > 6) config.assigned_fmin = std::atof(argv[6]);
    return config;
}

std::vector<std::vector<double>> initial_guesses(const FuncCraft::ComposedFunction& f) {
    const auto& x_star = f.metadata().known_global_minimizer;
    std::vector<double> center(static_cast<std::size_t>(f.dimension()), 0.0);
    for (int i = 0; i < f.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        center[idx] = 0.5 * (f.domain().lower[idx] + f.domain().upper[idx]);
    }
    return {center};
}

std::vector<std::pair<double, double>> bounds_from_domain(const FuncCraft::Domain& domain) {
    std::vector<std::pair<double, double>> bounds;
    bounds.reserve(domain.lower.size());
    for (std::size_t i = 0; i < domain.lower.size(); ++i) {
        bounds.emplace_back(domain.lower[i], domain.upper[i]);
    }
    return bounds;
}

std::vector<double> evaluate_funcraft_batch(
    const std::vector<std::vector<double>>& xs,
    void* data) {
    const auto* function = static_cast<const FuncCraft::ComposedFunction*>(data);
    if (function == nullptr) {
        throw std::invalid_argument("FuncCraft function pointer must not be null");
    }

    std::vector<double> values;
    values.reserve(xs.size());
    for (const auto& x : xs) {
        values.push_back(function->evaluate(x));
    }
    return values;
}

std::string trim_label(std::string label, std::size_t width) {
    if (label.size() <= width) {
        return label;
    }
    if (width <= 3) {
        return label.substr(0, width);
    }
    return label.substr(0, width - 3) + "...";
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const RunConfig config = parse_cli(argc, argv);

        FuncCraft::BenchmarkSuiteOptions options;
        options.function_count = config.function_count;
        options.dimension = config.dimension;
        options.seed = config.seed;
        options.assigned_fmin = config.assigned_fmin;

        const FuncCraft::BenchmarkSuite suite(options);

        std::cout << "Usage: run_custom [algo] [dimension] [maxevals] [function_count] [seed] [assigned_fmin]\n";
        std::cout << "Running FuncCraft suite with algo=" << config.algo
                  << ", dimension=" << config.dimension
                  << ", maxevals=" << config.max_evals
                  << ", functions=" << suite.size()
                  << ", seed=" << config.seed
                  << ", assigned_fmin=" << std::scientific << config.assigned_fmin << "\n\n";

        std::cout << std::left
                  << std::setw(6) << "idx"
                  << std::setw(15) << "C"
                  << std::setw(10) << "P"
                  << std::setw(8) << "T"
                  << std::setw(42) << "class"
                  << std::right
                  << std::setw(14) << "f(x*)"
                  << std::setw(14) << "best"
                  << std::setw(14) << "gap"
                  << std::setw(9) << "nfev"
                  << std::setw(6) << "ok"
                  << "\n";
        std::cout << std::string(138, '-') << "\n";

        double total_gap = 0.0;
        double best_gap = std::numeric_limits<double>::infinity();
        double worst_gap = -std::numeric_limits<double>::infinity();

        for (int i = 0; i < suite.size(); ++i) {
            const auto& function = suite.function(i);
            const auto& metadata = function.metadata();
            const auto bounds = bounds_from_domain(function.domain());

            auto settings = minion::DefaultSettings().getDefaultSettings(config.algo);
            settings["population_size"] = 0;

            minion::Minimizer minimizer(
                evaluate_funcraft_batch,
                bounds,
                initial_guesses(function),
                const_cast<FuncCraft::ComposedFunction*>(&function),
                nullptr,
                config.algo,
                static_cast<std::size_t>(config.max_evals),
                static_cast<int>(config.seed) + i,
                settings);

            const minion::MinionResult result = minimizer.optimize();
            const double optimum_value = function.evaluate(metadata.known_global_minimizer);
            const double gap = result.fun - metadata.known_global_value;

            total_gap += gap;
            best_gap = std::min(best_gap, gap);
            worst_gap = std::max(worst_gap, gap);

            std::cout << std::left
                      << std::setw(6) << i
                      << std::setw(15) << FuncCraft::to_string(metadata.function_class.composition)
                      << std::setw(10) << FuncCraft::to_string(metadata.function_class.value_transform)
                      << std::setw(8) << FuncCraft::to_string(metadata.function_class.coordinate_transform)
                      << std::setw(42) << trim_label(metadata.parameters.at("class_label"), 41)
                      << std::right
                      << std::setw(14) << std::scientific << std::setprecision(4) << optimum_value
                      << std::setw(14) << std::scientific << std::setprecision(4) << result.fun
                      << std::setw(14) << std::scientific << std::setprecision(4) << gap
                      << std::setw(9) << result.nfev
                      << std::setw(6) << (result.success ? "1" : "0")
                      << "\n";
        }

        const double mean_gap = total_gap / static_cast<double>(suite.size());
        std::cout << std::string(138, '-') << "\n";
        std::cout << "Summary: mean_gap=" << std::scientific << std::setprecision(6) << mean_gap
                  << ", best_gap=" << best_gap
                  << ", worst_gap=" << worst_gap << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_custom failed: " << e.what() << "\n";
        return 1;
    }
}
