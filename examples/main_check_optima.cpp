#include "suite.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

struct CheckConfig {
    int dimension = 10;
    int max_functions = 500;
    unsigned long long seed = 1;
    double tolerance = 1.0e-8;
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

CheckConfig parse_cli(int argc, char* argv[]) {
    CheckConfig config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        ++arg_index;
    }
    if (argc > arg_index) config.dimension = std::max(1, std::atoi(argv[arg_index]));
    if (argc > arg_index + 1) config.max_functions = std::max(1, std::atoi(argv[arg_index + 1]));
    if (argc > arg_index + 2) config.seed = static_cast<unsigned long long>(std::strtoull(argv[arg_index + 2], nullptr, 10));
    if (argc > arg_index + 3) config.tolerance = std::atof(argv[arg_index + 3]);
    if (argc > arg_index + 4) config.f_opt = std::atof(argv[arg_index + 4]);
    return config;
}

std::filesystem::path resolve_default_suite_yaml(const char* argv0) {
    const std::filesystem::path cwd_candidate = std::filesystem::path("BenchmarkSuites") / "default_suite.yaml";
    if (std::filesystem::exists(cwd_candidate)) {
        return cwd_candidate;
    }

    if (argv0 != nullptr && *argv0 != '\0') {
        const std::filesystem::path exe_path = std::filesystem::absolute(std::filesystem::path(argv0));
        const std::filesystem::path exe_candidate = exe_path.parent_path() / ".." / "BenchmarkSuites" / "default_suite.yaml";
        if (std::filesystem::exists(exe_candidate)) {
            return exe_candidate;
        }
    }

    return cwd_candidate;
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
        const std::filesystem::path suite_yaml = resolve_default_suite_yaml(argc > 0 ? argv[0] : nullptr);
        FuncCraft::SuiteSpec suite_spec = FuncCraft::load_suite_spec_yaml(suite_yaml.string());
        suite_spec.requested_number_of_functions = config.max_functions;
        suite_spec.master_seed = config.seed;
        suite_spec.f_opt = config.f_opt;
        suite_spec.suite_label = "check_optima";
        const FuncCraft::BenchmarkSuite suite(suite_spec, config.dimension);

        int failures = 0;
        double max_abs_error = 0.0;

        std::cout << "FuncCraft optimum check\n";
        std::cout << "Suite yaml: " << suite_yaml.string() << "\n";
        std::cout << "size=" << suite.size()
                  << ", max_number_of_functions=" << suite.max_number_of_functions()
                  << ", dimension=" << suite.dimension()
                  << ", requested_functions=" << config.max_functions
                  << ", seed=" << config.seed
                  << ", tolerance=" << std::scientific << config.tolerance
                  << ", f_opt=" << config.f_opt << "\n\n";

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
            const auto fields = split_class_label(spec.function_class_label);
            const double max_x_star = spec.known_global_minimizer.empty()
                ? 0.0
                : *std::max_element(spec.known_global_minimizer.begin(), spec.known_global_minimizer.end());
            const double value = function(std::vector<std::vector<double>>{spec.known_global_minimizer}).front();
            const double error = std::fabs(value - spec.known_global_value);
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
