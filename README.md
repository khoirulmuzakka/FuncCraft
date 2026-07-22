# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)

FuncCraft is a C++17 library, with a Python interface, for scalable generation
of reproducible continuous optimization benchmark functions. From one suite
specification it can generate hundreds, thousands, or practically unbounded
numbers of distinct benchmark instances across dimensions while preserving the
metadata needed to reproduce each function exactly and still controlling the
location of the constructed function's known minimum.

The library separates benchmark construction into primitive functions,
coordinate transforms, value transforms, and composition rules. A generated
function can be exported as YAML and rebuilt later with the same parameters,
including transform matrices, selected subspaces, centers, offsets, scale, and
bias.

## Generating Mechanism

FuncCraft follows the composition framework described in
`docs/funccraft.tex`, section `A General Composition Framework`. A benchmark is
assembled as

```text
f(x) = psi(x, phi_1(g_1(T_1(x))), ..., phi_m(g_m(T_m(x))))
```

where:

- `g_i` is a primitive benchmark function.
- `T_i` is a coordinate transform that changes where and how the component is
  sampled in the search space.
- `phi_i` is a scalar value transform applied to the component output.
- `psi` is the composition function that combines all transformed component
  values.

For the implemented linear coordinate transforms, the convention is:

```text
T(x) = target_point + M * (x - source_point)
```

Here `source_point` is the desired minimum location in the generated/search
coordinates. `target_point` is the corresponding minimum location in the
primitive coordinates, which must be the base function's `x_opt` by
construction. This lets the suite place a generated optimum at `source_point`
while evaluating the primitive at its own optimum.

The usual suite generator chooses these ingredients from weighted
`ChoiceSpec` lists, places the global optimum and deceptive centers
deterministically from the suite seed, and then materializes a
`FunctionSpec`. A `FunctionSpec` is the reproducibility record: it is the data
used to build a `BenchmarkFunction`, and it is also what is exported to YAML.

## Implemented Mechanisms

Primitive functions are exposed through `BasicFunctionId`. The current enum
contains 36 base landscapes, including Sphere, Ellipsoidal, Rosenbrock, Ackley,
Rastrigin, Griewank, Schwefel, Schaffer variants, Gallagher21, Katsuura, Levy,
Michalewicz, BentCigar, Discus, HappyCat, HGBat, Step, Quartic, Exponential,
and StyblinskiTang.

Coordinate transforms:

- `identity` / `none`: direct use of the input coordinates.
- `rotation` / `rot`: dense orthogonal rotation with a reproducible matrix.
- `affine` / `aff`: reproducible affine linear transform.
- `block_rotation` / `blockrotation` / `blockrot` / `brot`: rotation on a
  selected coordinate subspace.

Value transforms:

- `identity` / `none`: no scalar reshaping.
- `power`: power-law reshaping with scale.
- `oscillatory` / `osc`: oscillatory nonlinearity for positive component
  values.
- `cosine_zero` / `coszero`: nonmonotone transform preserving zero.

Composition functions:

- `single_component` / `single`: one transformed primitive component.
- `weighted_sum` / `sum` / `cpm`: common-point weighted sum.
- `power_mean` / `powermean`: common-point power-mean aggregation.
- `level_well` / `levelwell` / `lvlwell`: common-point level-well
  composition.
- `deceptive_softmax` / `dpm`: deceptive multi-point softmax composition.
- `deceptive_bg_softmax` / `dpmbgsoftmax`: deceptive softmax with a background
  component.

The default suite choices are in `include/suite_spec.h` and
`BenchmarkSuites/default_suite.yaml`.

## Repository Layout

- `include/`: public C++ headers.
- `src/`: C++ implementation and Python bindings.
- `funccraft/`: Python wrapper package.
- `BenchmarkSuites/`: YAML suite specifications.
- `tests/`: Python and C++ round-trip and cross-platform value checks.
- `examples/`: optional examples; Minion-dependent examples are built only when
  `BUILD_EXAMPLES=ON`. See `examples/main_minimize.cpp` for an end-to-end
  example that minimizes functions from a generated benchmark suite.
- `docs/`: paper source and plotting scripts.

## Build Options

| Option | Default | Purpose |
| --- | --- | --- |
| `BUILD_LIBRARY` | `ON` | Build the C++ `funccraft` static library. |
| `BUILD_PYTHON` | `ON` | Build the Python extension module. |
| `BUILD_EXAMPLES` | `OFF` | Build examples; this enables the Minion FetchContent dependency. |
| `BUILD_TEST` | `OFF` | Build C++ tests; this does not require Minion. |
| `FUNCCRAFT_INSTALL` | `OFF` | Generate install/export metadata. |

## Installation

Prerequisites:

- CMake 3.18 or newer.
- A C++17 compiler.
- Python 3.9 or newer for the Python package.

Build the C++ library, Python extension, and C++ tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_LIBRARY=ON \
  -DBUILD_PYTHON=ON \
  -DBUILD_TEST=ON \
  -DBUILD_EXAMPLES=OFF

cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
python tests/test.py
```

Build optional examples:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build --config Release
```

Build a Python wheel:

```bash
python -m pip install --upgrade build
python -m build --wheel
python -m pip install dist/funccraft-*.whl
```

On Windows, `compile.bat` is a convenience script for configuring and building
the project.

## C++ Usage

All benchmark evaluations are batched. A function receives a
`std::vector<std::vector<double>>`, where each inner vector is one point, and
returns one value per point.

Create one composed benchmark function from a plain `FunctionSpec`:

```cpp
#include "benchmark.h"

#include <random>
#include <vector>

int main() {
    using namespace FuncCraft;

    BasicF sphere_base(BasicFunctionId::Sphere, 2);
    BasicF rastrigin_base(BasicFunctionId::Rastrigin, 2);

    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> uniform(-4.0, 4.0);
    std::vector<double> x_star = {uniform(rng), uniform(rng)};

    TransformSpec identity;
    identity.kind = "identity";
    identity.dimension = 2;
    identity.source_point = x_star;
    identity.target_point = sphere_base.x_opt;

    TransformSpec rotation;
    rotation.kind = "rotation";
    rotation.dimension = 2;
    rotation.seed = 17;
    rotation.source_point = x_star;
    rotation.target_point = rastrigin_base.x_opt;

    ValueTransformSpec no_value_transform;
    no_value_transform.kind = "identity";

    ValueTransformSpec power;
    power.kind = "power";
    power.parameters = {1.25, 1.0};

    ComponentSpec sphere;
    sphere.base_function = "Sphere";
    sphere.component_dimension = 2;
    sphere.coordinate_transform = identity;
    sphere.value_transform = no_value_transform;

    ComponentSpec rastrigin;
    rastrigin.base_function = "Rastrigin";
    rastrigin.component_dimension = 2;
    rastrigin.coordinate_transform = rotation;
    rastrigin.value_transform = power;

    CompositionSpec composition;
    composition.kind = "weighted_sum";
    composition.weights = {0.5, 0.5};

    FunctionSpec spec;
    spec.dimension = 2;
    spec.lower_bound = {-5.0, -5.0};
    spec.upper_bound = {5.0, 5.0};
    spec.component_specs = {sphere, rastrigin};
    spec.composition_spec = composition;
    spec.known_global_minimizer = x_star;
    spec.known_global_value = 0.0;

    BenchmarkFunction f(spec);
    std::vector<double> values = f({x_star, {1.0, 1.0}});

    f.export_spec("function.yaml");
    BenchmarkFunction same_f = make_benchmark_function_from_yaml("function.yaml");
}
```

Create a benchmark suite and evaluate generated functions:

```cpp
#include "benchmark.h"

#include <vector>

int main() {
    using namespace FuncCraft;

    SuiteSpec spec = load_suite_spec_yaml("BenchmarkSuites/default_suite.yaml");
    spec.requested_number_of_functions = 500;
    spec.master_seed = 1;

    BenchmarkSuite suite(spec, 10);
    std::vector<std::vector<double>> points = {
        std::vector<double>(10, 0.0),
        std::vector<double>(10, 1.0),
    };

    for (int i = 0; i < suite.size(); ++i) {
        const BenchmarkFunction& f = suite.function(i);
        std::vector<double> values = f(points);

        const FunctionSpec& f_spec = f.spec();
        const std::vector<double>& x_star = f_spec.known_global_minimizer;
        const double f_star = f_spec.known_global_value;
    }

    suite.export_manifest("suite_manifest.yaml");
}
```

`BenchmarkSuite::export_manifest()` is the preferred suite-level export name.
It writes the normalized suite spec and every generated function spec, which is
useful for archiving the exact benchmark set used in an experiment.
`export_spec()` is available as an alias.

For a complete optimization run, see `examples/main_minimize.cpp`, which builds
a benchmark suite and minimizes each generated function with Minion.

Minimize a suite function in C++ with Minion:

```cpp
#include "benchmark.h"
#include <minion.h>

#include <utility>
#include <vector>

int main() {
    FuncCraft::SuiteSpec spec =
        FuncCraft::load_suite_spec_yaml("BenchmarkSuites/default_suite.yaml");
    spec.requested_number_of_functions = 10;

    FuncCraft::BenchmarkSuite suite(spec, 10);
    const FuncCraft::BenchmarkFunction& f = suite.function(0);
    const FuncCraft::Domain& domain = f.domain();

    std::vector<std::pair<double, double>> bounds;
    for (int i = 0; i < domain.dimension(); ++i) {
        bounds.emplace_back(domain.lower[i], domain.upper[i]);
    }

    std::vector<double> x0(10, 0.0);
    auto objective = [&f](const std::vector<std::vector<double>>& X, void*) {
        return f(X);
    };

    auto settings = minion::DefaultSettings().getDefaultSettings("ARRDE");
    settings["convergence_tol"] = 1e-8;

    minion::Minimizer optimizer(
        objective,
        bounds,
        x0,
        nullptr,
        nullptr,
        "ARRDE",
        10000,
        1,
        settings);

    minion::MinionResult result = optimizer.optimize();
}
```

