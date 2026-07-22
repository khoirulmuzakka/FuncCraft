#include "benchmark.h"

#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kCrossPlatformDimension = 10;
constexpr int kCrossPlatformFunctionCount = 500;
constexpr int kCrossPlatformPointCount = 1000;
constexpr std::uint64_t kCrossPlatformPointSeed = 0x46b8d7f2a9c33115ULL;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_close(double actual, double expected, double tolerance, const std::string& message) {
    if (std::fabs(actual - expected) > tolerance) {
        throw std::runtime_error(
            message + ": actual=" + std::to_string(actual) + ", expected=" + std::to_string(expected));
    }
}

void require_close_vector(
    const std::vector<double>& actual,
    const std::vector<double>& expected,
    double tolerance,
    const std::string& message) {
    require(actual.size() == expected.size(), message + ": size mismatch");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        require_close(actual[i], expected[i], tolerance, message + " at index " + std::to_string(i));
    }
}

double uniform01(std::mt19937_64& rng) {
    return static_cast<double>(rng() >> 11) * 0x1.0p-53;
}

std::vector<std::vector<double>> candidate_points(const FuncCraft::BenchmarkFunction& function) {
    const int dimension = function.dimension();
    std::vector<double> pattern(static_cast<std::size_t>(dimension), 0.0);
    for (int i = 0; i < dimension; ++i) {
        pattern[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.25 : -0.75;
    }
    return {
        function.spec().assigned_xopt,
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0),
        pattern,
    };
}

void check_optima() {
    const std::filesystem::path suite_yaml =
        std::filesystem::path(FUNCCRAFT_SOURCE_DIR) / "BenchmarkSuites" / "default_suite.yaml";
    FuncCraft::SuiteSpec spec = FuncCraft::load_suite_spec_yaml(suite_yaml.string());
    spec.requested_number_of_functions = 36;
    const FuncCraft::BenchmarkSuite suite(spec, 2);

    for (int i = 0; i < suite.size(); ++i) {
        const FuncCraft::BenchmarkFunction& function = suite.function(i);
        const double value = function({function.spec().assigned_xopt}).front();
        require_close(
            value,
            function.spec().assigned_fopt,
            20.0,
            "optimum mismatch for function " + std::to_string(i));
    }
}

FuncCraft::BenchmarkSuite make_roundtrip_suite() {
    FuncCraft::SuiteSpec spec;
    spec.requested_number_of_functions = 100;
    spec.max_components = 4;
    spec.master_seed = 123;
    return FuncCraft::BenchmarkSuite(spec, 3);
}

FuncCraft::BenchmarkSuite make_cross_platform_suite() {
    const std::filesystem::path suite_yaml =
        std::filesystem::path(FUNCCRAFT_SOURCE_DIR) / "BenchmarkSuites" / "default_suite.yaml";
    FuncCraft::SuiteSpec spec = FuncCraft::load_suite_spec_yaml(suite_yaml.string());
    spec.requested_number_of_functions = kCrossPlatformFunctionCount;
    return FuncCraft::BenchmarkSuite(spec, kCrossPlatformDimension);
}

FuncCraft::FunctionSpec make_alias_function_spec(FuncCraft::CompositionKind composition_kind) {
    FuncCraft::FunctionSpec spec;
    spec.dimension = 2;
    spec.domain.dimension = 2;
    spec.domain.lower_bound = {-10.0, -10.0};
    spec.domain.upper_bound = {10.0, 10.0};
    spec.seed = 7;
    spec.assigned_xopt = {0.0, 0.0};
    spec.assigned_fopt = 0.0;

    for (int i = 0; i < 2; ++i) {
        FuncCraft::ComponentSpec component;
        component.base_function = FuncCraft::BasicFunctionId::Sphere;
        component.component_dimension = 2;
        component.seed = 11 + i;
        component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::None;
        component.coordinate_transform.dimension = 2;
        component.coordinate_transform.assigned_xopt = {static_cast<double>(i), 0.0};
        component.coordinate_transform.base_xopt = {0.0, 0.0};
        component.value_transform.kind = FuncCraft::ValueTransformKind::None;
        component.f_bias = 10.0 * static_cast<double>(i);
        spec.components.push_back(component);
    }

    spec.composition.kind = composition_kind;
    spec.composition.parameters = {0.01, 1.0, 0.01};
    return spec;
}

