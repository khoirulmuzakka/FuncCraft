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
including transform matrices, selected subspaces, component optimum locations,
component centers, DPM composition biases, and scale factors.

## Generating Mechanism

FuncCraft follows the general composition framework described in the
documentation. A benchmark is assembled as

```text
f(x) = psi(phi_1(g_1(T_1(x))), ..., phi_m(g_m(T_m(x))))
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
T(x) = target_xopt + M * (x - assigned_xopt)
```

Here `assigned_xopt` is the desired minimum location in the generated/search
coordinates. `target_xopt` is runtime-only and is filled internally by
`BenchmarkFunction` from the selected base function's native `x_opt` and the
active benchmark domain. This lets users place a generated optimum at
`assigned_xopt` without needing to know the primitive domain-scaling details.

The usual suite generator chooses these ingredients from weighted
choice lists, places the global optimum and component centers deterministically
from the suite seed, and then materializes a
`FunctionSpec`. A `FunctionSpec` is the reproducibility record: it is the data
used to build a `BenchmarkFunction`, and it is also what is exported to YAML.
For DPM compositions, component 0 is assigned to the constructed global
optimum and every other component center is sampled from the shrunken search
domain.

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

- `none` / `identity`: no composition for a one-component function.
- `cpm-wsum` / `cpmsum` / `weighted_sum`: common-point weighted sum.
- `cpm-power-mean` / `cpmpmean` / `power_mean`: common-point power-mean aggregation.
- `cpm-level-well` / `cpmlwell` / `level_well`: common-point level-well
  composition.
- `dpm-softmax` / `dpmsoftmax` / `dpm`: deceptive multi-point softmax composition.
- `dpm-bgsoftmax` / `dpmbgsoftmax`: deceptive softmax with a background
  component.

The default suite choices are in `include/suite_spec.h` and
`suites/2026_v1.yaml`.

Suite generation can also nest composed functions inside components.
`max_nested_composition_depth = 0` means composed suite functions use only
primitive components. Larger values allow nested composed components, and
`nested_probability` controls how often each component is sampled as a nested
composition.

## Repository Layout

- `include/`: public C++ headers.
- `src/`: C++ implementation and Python bindings.
- `funccraft/`: Python wrapper package.
- `suites/`: versioned YAML suite collections, named like `2026_v1.yaml`.
- `tests/`: Python and C++ round-trip and cross-platform value checks.
- `examples/`: optional examples; Minion-dependent examples are built only when
  `BUILD_EXAMPLES=ON`. See `examples/main_minimize.cpp` for an end-to-end
  example that minimizes functions from a generated benchmark suite.
- `docs/`: Sphinx/Read the Docs-style documentation source and build scripts.

## Public C++ API

Use `#include "funccraft.h"` for normal C++ code. This is the documented
high-level entry point for `BasicF`, `BenchmarkFunction`, `BenchmarkSuite`,
`SuiteCollection`, `FunctionSpec`, `SuiteSpec`, `load_function_spec`,
`load_suite_spec`, and construction/export helpers. Lower-level builder,
transform, and composition implementation headers are available separately for
internal or advanced use.

The built-in benchmark collections are versioned by year and version number
and are defined by YAML files in `suites/`, for example `2026_v1.yaml`.
For the current collection:

```cpp
FuncCraft::SuiteCollection collection = FuncCraft::suite_collection(2026, 1);
int n = collection.number_of_functions();
FuncCraft::SuiteSpec spec = collection.spec();
FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(10);
```

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

Create one composed benchmark function from a `FunctionSpec`:

```cpp
#include "funccraft.h"

#include <random>
#include <vector>

int main() {
    using namespace FuncCraft;

    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> uniform(-4.0, 4.0);
    std::vector<double> x_star = {uniform(rng), uniform(rng)};

    CoordinateTransformSpec sphere_transform;
    sphere_transform.kind = CoordinateTransformKind::None;
    sphere_transform.dimension = 2;
    sphere_transform.assigned_xopt = x_star;

    CoordinateTransformSpec rastrigin_transform;
    rastrigin_transform.kind = CoordinateTransformKind::Rotation;
    rastrigin_transform.dimension = 2;
    rastrigin_transform.seed = 17;
    rastrigin_transform.assigned_xopt = x_star;

    ValueTransformSpec no_value_transform;
    no_value_transform.kind = ValueTransformKind::None;

    ValueTransformSpec power;
    power.kind = ValueTransformKind::Power;
    power.parameters = {1.25, 1.0};

    ComponentSpec sphere;
    sphere.base_function = BasicFunctionId::Sphere;
    sphere.coordinate_transform = sphere_transform;
    sphere.value_transform = no_value_transform;

    ComponentSpec rastrigin;
    rastrigin.base_function = BasicFunctionId::Rastrigin;
    rastrigin.coordinate_transform = rastrigin_transform;
    rastrigin.value_transform = power;

    CompositionSpec composition;
    composition.kind = CompositionKind::CpmWeightedSum;

    FunctionSpec spec;
    spec.dimension = 2;
    spec.domain.dimension = 2;
    spec.domain.lower_bound = {-5.0, -5.0};
    spec.domain.upper_bound = {5.0, 5.0};
    spec.components = {sphere, rastrigin};
    spec.composition = composition;
    spec.assigned_xopt = x_star;
    spec.assigned_fopt = 0.0;

    BenchmarkFunction f(spec);
    std::vector<double> values = f({x_star, {1.0, 1.0}});

    f.export_spec("function.yaml");
    BenchmarkFunction same_f = make_benchmark_function("function.yaml");
}
```

Create a benchmark suite and evaluate generated functions:

