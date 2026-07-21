#ifndef FUNCCRAFT_SUITE_H
#define FUNCCRAFT_SUITE_H

/**
 * @file suite.h
 * @brief Declarative benchmark-suite specification and container API.
 *
 * A suite is now described by a plain-data `SuiteSpec` plus one fixed ambient
 * dimension. The suite container consumes that spec, resolves its sampling
 * rules deterministically, and stores blueprints only. Concrete
 * `BenchmarkFunction` objects are created lazily when the caller requests a
 * specific function index.
 */

#include "benchmark_function.h"
#include "suite_spec.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace FuncCraft {

/**
 * @brief Deterministic collection of generated benchmark functions.
 */
class BenchmarkSuite final {
public:
    /**
     * @brief Construct a benchmark suite from a declarative suite spec.
     *
     * The constructor does not instantiate benchmark functions. It only
     * normalizes the spec, validates the requested ambient dimension, and
     * prepares the internal blueprints.
     */
    BenchmarkSuite(SuiteSpec spec, int dimension);
    /**
     * @brief Construct a benchmark suite by loading its spec from a YAML file.
     */
    BenchmarkSuite(const std::string& yaml_path, int dimension);

    /**
     * @brief Return the number of generated functions.
     */
    int size() const;
    /**
     * @brief Return the maximum number of lazily generatable functions.
     *
     * This is the number of blueprints prepared by the constructor, so it is
     * the upper bound on valid function indices for this suite.
     */
    int max_number_of_functions() const;
    /**
     * @brief Return the theoretical upper bound implied by the suite spec.
     *
     * This is a combinatorial bound computed from the available base functions,
     * transform families, and composition families.
     */
    std::uint64_t theoretical_max_number_of_functions() const;
    /**
     * @brief Return the fixed ambient dimension of this suite.
     */
    int dimension() const;
    /**
     * @brief Build one generated function lazily for the suite dimension.
     */
    const BenchmarkFunction& function(int index) const;
    /**
     * @brief Batch-evaluate one generated function at multiple points.
     */
    std::vector<double> operator()(int index, const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the normalized suite specification used to build this suite.
     */
    const SuiteSpec& spec() const;
    /**
     * @brief Export the suite spec and every generated function spec as a YAML node.
     */
    YAML::Node export_manifest() const;
    /**
     * @brief Export the suite manifest to a YAML file.
     */
    void export_manifest(const std::string& path) const;
    /**
     * @brief Alias for export_manifest().
     */
    YAML::Node export_spec() const;
    /**
     * @brief Alias for export_manifest(path).
     */
    void export_spec(const std::string& path) const;

private:
    struct FunctionBlueprint {
        bool composed = false;
        BasicFunctionId base_function = BasicFunctionId::Sphere;
        std::vector<BasicFunctionId> component_bases;
        ChoiceSpec base_transform_choice;
        std::vector<ChoiceSpec> coord_transform_choices;
        std::vector<ChoiceSpec> value_transform_choices;
        ChoiceSpec composition_choice;
        int component_count = 1;
        std::uint64_t seed = 0;
    };

    BenchmarkFunction build_function(const FunctionBlueprint& blueprint) const;
    bool supports_dimension(int dimension) const;

    SuiteSpec spec_;
    int dimension_ = 0;
    std::uint64_t theoretical_max_number_of_functions_ = 0;
    std::vector<FunctionBlueprint> blueprints_;
    std::vector<int> supported_dimensions_;
    mutable std::vector<std::optional<BenchmarkFunction>> function_cache_;
};

/**
 * @brief Build a suite directly from a declarative suite spec.
 */
BenchmarkSuite make_benchmark_suite(SuiteSpec spec, int dimension);
/**
 * @brief Load a suite specification from a YAML file.
 */
SuiteSpec load_suite_spec_yaml(const std::string& path);
/**
 * @brief Build a suite directly from a YAML file and an ambient dimension.
 */
BenchmarkSuite make_benchmark_suite_from_yaml(const std::string& path, int dimension);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_H