void check_composition_kind_aliases() {
    const std::vector<FuncCraft::CompositionKind> choices = {
        FuncCraft::CompositionKind::DpmSoftmax,
        FuncCraft::CompositionKind::DpmBgSoftmax,
    };

    for (FuncCraft::CompositionKind choice : choices) {
        const FuncCraft::BenchmarkFunction function(make_alias_function_spec(choice));
        require(
            function.spec().composition.kind == choice,
            "composition kind did not roundtrip");
        const std::vector<double> values = function({{0.0, 0.0}, {1.0, 0.0}});
        require(values.size() == 2, "composition evaluation failed");
    }

    FuncCraft::SuiteSpec suite_spec;
    FuncCraft::CompositionChoice composition_choice;
    composition_choice.kind = FuncCraft::CompositionKind::DpmBgSoftmax;
    composition_choice.probability = 1.0;
    composition_choice.parameters = {0.01, 1.0, 0.01};
    suite_spec.compositions = {composition_choice};
    suite_spec.requested_number_of_functions = 40;
    suite_spec.max_components = 3;
    suite_spec.master_seed = 19;

    const FuncCraft::BenchmarkSuite suite(suite_spec, 2);
    bool found_composed_function = false;
    for (int i = 0; i < suite.size(); ++i) {
        const FuncCraft::CompositionKind kind = suite.function(i).spec().composition.kind;
        if (kind == FuncCraft::CompositionKind::DpmBgSoftmax) {
            found_composed_function = true;
            break;
        }
    }
    require(found_composed_function, "suite composition choice did not generate DPM bg softmax functions");
}

std::vector<std::vector<double>> sample_cross_platform_points(const FuncCraft::Domain& domain, int function_index) {
    std::mt19937_64 rng(
        kCrossPlatformPointSeed + static_cast<std::uint64_t>(function_index) * 0x9e3779b97f4a7c15ULL);
    std::vector<std::vector<double>> points(
        static_cast<std::size_t>(kCrossPlatformPointCount),
        std::vector<double>(static_cast<std::size_t>(domain.dimension()), 0.0));

    for (std::vector<double>& point : points) {
        for (int d = 0; d < domain.dimension(); ++d) {
            const auto dim = static_cast<std::size_t>(d);
            point[dim] = domain.lower[dim] + (domain.upper[dim] - domain.lower[dim]) * uniform01(rng);
        }
    }
    return points;
}

void check_function_yaml_roundtrip(const std::filesystem::path& path) {
    const FuncCraft::BenchmarkSuite suite = make_roundtrip_suite();
    const FuncCraft::BenchmarkFunction& function = suite.function(36);
    const std::vector<std::vector<double>> points = candidate_points(function);
    const std::vector<double> before = function(points);

    function.export_spec(path.string());
    const FuncCraft::BenchmarkFunction imported = FuncCraft::make_benchmark_function_from_yaml(path.string());
    require_close_vector(imported(points), before, 1.0e-9, "function YAML roundtrip mismatch");
}

void check_suite_yaml_roundtrip(const std::filesystem::path& path) {
    const FuncCraft::BenchmarkSuite suite = make_roundtrip_suite();
    const std::vector<int> indices = {0, 36, suite.size() - 1};
    std::vector<std::vector<double>> before;
    before.reserve(indices.size());
    for (int index : indices) {
        const FuncCraft::BenchmarkFunction& function = suite.function(index);
        before.push_back(function(candidate_points(function)));
    }

    suite.export_manifest(path.string());
    const FuncCraft::BenchmarkSuite imported(path.string(), suite.dimension());
    for (std::size_t i = 0; i < indices.size(); ++i) {
        const FuncCraft::BenchmarkFunction& function = imported.function(indices[i]);
        require_close_vector(
            function(candidate_points(function)),
            before[i],
            1.0e-9,
            "suite YAML roundtrip mismatch for function " + std::to_string(indices[i]));
    }
}

void write_cross_platform_values(const std::string& platform, const std::filesystem::path& output_path) {
    const FuncCraft::BenchmarkSuite suite = make_cross_platform_suite();
    std::ofstream out(output_path);
    if (!out) {
        throw std::runtime_error("failed to open output file: " + output_path.string());
    }

    out << "# format: funccraft_values_v1\n";
    out << "# platform: " << platform << "\n";
    out << "# dimension: " << kCrossPlatformDimension << "\n";
    out << "# functions: " << kCrossPlatformFunctionCount << "\n";
    out << "# points_per_function: " << kCrossPlatformPointCount << "\n";
    out << "# columns: function_index values...\n";
    out << std::scientific << std::setprecision(std::numeric_limits<double>::max_digits10);

    for (int index = 0; index < suite.size(); ++index) {
        const FuncCraft::BenchmarkFunction& function = suite.function(index);
        const std::vector<double> values = function(sample_cross_platform_points(function.domain(), index));
        out << index;
        for (double value : values) {
            out << ' ' << value;
        }
        out << '\n';
    }
}

int run_tests() {
    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "funccraft_cpp_test";
    std::filesystem::create_directories(temp);

    check_optima();
    check_composition_kind_aliases();
    check_function_yaml_roundtrip(temp / "function.yaml");
    check_suite_yaml_roundtrip(temp / "suite.yaml");

    std::cout << "FuncCraft C++ tests passed\n";
    return 0;
}

int generate_values(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: funccraft_test --generate-values <platform> <output-path>\n";
        return 2;
    }

    write_cross_platform_values(argv[2], argv[3]);
    std::cout << "Wrote FuncCraft value table for " << argv[2] << " to " << argv[3] << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            return run_tests();
        }
        if (std::string(argv[1]) == "--generate-values") {
            return generate_values(argc, argv);
        }
        std::cerr << "usage: funccraft_test [--generate-values <platform> <output-path>]\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "FuncCraft C++ test failed: " << e.what() << '\n';
        return 1;
    }
}
