#ifndef FUNCCRAFT_BENCHMARK_FUNCTION_H
#define FUNCCRAFT_BENCHMARK_FUNCTION_H

/**
 * @file benchmark_function.h
 * @brief High-level wrapper around a built benchmark function.
 *
 * This class owns a plain-data `FunctionSpec` together with the derived
 * runtime callable and domain. It is the user-facing object you should pass
 * around after construction. The stored spec is the normalized, serializable
 * record that can be written back to disk.
 */

#include "builder.h"
#include "function_spec.h"
#include "core.h"

#include <string>

#include <yaml-cpp/yaml.h>

namespace FuncCraft {

/**
 * @brief Concrete benchmark function assembled from components and a composition rule.
 */
class BenchmarkFunction final {
public:
    /**
     * @brief Construct a benchmark function from a plain-data specification.
     *
     * The constructor validates and materializes the runtime pieces from the
     * supplied spec. The spec itself remains available through `spec()`.
     */
    explicit BenchmarkFunction(FunctionSpec spec);
    ~BenchmarkFunction() = default;
    BenchmarkFunction(const BenchmarkFunction&) = default;
    BenchmarkFunction& operator=(const BenchmarkFunction&) = default;
    BenchmarkFunction(BenchmarkFunction&&) noexcept = default;
    BenchmarkFunction& operator=(BenchmarkFunction&&) noexcept = default;

    /**
     * @brief Evaluate a batch of input vectors.
     */
    std::vector<double> operator()(const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the input domain of this benchmark function.
     */
    const Domain& domain() const;
    /**
     * @brief Return the problem dimension.
     */
    int dimension() const;
    /**
     * @brief Return the runtime scale factor used to normalize composed values.
     */
    double lambda() const;
    /**
     * @brief Return the runtime additive bias applied after scaling.
     */
    double bias() const;
    /**
     * @brief Return the complete specification used to build this function.
     *
     * The returned spec is the normalized source record, including the
     * derived class label and any captured descriptive fields.
     */
    const FunctionSpec& spec() const;
    /**
     * @brief Export the complete reproducibility spec as a YAML node.
     */
    YAML::Node export_spec() const;
    /**
     * @brief Export the complete reproducibility spec to a YAML file.
     */
    void export_spec(const std::string& path) const;

private:
    FunctionSpec spec_;
    Domain domain_;
    double lambda_ = 1.0;
    double bias_ = 0.0;
    ComposedFunction function_;
};

/**
 * @brief Load a benchmark-function specification from a YAML file.
 */
FunctionSpec load_function_spec_yaml(const std::string& path);
/**
 * @brief Build a benchmark function directly from a YAML file.
 */
BenchmarkFunction make_benchmark_function_from_yaml(const std::string& path);

} // namespace FuncCraft

#endif // FUNCCRAFT_BENCHMARK_FUNCTION_H
