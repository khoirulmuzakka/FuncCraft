#include "funccraft.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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
    std::strtol(text, &end, 10);
    return end != nullptr && *end == '\0';
}

Config parse_cli(int argc, char* argv[]) {
    Config config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        config.out_dir = argv[1];
        arg_index = 2;
    }
    if (argc > arg_index) config.dimension = std::max(1, std::atoi(argv[arg_index]));
    if (argc > arg_index + 1) config.max_functions = std::max(1, std::atoi(argv[arg_index + 1]));
    return config;
}

void write_manifest(
    const std::filesystem::path& path,
    const FuncCraft::BenchmarkSuite& suite,
    int function_count) {
    std::ofstream out(path);
    out << "idx label assigned_fopt\n";
    for (int i = 0; i < function_count; ++i) {
        const auto function = suite.function(i);
        const auto& spec = function.spec();
        out << i << ' '
            << spec.label << ' '
            << std::scientific << std::setprecision(16) << spec.assigned_fopt
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

        const FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(2026, 1);
        const FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(config.dimension);
        const std::filesystem::path manifest_path =
            std::filesystem::path(config.out_dir) / "suite_manifest.txt";
        const int function_count = std::min(config.max_functions, suite.size());

        write_manifest(manifest_path, suite, function_count);

        std::cout << "Usage: run_experiments [out_dir] [dimension] [max_functions]\n";
        std::cout << "Suite collection: " << collection.name()
                  << " (" << collection.year() << "_v" << collection.version() << ")\n";
        std::cout << "Suite generated. size=" << suite.size()
                  << ", max_number_of_functions=" << suite.max_number_of_functions()
                  << ", dimension=" << suite.dimension()
                  << ", written_functions=" << function_count << "\n";
        std::cout << "Manifest written to: " << manifest_path.string() << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
