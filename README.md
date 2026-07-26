<p align="center">
  <img src="logo/logo.png" alt="FuncCraft logo" width="450">
</p>

# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)
[![Documentation Status](https://readthedocs.org/projects/funccraft/badge/?version=latest)](https://funccraft.readthedocs.io/)
[![PyPI version](https://img.shields.io/pypi/v/funccraft.svg)](https://pypi.org/project/funccraft/)
[![PyPI Python Version](https://img.shields.io/pypi/pyversions/funccraft.svg)](https://pypi.org/project/funccraft/)
[![PyPI pip downloads](https://img.shields.io/pypi/dm/funccraft.svg)](https://pypi.org/project/funccraft/)
[![PyPI License](https://img.shields.io/pypi/l/funccraft.svg)](https://pypi.org/project/funccraft/)

FuncCraft is a C++17 library with a Python interface for generating scalable
continuous-optimization benchmark suites. A single suite specification can
generate hundreds, thousands, or practically unlimited numbers of distinct
benchmark functions across dimensions while keeping control over the
constructed optimum location and optimum value.

FuncCraft is configured with YAML-friendly specs. The main workflow is to edit
a suite YAML file, load it in C++ or Python, choose a dimension, and evaluate
the generated benchmark functions in batches.

## Install

Python:

```bash
python -m pip install --upgrade funccraft
python -m pip install numpy scipy minionpy
```

From source:

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

On Windows, `compile.bat` builds release by default and `compile.bat --debug`
builds debug. On Linux/macOS, use `./compile.sh` or `./compile.sh --debug`.

## Configure A Suite With YAML

Suite YAML is the easiest way to create and edit benchmark suites:

```yaml
supported_dimensions: any
base_functions: [1, 9, 10, 11, 12]
composition_base_functions: [9, 10, 11, 12]

coordinate_transforms:
  - kind: rotation
    probability: 0.5
  - kind: blockrotation
    probability: 0.5

value_transforms:
  - kind: none
    probability: 0.5
  - kind: osc
    probability: 0.5

compositions:
  - kind: cpmsum
    probability: 0.5
  - kind: dpmsoftmax
    probability: 0.5
    parameters: [0.005]

min_components: 2
max_components: 4
max_nested_composition_depth: 1
nested_probability: 0.1
requested_number_of_functions: 500
master_seed: 1
lower_bound: -100
upper_bound: 100
assigned_fopt: 100.0
xopt_domain_shrink_factor: 0.8
suite_label: my-suite
```

Choice probabilities in each table are fractions and should sum to one. Names
are parsed permissively: case, spaces, hyphens, and underscores are normalized.
For example, `DPM BG Softmax`, `dpm-bgsoftmax`, and `dpmbgsoftmax` refer to
the same composition.

The repository ships a standard YAML suite at `suites/2026_v1.yaml`. You can
copy it, edit it, and load your edited file.

## Python Usage

Load an editable suite YAML and evaluate batched points:

```python
import funccraft as fc

dimension = 10
suite_spec = fc.load_suite_spec("my_suite.yaml")
suite = fc.BenchmarkSuite(suite_spec, dimension)

points = [[0.0] * dimension, [1.0] * dimension]
function_index = 0
f = suite.function(function_index)
values = f.evaluate(points)

print(f.spec.label)
print(f.get_xopt())
print(f.get_fopt())
print(values)
```

For the packaged 2026 suite, use the collection shortcut:

```python
import funccraft as fc

dimension = 10
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
function_index = 0
values = suite.evaluate(function_index, [[0.0] * dimension])
```

All evaluations are batched. Pass a list of points, not a single flat vector.

Minimize one generated function with SciPy:

```python
import numpy as np
from scipy.optimize import differential_evolution
import funccraft as fc

dimension = 10
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
function_index = 0
f = suite.function(function_index)
domain = f.domain
bounds = list(zip(domain.lower_bound, domain.upper_bound))

def objective(x):
    return f.evaluate([np.asarray(x, dtype=float).tolist()])[0]

result = differential_evolution(objective, bounds, seed=1, polish=False)
print(result.x, result.fun)
```

Minimize with MinionPy:

```python
import funccraft as fc
import minionpy as mpy

dimension = 10
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
function_index = 0
f = suite.function(function_index)
domain = f.domain

optimizer = mpy.Minimizer(
    func=f.evaluate,
    x0=[
        [0.0] * dimension,
        [1.0] * dimension,
        [-0.5] * dimension,
    ],
    bounds=list(zip(domain.lower_bound, domain.upper_bound)),
    algo="ARRDE",
    maxevals=10000,
    callback=None,
    seed=None,
    options=None,
)
result = optimizer.optimize()
print(result.x, result.fun)
```

## C++ Usage

Use `#include "funccraft.h"` for normal C++ code.

Load an editable suite YAML and evaluate batched points:

```cpp
#include "funccraft.h"

#include <iostream>
#include <vector>

int main() {
    const int dimension = 10;
    const int function_index = 0;

    FuncCraft::SuiteSpec spec =
        FuncCraft::load_suite_spec("my_suite.yaml");
    FuncCraft::BenchmarkSuite suite(spec, dimension);

    const FuncCraft::BenchmarkFunction& f = suite.function(function_index);
    std::vector<std::vector<double>> points = {
        std::vector<double>(dimension, 0.0),
        std::vector<double>(dimension, 1.0),
    };
    std::vector<double> values = f(points);

    std::cout << f.spec().label << '\n';
    std::cout << values.front() << '\n';
}
```

For the packaged 2026 suite:

```cpp
#include "funccraft.h"

int main() {
    const int dimension = 10;
    const int year = 2026;
    const int version = 1;
    FuncCraft::SuiteCollection collection =
        FuncCraft::suite_collection(year, version);
    FuncCraft::BenchmarkSuite suite =
        collection.benchmark_suite(dimension);
}
```

Minimize one generated function with Minion:

```cpp
#include "funccraft.h"
#include <minion.h>

#include <utility>
#include <vector>

int main() {
    const int dimension = 10;
    const int function_index = 0;
    const int year = 2026;
    const int version = 1;

    FuncCraft::BenchmarkSuite suite =
        FuncCraft::suite_collection(year, version).benchmark_suite(dimension);
    const FuncCraft::BenchmarkFunction& f = suite.function(function_index);
    const FuncCraft::Domain& domain = f.domain();

    std::vector<std::pair<double, double>> bounds;
    for (int i = 0; i < domain.dimension(); ++i) {
        bounds.emplace_back(domain.lower[i], domain.upper[i]);
    }

    std::vector<double> x0(dimension, 0.0);
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

## Function Specs

A `FunctionSpec` describes one benchmark function. You can create it in code
or write it as YAML. User-authored function specs can omit generated details
such as rotation matrices and scale factors; FuncCraft fills them when the
runtime `BenchmarkFunction` is built.

```python
import funccraft as fc

spec = fc.load_function_spec("my_function.yaml")
f = fc.BenchmarkFunction(spec)
values = f.evaluate([[0.0, 0.0]])
```

## Exported Manifests

Input YAML is for configuration and editing. Exported function specs and suite
manifests are materialized records: they include generated matrices, selected
subspaces, DPM centers and biases, assigned optima, scale factors, labels, and
metadata.

```python
f.export_spec("function_materialized.yaml")
suite.export_manifest("suite_manifest.yaml")
```

Archive an exported manifest when you want the exact generated function table
used in an experiment.

## Documentation

Full documentation is available at https://funccraft.readthedocs.io/.

Key pages:

- `docs/source/yaml_specs.rst`: editable YAML specs.
- `docs/source/construction.rst`: mathematical construction model.
- `docs/source/mechanisms.rst`: implemented base functions, transforms, and compositions.
- `docs/source/funccraft_py/examples.rst`: Python examples.
- `docs/source/funccraft_cpp/examples.rst`: C++ examples.

## CI Status

The CI workflow builds the C++ library, Python extension, and C++ test binary
on Linux, Windows, and macOS arm, runs C++ and Python tests, generates
cross-platform benchmark value tables, requires F1-F34 to have every sampled
value agree within `1e-8` relative error, and applies a 75% point-agreement /
75% function-agreement rule to the remaining generated functions. The wheel
workflow builds and tests Python wheels.