## Python Usage

All benchmark evaluations are batched. Pass a list of points, such as
`[[0.0] * dimension, [1.0] * dimension]`, rather than one flat point vector.

Create one composed benchmark function:

```python
import numpy as np
from funccraft import (
    BasicF,
    BasicFunctionId,
    BenchmarkFunction,
    ComponentSpec,
    CompositionSpec,
    FunctionSpec,
    TransformSpec,
    ValueTransformSpec,
    load_function_spec_yaml,
)

sphere_base = BasicF(BasicFunctionId.Sphere, 2)
rastrigin_base = BasicF(BasicFunctionId.Rastrigin, 2)

rng = np.random.default_rng(1)
x_star = rng.uniform(-4.0, 4.0, size=2).tolist()

identity = TransformSpec(
    kind="identity",
    dimension=2,
    source_point=x_star,
    target_point=list(sphere_base.x_opt),
)

rotation = TransformSpec(
    kind="rotation",
    dimension=2,
    seed=17,
    source_point=x_star,
    target_point=list(rastrigin_base.x_opt),
)

spec = FunctionSpec(
    dimension=2,
    lower_bound=[-5.0, -5.0],
    upper_bound=[5.0, 5.0],
    component_specs=[
        ComponentSpec(
            base_function="Sphere",
            component_dimension=2,
            coordinate_transform=identity,
            value_transform=ValueTransformSpec(kind="identity"),
        ),
        ComponentSpec(
            base_function="Rastrigin",
            component_dimension=2,
            coordinate_transform=rotation,
            value_transform=ValueTransformSpec(kind="power", parameters=[1.25, 1.0]),
        ),
    ],
    composition_spec=CompositionSpec(kind="weighted_sum", weights=[0.5, 0.5]),
    known_global_minimizer=x_star,
    known_global_value=0.0,
)

f = BenchmarkFunction(spec)
before = f.evaluate([x_star, [1.0, 1.0]])

f.export_spec("function.yaml")
same_f = BenchmarkFunction(load_function_spec_yaml("function.yaml"))
after = same_f.evaluate([x_star, [1.0, 1.0]])
assert before == after
```

Create and use a generated benchmark suite:

```python
from funccraft import BenchmarkSuite, load_suite_spec_yaml

suite_spec = load_suite_spec_yaml("BenchmarkSuites/default_suite.yaml")
suite_spec.requested_number_of_functions = 500
suite_spec.master_seed = 1

suite = BenchmarkSuite(suite_spec, 10)
points = [[0.0] * 10, [1.0] * 10]

for i in range(suite.size):
    f = suite.function(i)
    values = f.evaluate(points)

    x_star = f.spec.known_global_minimizer
    f_star = f.spec.known_global_value
    label = f.spec.function_class_label

suite.export_manifest("suite_manifest.yaml")
```

Minimize one generated function with SciPy:

```python
import numpy as np
from scipy.optimize import minimize
from funccraft import BenchmarkSuite, load_suite_spec_yaml

suite_spec = load_suite_spec_yaml("BenchmarkSuites/default_suite.yaml")
suite_spec.requested_number_of_functions = 10

suite = BenchmarkSuite(suite_spec, 10)
f = suite.function(0)
domain = f.domain

bounds = list(zip(domain.lower_bound, domain.upper_bound))
x0 = np.zeros(domain.dimension)

def objective(x):
    return f.evaluate([x.tolist()])[0]

result = minimize(objective, x0, method="L-BFGS-B", bounds=bounds)
print(result.fun, result.x)
```

The Python spec classes are dict-like. You can use attribute access
(`spec.master_seed`) or mapping access (`spec["master_seed"]`).

## CI Status

The badges at the top of this README show the pass/fail state for the standard
CI and wheel workflows: CI builds the C++ library, Python extension, and C++
test binary on Linux, Windows, and macOS arm, runs C++ and Python round-trip
tests, generates deterministic benchmark value tables on each platform, and
compares them with a `1e-12` tolerance; the wheel workflow builds Python wheels
and tests the installed wheel.

## Exported YAML

`BenchmarkFunction::export_spec()` and `BenchmarkFunction.export_spec()` write
one complete function spec. The exported YAML contains:

- dimension and bounds;
- component base functions;
- coordinate transform kind, seed, selected indices, source and target points,
  parameters, and generated matrix when applicable;
- value transform kind, seed, and parameters;
- composition kind, seed, weights, centers, offsets, and parameters;
- known global minimizer and known global value;
- runtime normalization values under `runtime.lambda` and `runtime.bias`.

`BenchmarkSuite::export_manifest()` and `BenchmarkSuite.export_manifest()` write
the suite spec plus every generated function spec. This is the right export
when the exact suite, not just one function, needs to be reproduced.
