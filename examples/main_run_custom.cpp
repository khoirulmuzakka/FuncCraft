#include "suite.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

struct RunConfig {
    int dimension = 10;
    int max_functions = 32;
    unsigned long long seed = 1;
    double f_opt = 100.0;
};

bool is_integer_arg(const char* text) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    std::strtol(text, &end, 10);
    return end != nullptr && *end == '\0';
}

RunConfig parse_cli(int argc, char* argv[]) {
    RunConfig config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        ++arg_index; // legacy leading algo argument is accepted and ignored
    }
    if (argc > arg_index) config.dimension = std::max(1, std::atoi(argv[arg_index]));
    if (argc > arg_index + 1) config.max_functions = std::max(1, std::atoi(argv[arg_index + 1]));
    if (argc > arg_index + 2) config.seed = static_cast<unsigned long long>(std::strtoull(argv[arg_index + 2], nullptr, 10));
    if (argc > arg_index + 3) config.f_opt = std::atof(argv[arg_index + 3]);
    return config;
}

FuncCraft::SuiteSpec make_spec(const RunConfig& config) {
    FuncCraft::SuiteSpec spec;
    spec.supported_dimensions = std::to_string(config.dimension);
    spec.max_dimension = config.dimension;
    spec.requested_number_of_functions = config.max_functions;
    spec.master_seed = config.seed;
    spec.f_opt = config.f_opt;
    spec.suite_label = "run_custom";
    return spec;
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const RunConfig config = parse_cli(argc, argv);
        const FuncCraft::BenchmarkSuite suite(make_spec(config), config.dimension);

        std::cout << "Usage: run_custom [dimension] [max_functions] [seed] [f_opt]\n";
        std::cout << "Suite size: " << suite.size()
                  << ", max_number_of_functions: " << suite.max_number_of_functions()
                  << ", dimension: " << suite.dimension()
                  << ", requested_functions: " << config.max_functions
                  << ", seed: " << config.seed
                  << ", f_opt: " << std::scientific << config.f_opt << "\n\n";

        std::cout << std::left
                  << std::setw(6) << "idx"
                  << std::setw(16) << "label"
                  << std::right
                  << std::setw(16) << "f(x*)"
                  << std::setw(16) << "|f(x*)-f_opt|"
                  << "\n";
        std::cout << std::string(54, '-') << "\n";

        const int function_count = std::min(config.max_functions, suite.size());
        for (int i = 0; i < function_count; ++i) {
            const auto function = suite.function(i);
            const auto& spec = function.spec();
            const double value = function(std::vector<std::vector<double>>{spec.known_global_minimizer}).front();
            const double error = std::abs(value - spec.known_global_value);

            std::cout << std::left
                      << std::setw(6) << i
                      << std::setw(16) << spec.function_class_label
                      << std::right
                      << std::setw(16) << std::scientific << std::setprecision(6) << value
                      << std::setw(16) << std::scientific << std::setprecision(3) << error
                      << "\n";
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_custom failed: " << e.what() << "\n";
        return 1;
    }
}
