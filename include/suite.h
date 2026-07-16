#ifndef FUNCCRAFT_SUITE_H
#define FUNCCRAFT_SUITE_H

/**
 * @file suite.h
 * @brief Benchmark-suite generation and access utilities.
 *
 * This header defines the top-level suite container that generates a collection
 * of composed benchmark functions using a deterministic recipe.
 */

#include "composition.h"

namespace FuncCraft {

/**
 * @brief Configuration parameters for benchmark-suite generation.
 */
struct BenchmarkSuiteOptions {
    int function_count = 100;
    int dimension = 10;
    unsigned long long seed = 1;
    double lower_bound = -100.0;
    double upper_bound = 100.0;
    double assigned_fmin = 100.0;
};

/**
 * @brief Deterministic collection of generated benchmark functions.
 */
class BenchmarkSuite final {
public:
    explicit BenchmarkSuite(BenchmarkSuiteOptions options);

    /**
     * @brief Return the number of functions in the suite.
     */
    static int mandatory_function_count();

    /**
     * @brief Return the number of generated functions.
     */
    int size() const;
    /**
     * @brief Access one generated function by index.
     */
    const ComposedFunction& function(int index) const;
    /**
     * @brief Batch-evaluate one generated function at multiple points.
     */
    std::vector<double> operator()(int index, const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the generation options used for this suite.
     */
    const BenchmarkSuiteOptions& options() const;
    /**
     * @brief Return all generated functions.
     */
    const std::vector<ComposedFunction>& functions() const;

private:
    BenchmarkSuiteOptions options_;
    std::vector<ComposedFunction> functions_;
};

/**
 * @brief Convenience helper for building a suite with the default options.
 */
BenchmarkSuite make_benchmark_suite(int function_count, int dimension, unsigned long long seed = 1);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_H
