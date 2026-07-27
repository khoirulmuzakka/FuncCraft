#include "funccraft.h"
#include <minion.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct RunConfig {
    std::string algo = "ARRDE";
    int dimension = 10;
    std::size_t max_evals = 10000;
    int population_size = 0;
    unsigned long long seed = 1;
    int low = 1;
    int high = 32;
};

bool is_integer_arg(const char* text) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    std::strtol(text, &end, 10);
    return errno == 0 && end != nullptr && *end == '\0';
}

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

std::size_t parse_size_arg(const char* text, const std::string& name) {
    if (text == nullptr || *text == '\0' || *text == '-') {
        throw std::invalid_argument(name + " must be a non-negative integer");
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (errno != 0 || end == text || end == nullptr || *end != '\0') {
        throw std::invalid_argument(name + " must be a non-negative integer");
    }
    if (value > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        throw std::out_of_range(name + " is outside the size_t range");
    }
    return static_cast<std::size_t>(value);
}

unsigned long long parse_ull_arg(const char* text, const std::string& name) {
    if (text == nullptr || *text == '\0' || *text == '-') {
        throw std::invalid_argument(name + " must be a non-negative integer");
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (errno != 0 || end == text || end == nullptr || *end != '\0') {
        throw std::invalid_argument(name + " must be a non-negative integer");
    }
    return value;
}

RunConfig parse_cli(int argc, char* argv[]) {
    RunConfig config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        config.algo = argv[arg_index++];
    }
    if (argc > arg_index) config.dimension = parse_int_arg(argv[arg_index], "dimension");
    if (argc > arg_index + 1) config.max_evals = parse_size_arg(argv[arg_index + 1], "maxevals");
    if (argc > arg_index + 2) config.population_size = parse_int_arg(argv[arg_index + 2], "population_size");
    if (argc > arg_index + 3) config.seed = parse_ull_arg(argv[arg_index + 3], "seed");
    if (argc > arg_index + 4) config.low = parse_int_arg(argv[arg_index + 4], "low");
    if (argc > arg_index + 5) config.high = parse_int_arg(argv[arg_index + 5], "high");
    if (config.dimension < 1) {
        throw std::invalid_argument("dimension must be at least 1");
    }
    if (config.max_evals < 1) {
        throw std::invalid_argument("maxevals must be at least 1");
    }
    if (config.population_size < 0) {
        throw std::invalid_argument("population_size must be non-negative");
    }
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
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const RunConfig config = parse_cli(argc, argv);
        const int year = 2026;
        const int version = 1;
        const FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(year, version);
        const FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(config.dimension);
        if (config.low < 1 || config.high < 1) {
            throw std::invalid_argument("function indices are one-based; low and high must be at least 1");
        }
        if (config.high < config.low) {
            throw std::invalid_argument("high must be greater than or equal to low");
        }
        if (config.low > suite.size() || config.high > suite.size()) {
            throw std::out_of_range("requested function index range is outside the suite size");
        }
        const int processed_functions = config.high - config.low + 1;

        std::cout << "Usage: run_minimize [algo] [dimension] [maxevals] [population_size] [seed] [low] [high]\n";
        std::cout << "Suite collection: " << collection.name()
                  << " (" << collection.year() << "_v" << collection.version() << ")\n";
        std::cout << "Suite size: " << suite.size()
                  << ", dimension: " << suite.dimension()
                  << ", index_range: [" << config.low << ", " << config.high << "]"
                  << ", processed_functions: " << processed_functions
                  << ", algo: " << config.algo
                  << ", maxevals: " << config.max_evals
                  << ", population_size: " << config.population_size
                  << ", seed: " << config.seed
                  << "\n\n";

        std::cout << std::left
                  << std::setw(6) << "idx"
                  << std::setw(14) << "C"
                  << std::setw(14) << "P"
                  << std::setw(12) << "T"
                  << std::setw(30) << "G"
                  << std::right
                  << std::setw(16) << "best_f"
                  << std::setw(16) << "nfev"
                  << std::setw(16) << "|best-fopt|"
                  << "\n";
        std::cout << std::string(110, '-') << "\n";

        int failed = 0;
        for (int i = config.low; i <= config.high; ++i) {
            try {
                const auto& function = suite.function(i);
                const auto& spec = function.spec();
                const auto fields = split_class_label(spec.label);
                const auto bounds = function.domain();
                std::vector<std::pair<double, double>> minion_bounds;
                minion_bounds.reserve(static_cast<std::size_t>(bounds.dimension()));
                for (int d = 0; d < bounds.dimension(); ++d) {
                    minion_bounds.emplace_back(bounds.lower[static_cast<std::size_t>(d)], bounds.upper[static_cast<std::size_t>(d)]);
                }
                std::mt19937_64 rng(static_cast<std::uint64_t>(config.seed) ^ (0x9E3779B97F4A7C15ULL + static_cast<std::uint64_t>(i)));
                std::vector<double> x0(static_cast<std::size_t>(bounds.dimension()), 0.0);
                for (int d = 0; d < bounds.dimension(); ++d) {
                    const double lo = bounds.lower[static_cast<std::size_t>(d)];
                    const double hi = bounds.upper[static_cast<std::size_t>(d)];
                    const double t = std::generate_canonical<double, 53>(rng);
                    x0[static_cast<std::size_t>(d)] = lo + (hi - lo) * t;
                }
                auto objective = [&function](const std::vector<std::vector<double>>& X, void*) {
                    return function(X);
                };

                auto settings = minion::DefaultSettings().getDefaultSettings(config.algo);
                settings["convergence_tol"] = 1e-8;
                settings["population_size"] = config.population_size;
                minion::Minimizer optimizer(
                    objective,
                    minion_bounds,
                    x0,
                    nullptr,
                    nullptr,
                    config.algo,
                    config.max_evals,
                    static_cast<int>(config.seed),
                    settings);
                const minion::MinionResult result = optimizer.optimize();
                const double error = std::abs(result.fun - function.get_fopt());
                const bool ok = std::isfinite(result.fun);
                failed += ok ? 0 : 1;

                std::cout << std::left
                          << std::setw(6) << i
                          << std::setw(14) << fields[0]
                          << std::setw(14) << fields[1]
                          << std::setw(12) << fields[2]
                          << std::setw(30) << truncate_with_ellipsis(fields[3], 29)
                          << std::right
                          << std::setw(16) << std::scientific << std::setprecision(6) << result.fun
                          << std::setw(16) << result.nfev
                          << std::setw(16) << std::scientific << std::setprecision(3) << error
                          << "\n";
            } catch (const std::exception& e) {
                const auto& function = suite.function(i);
                std::cerr << "run_minimize failed at index " << i
                          << " label=" << function.spec().label
                          << ": " << e.what() << "\n";
                throw;
            }
        }

        return failed == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "run_minimize failed: " << e.what() << "\n";
        return 1;
    }
}
