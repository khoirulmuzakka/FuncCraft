#include "benchmark.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

struct CheckConfig {
    int function_count = 500;
    int dimension = 10;
    unsigned long long seed = 1;
    double assigned_fmin = 100.0;
    double tolerance = 1.0e-8;
};

CheckConfig parse_cli(int argc, char* argv[]) {
    CheckConfig config;
    if (argc > 1) config.function_count = std::max(FuncCraft::BenchmarkSuite::mandatory_function_count(), std::atoi(argv[1]));
    if (argc > 2) config.dimension = std::max(2, std::atoi(argv[2]));
    if (argc > 3) config.seed = static_cast<unsigned long long>(std::strtoull(argv[3], nullptr, 10));
    if (argc > 4) config.assigned_fmin = std::atof(argv[4]);
    if (argc > 5) config.tolerance = std::atof(argv[5]);
    return config;
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
    try {
        const CheckConfig config = parse_cli(argc, argv);

        FuncCraft::BenchmarkSuiteOptions options;
        options.function_count = config.function_count;
        options.dimension = config.dimension;
        options.seed = config.seed;
        options.assigned_fmin = config.assigned_fmin;

        const FuncCraft::BenchmarkSuite suite(options);

        int failures = 0;
        double max_abs_error = 0.0;

        std::cout << "FuncCraft optimum check\n";
        std::cout << "functions=" << suite.size()
                  << ", dimension=" << options.dimension
                  << ", seed=" << options.seed
                  << ", assigned_fmin=" << std::scientific << options.assigned_fmin
                  << ", tolerance=" << std::scientific << config.tolerance << "\n\n";

        std::cout << std::left
                  << std::setw(6) << "idx"
                  << std::setw(15) << "C"
                  << std::setw(10) << "P"
                  << std::setw(8) << "T"
                  << std::setw(42) << "G"
                  << std::right
                  << std::setw(16) << "f(x*)"
                  << std::setw(14) << "|err|"
                  << std::setw(8) << "status"
                  << "\n";
        std::cout << std::string(119, '-') << "\n";

        for (int i = 0; i < suite.size(); ++i) {
            const auto& function = suite.function(i);
            const auto& metadata = function.metadata();
            const double value = function.evaluate(metadata.known_global_minimizer);
            const double error = std::fabs(value - metadata.known_global_value);
            const bool ok = std::isfinite(value) && error <= config.tolerance;

            max_abs_error = std::max(max_abs_error, error);
            failures += ok ? 0 : 1;

            std::cout << std::left
                      << std::setw(6) << i
                      << std::setw(15) << FuncCraft::to_string(metadata.function_class.composition)
                      << std::setw(10) << FuncCraft::to_string(metadata.function_class.value_transform)
                      << std::setw(8) << FuncCraft::to_string(metadata.function_class.coordinate_transform)
                      << std::setw(42) << trim_label(metadata.parameters.at("class_label"), 41)
                      << std::right
                      << std::setw(16) << std::scientific << std::setprecision(6) << value
                      << std::setw(14) << std::scientific << std::setprecision(3) << error
                      << std::setw(8) << (ok ? "OK" : "FAIL")
                      << "\n";
        }

        std::cout << std::string(119, '-') << "\n";
        std::cout << "Summary: checked=" << suite.size()
                  << ", passed=" << (suite.size() - failures)
                  << ", failed=" << failures
                  << ", max_abs_error=" << std::scientific << std::setprecision(6) << max_abs_error
                  << "\n";

        return failures == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "check_basicf_optima failed: " << e.what() << "\n";
        return 1;
    }
}
