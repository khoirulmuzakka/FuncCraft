#include "suite.h"
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

Config parse_cli(int argc, char* argv[]) {
    Config config;
    int arg_index = 1;
    if (argc > 1 && !is_integer_arg(argv[1])) {
        config.out_dir = argv[1];
        arg_index = 2;
    }
    if (argc > arg_index) config.dimension = std::max(1, std::atoi(argv[arg_index]));
    if (argc > arg_index + 1) config.max_functions = std::max(1, std::atoi(argv[arg_index + 1]));
    if (argc > arg_index + 2) config.seed = static_cast<unsigned long long>(std::strtoull(argv[arg_index + 2], nullptr, 10));
    if (argc > arg_index + 3) config.f_opt = std::atof(argv[arg_index + 3]);
    return config;
}

FuncCraft::SuiteSpec make_spec(const Config& config) {
    FuncCraft::SuiteSpec spec;
    spec.supported_dimensions = std::to_string(config.dimension);
    spec.requested_number_of_functions = config.max_functions;
    spec.master_seed = config.seed;
    spec.f_opt = config.f_opt;
    spec.suite_label = "run_experiments";
    return spec;
}

void write_manifest(
    const std::filesystem::path& path,
    const FuncCraft::BenchmarkSuite& suite,
    int function_count) {
    std::ofstream out(path);
    out << "idx label known_global_value\n";
    for (int i = 0; i < function_count; ++i) {
        const auto function = suite.function(i);
        const auto& spec = function.spec();
        out << i << ' '
            << spec.function_class_label << ' '
            << std::scientific << std::setprecision(16) << spec.known_global_value
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

        const FuncCraft::BenchmarkSuite suite(make_spec(config), config.dimension);
        const std::filesystem::path manifest_path =
            std::filesystem::path(config.out_dir) / "suite_manifest.txt";
        const int function_count = std::min(config.max_functions, suite.size());

        write_manifest(manifest_path, suite, function_count);

        std::cout << "Usage: run_experiments [out_dir] [dimension] [max_functions] [seed] [f_opt]\n";
        std::cout << "Suite generated. size=" << suite.size()
                  << ", max_number_of_functions=" << suite.max_number_of_functions()
                  << ", dimension=" << suite.dimension()
                  << ", requested_functions=" << config.max_functions
                  << ", seed=" << config.seed
                  << ", f_opt=" << std::scientific << config.f_opt << "\n";
        std::cout << "Manifest written to: " << manifest_path.string() << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "run_experiments failed: " << e.what() << "\n";
        return 1;
    }
}
