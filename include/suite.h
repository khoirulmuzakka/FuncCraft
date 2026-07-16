#ifndef FUNCCRAFT_SUITE_H
#define FUNCCRAFT_SUITE_H

#include "composition.h"

namespace FuncCraft {

struct BenchmarkSuiteOptions {
    int function_count = 100;
    int dimension = 10;
    unsigned long long seed = 1;
    double lower_bound = -100.0;
    double upper_bound = 100.0;
    double assigned_fmin = 100.0;
};

class BenchmarkSuite final {
public:
    explicit BenchmarkSuite(BenchmarkSuiteOptions options);

    static int mandatory_function_count();

    int size() const;
    const ComposedFunction& function(int index) const;
    double evaluate(int index, const std::vector<double>& x) const;
    const BenchmarkSuiteOptions& options() const;
    const std::vector<ComposedFunction>& functions() const;

private:
    BenchmarkSuiteOptions options_;
    std::vector<ComposedFunction> functions_;
};

BenchmarkSuite make_benchmark_suite(int function_count, int dimension, unsigned long long seed = 1);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_H
