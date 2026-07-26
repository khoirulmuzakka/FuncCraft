#include "funccraft.h"

#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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

void require_exact_vector(
    const std::vector<double>& actual,
    const std::vector<double>& expected,
    const std::string& message) {
    require(actual.size() == expected.size(), message + ": size mismatch");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) {
            std::ostringstream out;
            out << message << " at index " << i
                << ": actual=" << std::setprecision(std::numeric_limits<double>::max_digits10) << actual[i]
                << ", expected=" << expected[i];
            throw std::runtime_error(out.str());
        }
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
        function.get_xopt(),
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0),
        pattern,
    };
}

void check_optima() {
    constexpr int suite_year = 2026;
    constexpr int suite_version = 1;
    const FuncCraft::BenchmarkSuite suite = FuncCraft::suite_collection(suite_year, suite_version).benchmark_suite(2);

    for (int i = 0; i < 36; ++i) {
        const FuncCraft::BenchmarkFunction& function = suite.function(i);
        const double value = function({function.get_xopt()}).front();
        require_close(
            value,
            function.get_fopt(),
            20.0,
            "optimum mismatch for function " + std::to_string(i));
    }
}

void check_basic_function_registry_ids() {
    const std::vector<FuncCraft::BasicFunctionId> ids = FuncCraft::list_basic_functions();
    require(!ids.empty(), "basic function registry is empty");
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const int expected = static_cast<int>(i + 1);
        const int actual = static_cast<int>(ids[i]);
        require(
            actual == expected,
            "basic function registry id mismatch at index " + std::to_string(i)
                + ": actual=" + std::to_string(actual)
                + ", expected=" + std::to_string(expected));
    }
    require(ids[0] == FuncCraft::BasicFunctionId::Sphere, "basic function id 1 must be Sphere");
    require(ids[1] == FuncCraft::BasicFunctionId::Ellipsoidal, "basic function id 2 must be Ellipsoidal");
    require(ids[2] == FuncCraft::BasicFunctionId::SumDifferentPowers, "basic function id 3 must be SumDifferentPowers");
}

void check_suite_collection() {
    const std::vector<FuncCraft::SuiteCollectionId> collections = FuncCraft::list_suite_collections();
    require(!collections.empty(), "suite collection registry is empty");

    constexpr int suite_year = 2026;
    constexpr int suite_version = 1;
    const FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(suite_year, suite_version);
    require(collection.year() == suite_year, "suite collection year mismatch");
    require(collection.version() == suite_version, "suite collection version mismatch");
    require(
        FuncCraft::suite_collection_number_of_functions(suite_year, suite_version) == collection.number_of_functions(),
        "suite collection free function count mismatch");

    FuncCraft::SuiteSpec spec = collection.spec();
    require(collection.number_of_functions() == spec.requested_number_of_functions, "suite collection default function count mismatch");
    require(spec.requested_number_of_functions == collection.number_of_functions(), "suite collection spec count mismatch");
    require(spec.suite_label == collection.name(), "suite collection label mismatch");

    const FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(2);
    require(suite.size() == collection.number_of_functions(), "suite collection benchmark suite size mismatch");
    const FuncCraft::BenchmarkFunction& function = suite.function(0);
    require(function.dimension() == 2, "suite collection function dimension mismatch");
}

void check_identity_transform_assigned_optimum() {
    FuncCraft::FunctionSpec spec;
    spec.dimension = 2;
    spec.domain.dimension = 2;
    spec.domain.lower_bound = {-10.0, -10.0};
    spec.domain.upper_bound = {10.0, 10.0};
    spec.assigned_xopt = {1.0, 2.0};
    spec.assigned_fopt = 0.0;
    spec.scale_factor = 1.0;

    FuncCraft::ComponentSpec component;
    component.base_function = FuncCraft::BasicFunctionId::Sphere;
    component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::None;
    component.coordinate_transform.input_dimension = 2;
    component.coordinate_transform.output_dimension = 2;
    component.coordinate_transform.assigned_xopt = spec.assigned_xopt;
    component.value_transform.kind = FuncCraft::ValueTransformKind::None;
    spec.components = {component};
    spec.composition.kind = FuncCraft::CompositionKind::None;

    const FuncCraft::BenchmarkFunction function(spec);
    const double value = function({spec.assigned_xopt}).front();
    require_close(value, spec.assigned_fopt, 1.0e-12, "identity transform assigned optimum mismatch");
}

