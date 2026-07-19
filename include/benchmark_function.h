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
     * @brief Return the complete specification used to build this function.
     *
     * The returned spec is the normalized source record, including the
     * derived class label and any captured descriptive fields.
     */
    const FunctionSpec& spec() const;

private:
    FunctionSpec spec_;
    Domain domain_;
    ComposedFunction function_;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_BENCHMARK_FUNCTION_H