```cpp
#include "funccraft.h"

#include <vector>

int main() {
    using namespace FuncCraft;

    SuiteSpec spec = load_suite_spec("suites/2026_v1.yaml");
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
        const std::vector<double>& x_star = f_spec.assigned_xopt;
        const double f_star = f_spec.assigned_fopt;
        const std::string& label = f_spec.label;
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
#include "funccraft.h"
#include <minion.h>

#include <utility>
#include <vector>

int main() {
    FuncCraft::SuiteSpec spec =
        FuncCraft::load_suite_spec("suites/2026_v1.yaml");
    spec.requested_number_of_functions = 40;

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

The Python package exposes the native C++ spec objects, plus small `make_*`
factory helpers so users do not need to know pybind constructor details. The
factories return native specs, not a separate Python spec model.

Create one composed benchmark function:

```python
import numpy as np
from funccraft import (
    BenchmarkFunction,
    make_benchmark_function,
    make_component,
    make_composition,
    make_coordinate_transform,
    make_domain,
    make_function_spec,
    make_value_transform,
)

rng = np.random.default_rng(1)
x_star = rng.uniform(-4.0, 4.0, size=2).tolist()

spec = make_function_spec(
    dimension=2,
    domain=make_domain(2, -5.0, 5.0),
    components=[
        make_component(
            base_function="Sphere",
            coordinate_transform=make_coordinate_transform(
                kind="none",
                dimension=2,
                assigned_xopt=x_star,
            ),
            value_transform=make_value_transform("none"),
        ),
        make_component(
            base_function="Rastrigin",
            coordinate_transform=make_coordinate_transform(
                kind="rotation",
                dimension=2,
                assigned_xopt=x_star,
                seed=17,
            ),
            value_transform=make_value_transform("power", parameters=[1.25, 1.0]),
        ),
    ],
    composition=make_composition("cpm-wsum"),
    assigned_xopt=x_star,
    assigned_fopt=0.0,
)

f = BenchmarkFunction(spec)
before = f.evaluate([x_star, [1.0, 1.0]])

f.export_spec("function.yaml")
same_f = make_benchmark_function("function.yaml")
after = same_f.evaluate([x_star, [1.0, 1.0]])
assert before == after
```

Create and use a generated benchmark suite:

```python
from funccraft import BenchmarkSuite, load_suite_spec

suite_spec = load_suite_spec("suites/2026_v1.yaml")
suite_spec.requested_number_of_functions = 500
suite_spec.master_seed = 1

suite = BenchmarkSuite(suite_spec, 10)
points = [[0.0] * 10, [1.0] * 10]

for i in range(suite.size):
    f = suite.function(i)
    values = f.evaluate(points)

    x_star = f.spec.assigned_xopt
    f_star = f.spec.assigned_fopt
    label = f.spec.label

suite.export_manifest("suite_manifest.yaml")
```

You can also construct a suite spec directly in Python:

```python
from funccraft import (
    BenchmarkSuite,
    make_composition_choice,
    make_coordinate_transform_choice,
    make_suite_spec,
    make_value_transform_choice,
)

suite_spec = make_suite_spec(
    requested_number_of_functions=500,
    min_components=2,
    max_components=4,
    max_nested_composition_depth=1,
    nested_probability=0.05,
    master_seed=1,
    compositions=[
        make_composition_choice("dpm-bgsoftmax", 0.5, [0.01, 1.0, 0.01]),
        make_composition_choice("dpm-softmax", 0.5, [0.01]),
    ],
    coordinate_transforms=[
        make_coordinate_transform_choice("rotation", 0.5),
        make_coordinate_transform_choice("block-rotation", 0.5),
    ],
    value_transforms=[
        make_value_transform_choice("none", 0.5),
        make_value_transform_choice("oscillatory", 0.5, [0.1, 1.0]),
    ],
)

suite = BenchmarkSuite(suite_spec, 10)
```

For compact scripts, nested dictionaries with the same current field names are
accepted anywhere a function or suite spec is expected:

```python
suite = BenchmarkSuite(
    {
        "requested_number_of_functions": 100,
        "max_components": 4,
        "compositions": [
            {"kind": "DPM BG Softmax", "probability": 1.0, "parameters": [0.01, 1.0, 0.01]},
        ],
    },
    10,
)
```

Minimize one generated function with SciPy:

```python
import numpy as np
from scipy.optimize import minimize
from funccraft import BenchmarkSuite, load_suite_spec

suite_spec = load_suite_spec("suites/2026_v1.yaml")
suite_spec.requested_number_of_functions = 40

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

For a broader notebook walkthrough, see `examples/Create_functions.ipynb`.

## CI Status

The badges at the top of this README show the pass/fail state for the standard
CI and wheel workflows: CI builds the C++ library, Python extension, and C++
test binary on Linux, Windows, and macOS arm, runs C++ and Python round-trip
tests, generates deterministic benchmark value tables on each platform, and
compares them with a `1e-7` relative tolerance; the wheel workflow builds
Python wheels and tests the installed wheel.

## Exported YAML

`BenchmarkFunction::export_spec()` and `BenchmarkFunction.export_spec()` write
one complete function spec. The exported YAML contains:

- dimension and bounds;
- component base functions;
- coordinate transform kind, seed, selected indices, `assigned_xopt`,
  parameters, and generated matrix when applicable;
- value transform kind and parameters;
- each component's assigned center;
- composition kind, parameters, and DPM biases when applicable;
- function-level `assigned_xopt` and `assigned_fopt`;
- `scale_factor` when it has been materialized or explicitly set;
- label and metadata.

`BenchmarkSuite::export_manifest()` and `BenchmarkSuite.export_manifest()` write
the suite spec plus every generated function spec. This is the right export
when the exact suite, not just one function, needs to be reproduced.