void check_native_domain_scaled_optimum() {
    FuncCraft::FunctionSpec spec;
    spec.dimension = 2;
    spec.domain.dimension = 2;
    spec.domain.lower_bound = {-10.0, -10.0};
    spec.domain.upper_bound = {10.0, 10.0};
    spec.assigned_xopt = {4.0, -3.0};
    spec.assigned_fopt = 0.0;
    spec.scale_factor = 1.0;

    FuncCraft::ComponentSpec component;
    component.base_function = FuncCraft::BasicFunctionId::HappyCat;
    component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::None;
    component.coordinate_transform.input_dimension = 2;
    component.coordinate_transform.output_dimension = 2;
    component.coordinate_transform.assigned_xopt = spec.assigned_xopt;
    component.value_transform.kind = FuncCraft::ValueTransformKind::None;
    spec.components = {component};
    spec.composition.kind = FuncCraft::CompositionKind::None;

    const FuncCraft::BenchmarkFunction function(spec);
    const double value = function({spec.assigned_xopt}).front();
    require_close(value, spec.assigned_fopt, 1.0e-12, "native-domain scaled optimum mismatch");
}

void check_block_rotation_outputs_selected_subspace() {
    FuncCraft::BlockRotationTransform transform(
        4,
        {0, 3},
        {1.0, 4.0},
        {10.0, 40.0},
        0,
        {{1.0, 0.0}, {0.0, 1.0}});

    std::vector<double> out;
    transform.apply({1.0, 2.0, 3.0, 4.0}, out);
    require_close_vector(out, std::vector<double>{10.0, 40.0}, 0.0, "block rotation selected optimum mismatch");

    transform.apply({2.0, -200.0, 300.0, 6.0}, out);
    require_close_vector(out, std::vector<double>{11.0, 42.0}, 0.0, "block rotation selected subspace output mismatch");
    require(transform.input_dimension() == 4, "block rotation input dimension mismatch");
    require(transform.output_dimension() == 2, "block rotation output dimension mismatch");
}

void check_native_domain_scaled_optimum_high_dimension() {
    const std::vector<int> dimensions = {1, 100};

    for (int dimension : dimensions) {
        std::vector<double> x_star(static_cast<std::size_t>(dimension), 10.0);
        for (FuncCraft::CoordinateTransformKind kind : {
                 FuncCraft::CoordinateTransformKind::None,
                 FuncCraft::CoordinateTransformKind::Rotation,
                 FuncCraft::CoordinateTransformKind::BlockRotation,
             }) {
            FuncCraft::FunctionSpec spec;
            spec.dimension = dimension;
            spec.domain.dimension = dimension;
            spec.domain.lower_bound.assign(static_cast<std::size_t>(dimension), -100.0);
            spec.domain.upper_bound.assign(static_cast<std::size_t>(dimension), 100.0);
            spec.assigned_xopt = x_star;
            spec.assigned_fopt = 100.0;
            spec.scale_factor = 1.0;

            FuncCraft::ComponentSpec component;
            component.base_function = FuncCraft::BasicFunctionId::HappyCat;
            component.coordinate_transform.kind = kind;
            component.coordinate_transform.input_dimension = dimension;
            component.coordinate_transform.output_dimension = dimension;
            component.coordinate_transform.assigned_xopt = spec.assigned_xopt;
            if (kind == FuncCraft::CoordinateTransformKind::BlockRotation) {
                component.coordinate_transform.selected_indices.resize(static_cast<std::size_t>(dimension));
                for (int i = 0; i < dimension; ++i) {
                    component.coordinate_transform.selected_indices[static_cast<std::size_t>(i)] = i;
                }
            }
            component.value_transform.kind = FuncCraft::ValueTransformKind::None;
            spec.components = {component};
            spec.composition.kind = FuncCraft::CompositionKind::None;

            const FuncCraft::BenchmarkFunction function(spec);
            const double value = function({spec.assigned_xopt}).front();
            require_close(value, spec.assigned_fopt, 1.0e-12, "native-domain optimum mismatch");
        }
    }
}

