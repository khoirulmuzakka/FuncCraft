#include "funccraft.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Config {
    std::string out_dir = "results";
    int dimension = 10;
    int max_functions = 32;
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

int parse_int_arg(const char* text, const std::string& name) {
    if (text == nullptr || *text == '\0') {
        throw std::invalid_argument(name + " must be an integer");
    }
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || end == nullptr || *end != '\0') {
        throw std::invalid_argument(name + " must be an integer");
    }
    if (value < INT_MIN || value > INT_MAX) {
        throw std::out_of_range(name + " is outside the int range");
    }
    return static_cast<int>(value);
}

Config parse_cli(int argc, char* argv[]) {
    Config config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        config.out_dir = argv[1];
        arg_index = 2;
    }
    if (argc > arg_index) config.dimension = parse_int_arg(argv[arg_index], "dimension");
    if (argc > arg_index + 1) config.max_functions = parse_int_arg(argv[arg_index + 1], "max_functions");
    if (config.dimension < 1) {
        throw std::invalid_argument("dimension must be at least 1");
    }
    if (config.max_functions < 1) {
        throw std::invalid_argument("max_functions must be at least 1");
    }
    return config;
}

void write_manifest(
    const std::filesystem::path& path,
    const FuncCraft::BenchmarkSuite& suite,
    int function_count) {
    std::ofstream out(path);
    out << "idx label assigned_fopt\n";
    for (int i = 1; i <= function_count; ++i) {
        const auto& function = suite.function(i);
        const auto& spec = function.spec();
        out << i << ' '
            << spec.label << ' '
            << std::scientific << std::setprecision(16) << function.get_fopt()
            << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    try {
        const Config config = parse_cli(argc, argv);
        std::filesystem::create_directories(config.out_dir);

        const int year = 2026;
        const int version = 1;
        const FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(year, version);
        const FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(config.dimension);
        const std::filesystem::path manifest_path =
            std::filesystem::path(config.out_dir) / "suite_manifest.txt";
        const int function_count = std::min(config.max_functions, suite.size());

        write_manifest(manifest_path, suite, function_count);

        std::cout << "Usage: run_experiments [out_dir] [dimension] [max_functions]\n";
        std::cout << "Suite collection: " << collection.name()
                  << " (" << collection.year() << "_v" << collection.version() << ")\n";
        std::cout << "Suite generated. size=" << suite.size()
                  << ", dimension=" << suite.dimension()
                  << ", written_functions=" << function_count << "\n";
        std::cout << "Manifest written to: " << manifest_path.string() << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
