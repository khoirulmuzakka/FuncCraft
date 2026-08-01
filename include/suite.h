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
 * specific one-based function index.
 */

#include "benchmark_function.h"
#include "suite_spec.h"

#include <cstdint>
#include <memory>
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
     * @brief Construct a benchmark suite by loading its spec from a file.
     */
    BenchmarkSuite(const std::string& spec_path, int dimension);
    BenchmarkSuite(const BenchmarkSuite& other);
    BenchmarkSuite& operator=(const BenchmarkSuite& other);
    BenchmarkSuite(BenchmarkSuite&&) noexcept = default;
    BenchmarkSuite& operator=(BenchmarkSuite&&) noexcept = default;
    ~BenchmarkSuite() = default;

    /**
     * @brief Apply suite value-transform choices to mandatory base functions.
     *
     * Defaults to false, preserving the normal suite convention that F1-F34 are
     * primitive base-function entries with no value transform. Set this to true
     * before calling function(), operator(), export_manifest(), or export_spec()
     * when a controlled experiment needs F1-F34 to use value_transforms.
     */
    bool apply_value_transforms_to_basic = false;

    /**
     * @brief Return the number of generated functions.
     *
     * This is the number of blueprints prepared by the constructor, so it is
     * the upper bound on valid one-based function indices for this suite.
     */
    int size() const;
    /**
     * @brief Return the top-level combinatorial capacity implied by the suite spec.
     *
     * This is a combinatorial bound computed from the available base functions,
     * transform families, and composition families. Nested composed components
     * are not recursively expanded in this count, so when nesting is enabled it
     * is a lower bound on the full recursive combinatorial capacity.
     */
    std::uint64_t theoretical_max_number_of_functions() const;
    /**
     * @brief Return the fixed ambient dimension of this suite.
     */
    int dimension() const;
    /**
     * @brief Build one generated function lazily for the suite dimension.
     *
     * Function indices are one-based: valid indices are 1 through size().
     */
    const BenchmarkFunction& function(int index) const;
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
        CoordinateTransformChoice coordinate_transform_choice;
        std::uint64_t seed = 0;
    };

    BenchmarkFunction build_function(const FunctionBlueprint& blueprint) const;
    YAML::Node export_function_spec(int index) const;
    bool supports_dimension(int dimension) const;

    SuiteSpec spec_;
    int dimension_ = 0;
    std::uint64_t theoretical_max_number_of_functions_ = 0;
    std::vector<FunctionBlueprint> blueprints_;
    std::vector<int> supported_dimensions_;
    mutable std::vector<std::unique_ptr<BenchmarkFunction>> function_cache_;
};

/**
 * @brief Build a suite directly from a declarative suite spec.
 */
BenchmarkSuite make_benchmark_suite(SuiteSpec spec, int dimension);
/**
 * @brief Load a suite specification from a file.
 */
SuiteSpec load_suite_spec(const std::string& path);
/**
 * @brief Build a suite directly from a specification file and an ambient dimension.
 */
BenchmarkSuite make_benchmark_suite(const std::string& path, int dimension);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_H