FuncCraft::FunctionSpec make_nested_child_spec() {
    FuncCraft::FunctionSpec child;
    child.dimension = 2;
    child.domain.dimension = 2;
    child.domain.lower_bound = {-10.0, -10.0};
    child.domain.upper_bound = {10.0, 10.0};
    child.assigned_xopt = {2.0, -1.0};
    child.assigned_fopt = 0.0;
    child.scale_factor = 1.0;

    FuncCraft::ComponentSpec component;
    component.base_function = FuncCraft::BasicFunctionId::HappyCat;
    component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::Rotation;
    component.coordinate_transform.input_dimension = 2;
    component.coordinate_transform.output_dimension = 2;
    component.coordinate_transform.assigned_xopt = child.assigned_xopt;
    component.coordinate_transform.seed = 123;
    component.value_transform.kind = FuncCraft::ValueTransformKind::None;
    child.components = {component};
    child.composition.kind = FuncCraft::CompositionKind::None;
    return child;
}

void check_composed_function_component(const std::filesystem::path& path) {
    FuncCraft::FunctionSpec child = make_nested_child_spec();

    FuncCraft::FunctionSpec parent;
    parent.dimension = 2;
    parent.domain.dimension = 2;
    parent.domain.lower_bound = {-100.0, -100.0};
    parent.domain.upper_bound = {100.0, 100.0};
    parent.assigned_xopt = {30.0, -20.0};
    parent.assigned_fopt = 100.0;
    parent.scale_factor = 1.0;

    FuncCraft::ComponentSpec composed_component;
    composed_component.composed_function = std::make_shared<FuncCraft::FunctionSpec>(child);
    composed_component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::Rotation;
    composed_component.coordinate_transform.input_dimension = 2;
    composed_component.coordinate_transform.output_dimension = 2;
    composed_component.coordinate_transform.assigned_xopt = parent.assigned_xopt;
    composed_component.coordinate_transform.seed = 456;
    composed_component.value_transform.kind = FuncCraft::ValueTransformKind::None;
    parent.components = {composed_component};
    parent.composition.kind = FuncCraft::CompositionKind::None;

    const FuncCraft::BenchmarkFunction function(parent);
    const double value = function({parent.assigned_xopt}).front();
    require_close(value, parent.assigned_fopt, 1.0e-12, "composed function component optimum mismatch");
    require(
        function.component_types() == "1 level-1 nested",
        "composed function component type summary mismatch");

    function.export_spec(path.string());
    const FuncCraft::BenchmarkFunction imported = FuncCraft::make_benchmark_function(path.string());
    require_close(
        imported({parent.assigned_xopt}).front(),
        value,
        1.0e-12,
        "composed function component YAML roundtrip mismatch");
}

void check_composed_component_requires_zero_assigned_fopt() {
    FuncCraft::FunctionSpec child = make_nested_child_spec();
    child.assigned_fopt = 10.0;

    FuncCraft::FunctionSpec parent;
    parent.dimension = 2;
    parent.domain.dimension = 2;
    parent.domain.lower_bound = {-100.0, -100.0};
    parent.domain.upper_bound = {100.0, 100.0};
    parent.assigned_xopt = {30.0, -20.0};
    parent.assigned_fopt = 100.0;
    parent.scale_factor = 1.0;

    FuncCraft::ComponentSpec composed_component;
    composed_component.composed_function = std::make_shared<FuncCraft::FunctionSpec>(child);
    composed_component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::Rotation;
    composed_component.coordinate_transform.input_dimension = 2;
    composed_component.coordinate_transform.output_dimension = 2;
    composed_component.coordinate_transform.assigned_xopt = parent.assigned_xopt;
    composed_component.value_transform.kind = FuncCraft::ValueTransformKind::None;
    parent.components = {composed_component};
    parent.composition.kind = FuncCraft::CompositionKind::None;

    bool rejected = false;
    try {
        FuncCraft::BenchmarkFunction function(parent);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "nonzero composed component assigned_fopt was accepted");
}

