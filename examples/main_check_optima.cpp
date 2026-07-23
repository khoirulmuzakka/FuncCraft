#include "funccraft.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

struct CheckConfig {
    int dimension = 10;
    int max_functions = 500;
    double tolerance = 1.0e-8;
};

bool is_integer_arg(const char* text) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    std::strtol(text, &end, 10);
    return end != nullptr && *end == '\0';
}

CheckConfig parse_cli(int argc, char* argv[]) {
    CheckConfig config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        ++arg_index;
    }
    if (argc > arg_index) config.dimension = std::max(1, std::atoi(argv[arg_index]));
    if (argc > arg_index + 1) config.max_functions = std::max(1, std::atoi(argv[arg_index + 1]));
    if (argc > arg_index + 2) config.tolerance = std::atof(argv[arg_index + 2]);
    return config;
}

std::array<std::string, 4> split_class_label(const std::string& label) {
    std::array<std::string, 4> fields = {"-", "-", "-", "-"};
    std::size_t start = 0;
    while (start < label.size()) {
        const std::size_t end = label.find(';', start);
        const std::string token = label.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const std::size_t eq = token.find('=');
        if (eq != std::string::npos) {
            const std::string key = token.substr(0, eq);
            const std::string value = token.substr(eq + 1);
            if (key == "C") fields[0] = value;
            if (key == "P") fields[1] = value;
            if (key == "T") fields[2] = value;
            if (key == "G") fields[3] = value;
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return fields;
}

std::string truncate_with_ellipsis(const std::string& text, std::size_t max_len) {
    if (text.size() <= max_len) {
        return text;
    }
    if (max_len <= 3) {
        return text.substr(0, max_len);
    }
    return text.substr(0, max_len - 3) + "...";
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const CheckConfig config = parse_cli(argc, argv);
        const FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(2026, 1);
        const FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(config.dimension);

        int failures = 0;
        double max_abs_error = 0.0;

        std::cout << "FuncCraft optimum check\n";
        std::cout << "Suite collection: " << collection.name()
                  << " (" << collection.year() << "_v" << collection.version() << ")\n";
        std::cout << "size=" << suite.size()
                  << ", max_number_of_functions=" << suite.max_number_of_functions()
                  << ", dimension=" << suite.dimension()
                  << ", checked_functions=" << std::min(config.max_functions, suite.size())
                  << ", tolerance=" << std::scientific << config.tolerance
                  << "\n\n";

        std::cout << std::left
                  << std::setw(6) << "idx"
                  << std::setw(14) << "C"
                  << std::setw(14) << "P"
                  << std::setw(12) << "T"
                  << std::setw(30) << "G"
                  << std::right
                  << std::setw(16) << "max(x_i*)"
                  << std::setw(16) << "f(x*)"
                  << std::setw(14) << "|err|"
                  << std::setw(8) << "status"
                  << "\n";
        std::cout << std::string(110, '-') << "\n";

        const int function_count = std::min(config.max_functions, suite.size());
        for (int i = 0; i < function_count; ++i) {
            const auto function = suite.function(i);
            const auto& spec = function.spec();
            const auto fields = split_class_label(spec.label);
            const double max_x_star = spec.assigned_xopt.empty()
                ? 0.0
                : *std::max_element(spec.assigned_xopt.begin(), spec.assigned_xopt.end());
            const double value = function(std::vector<std::vector<double>>{spec.assigned_xopt}).front();
            const double error = std::fabs(value - spec.assigned_fopt);
            const bool ok = std::isfinite(value) && error <= config.tolerance;

            max_abs_error = std::max(max_abs_error, error);
            failures += ok ? 0 : 1;

            std::cout << std::left
                      << std::setw(6) << i
                      << std::setw(14) << fields[0]
                      << std::setw(14) << fields[1]
                      << std::setw(12) << fields[2]
                      << std::setw(30) << truncate_with_ellipsis(fields[3], 29)
                      << std::right
                      << std::setw(16) << std::scientific << std::setprecision(6) << max_x_star
                      << std::setw(16) << std::scientific << std::setprecision(6) << value
                      << std::setw(14) << std::scientific << std::setprecision(3) << error
                      << std::setw(8) << (ok ? "OK" : "FAIL")
                      << "\n";
        }

        std::cout << std::string(110, '-') << "\n";
        std::cout << "Summary: checked=" << function_count
                  << ", passed=" << (function_count - failures)
                  << ", failed=" << failures
                  << ", max_abs_error=" << std::scientific << std::setprecision(6) << max_abs_error
                  << "\n";

        return failures == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "check_optima failed: " << e.what() << "\n";
        return 1;
    }
}
