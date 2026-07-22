# FuncCraft

FuncCraft is a C++ library for generating continuous optimization benchmark functions from reusable building blocks.

The core idea is a three-stage construction:

1. Apply a coordinate transform to the input.
2. Apply a value transform to a primitive base function.
3. Combine the transformed components with a composition rule.

This makes benchmark generation reproducible, extensible, and suitable for large benchmark suites with known metadata and known global optima.

## Features

- Primitive benchmark functions in `BasicF`
- Coordinate transforms such as rotation, affine maps, subspace selection, and nonlinear folding
- Value transforms such as identity, power, oscillatory, and cosine-zero mappings
- Composition strategies such as weighted sum, power mean, level-well, and deceptive softmax
- `BenchmarkSuite` for generating many benchmark instances from one seed and option set
- Batch evaluation API through `operator()`
- Metadata for class labels, known minimizers, and known global values

## Project Layout

- `include/`
  - Public headers for the benchmark API
- `src/`
  - Implementation files and internal support headers
- `examples/`
  - Example executables
- `docs/`
  - The project paper source, including the benchmark-classification discussion

## Build

### Windows

Run:

```bat
compile.bat
```

This configures CMake, builds the library, and builds the example executables.

### Manual CMake build

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build --config Release
```

`BUILD_EXAMPLES=ON` builds the example targets, including the Minion-dependent minimization examples.

## Quick Start

Include the umbrella header:

```cpp
#include "benchmark.h"
```

Create a suite and evaluate one of its functions in batch mode:

```cpp
#include "benchmark.h"

int main() {
    FuncCraft::BenchmarkSuiteOptions options;
    options.function_count = 100;
    options.dimension = 10;
    options.seed = 1;

    FuncCraft::BenchmarkSuite suite(options);
    const auto& f = suite.function(0);

    std::vector<std::vector<double>> points = {
        std::vector<double>(10, 0.0),
        std::vector<double>(10, 1.0),
    };

    std::vector<double> values = f(points);
    return 0;
}
```

You can also evaluate through the suite:

```cpp
std::vector<double> values = suite(0, points);
```

## Benchmark Classification

FuncCraft classifies each generated benchmark function using:

- composition-function class
- value-transform class
- coordinate-transform class
- base-function set

That classification is stored in the function metadata and rendered as a compact class label. This is useful for organizing large benchmark collections and for reporting results consistently.

## Examples

- `check_basicf_optima`
  - Verifies that generated functions evaluate to the expected known optimum value
- `run_custom`
  - Runs a suite through the Minion optimizer and prints per-function results

## Notes

- The library is built around batch evaluation. Primitive and composed functions expose `operator()` on a vector of input vectors.
- The public API is namespaced under `FuncCraft`.
- The library and Python interface build by default. Examples are optional and can be enabled with `BUILD_EXAMPLES=ON`.