void check_suite_yaml_accepts_base_function_names(const std::filesystem::path& path) {
    {
        std::ofstream out(path);
        out << "supported_dimensions: 2\n";
        out << "base_functions: [Sphere, Ackley]\n";
        out << "composition_base_functions: [Rastrigin, Rosenbrock]\n";
        out << "requested_number_of_functions: 2\n";
        out << "master_seed: 7\n";
    }

    const FuncCraft::SuiteSpec spec = FuncCraft::load_suite_spec(path.string());
    require(spec.base_functions.size() == 2, "suite YAML base function name count mismatch");
    require(
        spec.base_functions.front() == FuncCraft::BasicFunctionId::Sphere,
        "suite YAML did not parse Sphere base function name");
    require(
        spec.base_functions.back() == FuncCraft::BasicFunctionId::Ackley,
        "suite YAML did not parse Ackley base function name");

    const FuncCraft::BenchmarkSuite suite(spec, 2);
    require(suite.size() == 2, "suite YAML name-based suite size mismatch");
}

FuncCraft::BenchmarkSuite make_roundtrip_suite() {
    FuncCraft::SuiteSpec spec;
    spec.requested_number_of_functions = 100;
    spec.max_components = 4;
    spec.master_seed = 123;
    return FuncCraft::BenchmarkSuite(spec, 3);
}

FuncCraft::BenchmarkSuite make_cross_platform_suite() {
    constexpr int suite_year = 2026;
    constexpr int suite_version = 1;
    return FuncCraft::suite_collection(suite_year, suite_version).benchmark_suite(kCrossPlatformDimension);
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
        component.seed = 11 + i;
        component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::None;
        component.coordinate_transform.input_dimension = 2;
        component.coordinate_transform.output_dimension = 2;
        component.coordinate_transform.assigned_xopt = {static_cast<double>(i), 0.0};
        component.value_transform.kind = FuncCraft::ValueTransformKind::None;
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

void require_same_double_vector(
    const std::vector<double>& lhs,
    const std::vector<double>& rhs,
    const std::string& message) {
    require(lhs.size() == rhs.size(), message + ": size mismatch");
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        require(lhs[i] == rhs[i], message + " at index " + std::to_string(i));
    }
}

void require_same_matrix(
    const std::vector<std::vector<double>>& lhs,
    const std::vector<std::vector<double>>& rhs,
    const std::string& message) {
    require(lhs.size() == rhs.size(), message + ": row count mismatch");
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        require_same_double_vector(lhs[i], rhs[i], message + " row " + std::to_string(i));
    }
}

void require_same_generated_structure(
    const FuncCraft::FunctionSpec& lhs,
    const FuncCraft::FunctionSpec& rhs,
    const std::string& path) {
    require(lhs.seed == rhs.seed, path + ": function seed mismatch");
    require(lhs.components.size() == rhs.components.size(), path + ": component count mismatch");
    require(lhs.composition.kind == rhs.composition.kind, path + ": composition kind mismatch");
    require_same_double_vector(lhs.composition.parameters, rhs.composition.parameters, path + ": composition parameters mismatch");
    require_same_double_vector(lhs.composition.biases, rhs.composition.biases, path + ": composition biases mismatch");

    for (std::size_t i = 0; i < lhs.components.size(); ++i) {
        const FuncCraft::ComponentSpec& left_component = lhs.components[i];
        const FuncCraft::ComponentSpec& right_component = rhs.components[i];
        const std::string component_path = path + ".component[" + std::to_string(i) + "]";
        require(left_component.seed == right_component.seed, component_path + ": component seed mismatch");
        require(
            left_component.base_function.has_value() == right_component.base_function.has_value(),
            component_path + ": base/composed source mismatch");
        if (left_component.base_function.has_value()) {
            require(
                left_component.base_function.value() == right_component.base_function.value(),
                component_path + ": base function mismatch");
        }
        require(
            static_cast<bool>(left_component.composed_function) == static_cast<bool>(right_component.composed_function),
            component_path + ": composed function presence mismatch");
        require(
            left_component.coordinate_transform.kind == right_component.coordinate_transform.kind,
            component_path + ": coordinate transform kind mismatch");
        require(
            left_component.coordinate_transform.seed == right_component.coordinate_transform.seed,
            component_path + ": coordinate transform seed mismatch");
        require_same_double_vector(
            left_component.coordinate_transform.parameters,
            right_component.coordinate_transform.parameters,
            component_path + ": coordinate transform parameters mismatch");
        require(
            left_component.value_transform.kind == right_component.value_transform.kind,
            component_path + ": value transform kind mismatch");
        require_same_double_vector(
            left_component.value_transform.parameters,
            right_component.value_transform.parameters,
            component_path + ": value transform parameters mismatch");
        if (left_component.composed_function) {
            require_same_generated_structure(
                *left_component.composed_function,
                *right_component.composed_function,
                component_path + ".composed_function");
        }
    }
}

void require_prefix_vector(
    const std::vector<double>& prefix,
    const std::vector<double>& full,
    const std::string& message) {
    require(prefix.size() <= full.size(), message + ": prefix vector is longer than full vector");
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        require(prefix[i] == full[i], message + " at index " + std::to_string(i));
    }
}

void require_prefix_int_vector(
    const std::vector<int>& prefix,
    const std::vector<int>& full,
    const std::string& message) {
    require(prefix.size() <= full.size(), message + ": prefix vector is longer than full vector");
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        require(prefix[i] == full[i], message + " at index " + std::to_string(i));
    }
}

void require_prefix_generated_geometry(
    const FuncCraft::FunctionSpec& low_dimension,
    const FuncCraft::FunctionSpec& high_dimension,
    const std::string& path) {
    require_prefix_vector(low_dimension.assigned_xopt, high_dimension.assigned_xopt, path + ": assigned_xopt prefix mismatch");
    require(low_dimension.composition.centers.size() == high_dimension.composition.centers.size(), path + ": composition center count mismatch");
    for (std::size_t i = 0; i < low_dimension.composition.centers.size(); ++i) {
        require_prefix_vector(
            low_dimension.composition.centers[i],
            high_dimension.composition.centers[i],
            path + ": composition center prefix mismatch");
    }
    require(low_dimension.components.size() == high_dimension.components.size(), path + ": component count mismatch");
    for (std::size_t i = 0; i < low_dimension.components.size(); ++i) {
        const FuncCraft::ComponentSpec& low_component = low_dimension.components[i];
        const FuncCraft::ComponentSpec& high_component = high_dimension.components[i];
        const std::string component_path = path + ".component[" + std::to_string(i) + "]";
        require_prefix_vector(
            low_component.coordinate_transform.assigned_xopt,
            high_component.coordinate_transform.assigned_xopt,
            component_path + ": coordinate assigned_xopt prefix mismatch");
        if (!low_component.coordinate_transform.selected_indices.empty()) {
            require_prefix_int_vector(
                low_component.coordinate_transform.selected_indices,
                high_component.coordinate_transform.selected_indices,
                component_path + ": selected index prefix mismatch");
        }
        if (low_component.composed_function) {
            require(
                static_cast<bool>(high_component.composed_function),
                component_path + ": composed function presence mismatch");
            require_prefix_generated_geometry(
                *low_component.composed_function,
                *high_component.composed_function,
                component_path + ".composed_function");
        }
    }
}

void check_suite_structure_stable_across_dimensions() {
    FuncCraft::SuiteSpec suite_spec;
    suite_spec.requested_number_of_functions = 80;
    suite_spec.min_components = 2;
    suite_spec.max_components = 4;
    suite_spec.max_nested_composition_depth = 3;
    suite_spec.nested_probability = 0.75;
    suite_spec.master_seed = 20260723;

    const FuncCraft::BenchmarkSuite suite_2d(suite_spec, 2);
    const FuncCraft::BenchmarkSuite suite_5d(suite_spec, 5);
    require(suite_2d.size() == suite_5d.size(), "dimension-stability suite size mismatch");
    for (int i = 0; i < suite_2d.size(); ++i) {
        require_same_generated_structure(
            suite_2d.function(i).spec(),
            suite_5d.function(i).spec(),
            "function[" + std::to_string(i) + "]");
    }
}

void check_suite_geometry_prefix_stable_across_dimensions() {
    FuncCraft::SuiteSpec suite_spec;
    suite_spec.coordinate_transforms = {FuncCraft::make_choice(FuncCraft::CoordinateTransformKind::BlockRotation, 1.0)};
    suite_spec.value_transforms = {FuncCraft::make_choice(FuncCraft::ValueTransformKind::None, 1.0)};
    suite_spec.compositions = {FuncCraft::make_choice(FuncCraft::CompositionKind::DpmSoftmax, 1.0, 0.01)};
    suite_spec.requested_number_of_functions = 50;
    suite_spec.min_components = 4;
    suite_spec.max_components = 4;
    suite_spec.max_nested_composition_depth = 0;
    suite_spec.nested_probability = 0.0;
    suite_spec.master_seed = 20260724;

    const FuncCraft::BenchmarkSuite suite_4d(suite_spec, 4);
    const FuncCraft::BenchmarkSuite suite_8d(suite_spec, 8);
    require(suite_4d.size() == suite_8d.size(), "geometry-prefix suite size mismatch");
    for (int i = 0; i < suite_4d.size(); ++i) {
        require_prefix_generated_geometry(
            suite_4d.function(i).spec(),
            suite_8d.function(i).spec(),
            "function[" + std::to_string(i) + "]");
    }
}

FuncCraft::FunctionSpec make_seeded_direct_dpm_spec(int dimension) {
    FuncCraft::FunctionSpec spec;
    spec.dimension = dimension;
    spec.domain.dimension = dimension;
    spec.domain.lower_bound.assign(static_cast<std::size_t>(dimension), -100.0);
    spec.domain.upper_bound.assign(static_cast<std::size_t>(dimension), 100.0);
    spec.assigned_fopt = 100.0;
    spec.scale_factor = 1.0;
    spec.seed = 1701;
    spec.composition.kind = FuncCraft::CompositionKind::DpmSoftmax;
    spec.composition.parameters = {0.01};
    spec.composition.biases = {0.0, 20.0, 40.0};

    for (int i = 0; i < 3; ++i) {
        FuncCraft::ComponentSpec component;
        component.base_function = FuncCraft::BasicFunctionId::Sphere;
        component.seed = 3100 + static_cast<std::uint64_t>(i);
        component.coordinate_transform.kind = FuncCraft::CoordinateTransformKind::None;
        component.value_transform.kind = FuncCraft::ValueTransformKind::None;
        spec.components.push_back(component);
    }
    return spec;
}

void check_direct_function_geometry_prefix_stable_across_dimensions() {
    const FuncCraft::BenchmarkFunction function_1d(make_seeded_direct_dpm_spec(1));
    const FuncCraft::BenchmarkFunction function_4d(make_seeded_direct_dpm_spec(4));
    require_close_vector(
        function_1d.spec().components.front().coordinate_transform.assigned_xopt,
        function_1d.spec().assigned_xopt,
        0.0,
        "direct DPM global component center mismatch");
    require_close_vector(
        function_4d.spec().components.front().coordinate_transform.assigned_xopt,
        function_4d.spec().assigned_xopt,
        0.0,
        "direct DPM global component center mismatch");
    require_prefix_generated_geometry(function_1d.spec(), function_4d.spec(), "direct function");
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
    const FuncCraft::BenchmarkFunction imported = FuncCraft::make_benchmark_function(path.string());
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

void check_packaged_suite_function_spec_manifest_exact_roundtrip(const std::filesystem::path& path) {
    constexpr int function_count = 500;
    constexpr int suite_year = 2026;
    constexpr int suite_version = 1;
    FuncCraft::SuiteSpec spec = FuncCraft::suite_collection_spec(suite_year, suite_version);
    spec.requested_number_of_functions = function_count;
    const FuncCraft::BenchmarkSuite suite(spec, kCrossPlatformDimension);
    require(suite.size() == function_count, "packaged suite exact roundtrip function count mismatch");

    std::vector<std::vector<std::vector<double>>> points;
    std::vector<std::vector<double>> before;
    points.reserve(static_cast<std::size_t>(function_count));
    before.reserve(static_cast<std::size_t>(function_count));
    for (int index = 0; index < function_count; ++index) {
        const FuncCraft::BenchmarkFunction& function = suite.function(index);
        points.push_back(sample_cross_platform_points(function.domain(), index));
        before.push_back(function(points.back()));
    }

    suite.export_manifest(path.string());
    const YAML::Node manifest = YAML::LoadFile(path.string());
    require(manifest["functions"] && manifest["functions"].IsSequence(), "suite manifest functions must be a sequence");
    require(
        static_cast<int>(manifest["functions"].size()) == function_count,
        "suite manifest function count mismatch");

    for (int index = 0; index < function_count; ++index) {
        const YAML::Node entry = manifest["functions"][static_cast<std::size_t>(index)];
        require(
            static_cast<bool>(entry["function_spec"]),
            "suite manifest function entry is missing function_spec");

        const std::filesystem::path function_path = path.parent_path()
            / ("manifest_function_" + std::to_string(index) + ".yaml");
        {
            std::ofstream out(function_path);
            if (!out) {
                throw std::runtime_error("failed to open function spec file: " + function_path.string());
            }
            YAML::Node function_doc;
            function_doc["function_spec"] = entry["function_spec"];
            out << function_doc << '\n';
        }

        const FuncCraft::BenchmarkFunction imported = FuncCraft::make_benchmark_function(function_path.string());
        const std::vector<double> after = imported(points[static_cast<std::size_t>(index)]);
        require_exact_vector(
            after,
            before[static_cast<std::size_t>(index)],
            "packaged suite function-spec exact roundtrip mismatch for function " + std::to_string(index));
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

    const int function_count = std::min(kCrossPlatformFunctionCount, suite.size());
    for (int index = 0; index < function_count; ++index) {
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

    struct TestCase {
        std::string name;
        std::function<void()> run;
    };

    const std::vector<TestCase> tests = {
        {"Basic function registry ids", check_basic_function_registry_ids},
        {"Packaged suite optima", check_optima},
        {"Suite collection API", check_suite_collection},
        {"Identity transform assigned optimum", check_identity_transform_assigned_optimum},
        {"Native-domain scaled optimum", check_native_domain_scaled_optimum},
        {"Block rotation subspace output", check_block_rotation_outputs_selected_subspace},
        {"Native-domain optimum in high dimension", check_native_domain_scaled_optimum_high_dimension},
        {"Composed function component", [&] { check_composed_function_component(temp / "composed_function.yaml"); }},
        {"Reject nonzero nested assigned_fopt", check_composed_component_requires_zero_assigned_fopt},
        {"Suite YAML accepts base-function names", [&] { check_suite_yaml_accepts_base_function_names(temp / "suite_names.yaml"); }},
        {"Composition kind aliases", check_composition_kind_aliases},
        {"Suite structure stable across dimensions", check_suite_structure_stable_across_dimensions},
        {"Suite geometry prefix-stable across dimensions", check_suite_geometry_prefix_stable_across_dimensions},
        {"Direct function geometry prefix-stable", check_direct_function_geometry_prefix_stable_across_dimensions},
        {"Function YAML roundtrip", [&] { check_function_yaml_roundtrip(temp / "function.yaml"); }},
        {"Suite YAML roundtrip", [&] { check_suite_yaml_roundtrip(temp / "suite.yaml"); }},
        {"Packaged suite manifest exact function-spec roundtrip", [&] {
            check_packaged_suite_function_spec_manifest_exact_roundtrip(temp / "packaged_suite_manifest.yaml");
        }},
    };

    int passed = 0;
    int failed = 0;

    std::cout << "FuncCraft C++ Test Suite\n";
    std::cout << "========================================\n";
    std::cout << "Temporary files: " << temp.string() << "\n\n";

    for (std::size_t i = 0; i < tests.size(); ++i) {
        const TestCase& test = tests[i];
        std::cout << "[" << std::setw(2) << (i + 1) << "/" << tests.size() << "] RUN  " << test.name << '\n';
        std::ostringstream captured_output;
        std::streambuf* const original_stdout = std::cout.rdbuf(captured_output.rdbuf());
        try {
            test.run();
            std::cout.rdbuf(original_stdout);
            ++passed;
            std::cout << "[" << std::setw(2) << (i + 1) << "/" << tests.size() << "] PASS " << test.name << "\n\n";
        } catch (const std::exception& e) {
            std::cout.rdbuf(original_stdout);
            ++failed;
            std::cout << "[" << std::setw(2) << (i + 1) << "/" << tests.size() << "] FAIL " << test.name << '\n';
            std::cout << "      " << e.what() << "\n\n";
            const std::string captured = captured_output.str();
            if (!captured.empty()) {
                std::cout << "      Captured output:\n";
                std::istringstream lines(captured);
                std::string line;
                while (std::getline(lines, line)) {
                    std::cout << "        " << line << '\n';
                }
                std::cout << '\n';
            }
        }
    }

    std::cout << "----------------------------------------\n";
    std::cout << "Passed: " << passed << " / " << tests.size() << '\n';
    std::cout << "Failed: " << failed << " / " << tests.size() << '\n';
    std::cout << "Overall status: " << (failed == 0 ? "PASS" : "FAIL") << '\n';

    return failed == 0 ? 0 : 1;
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
