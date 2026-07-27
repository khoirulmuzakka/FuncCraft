<p align="center">
  <img src="logo/logo.png" alt="FuncCraft logo" width="450">
</p>

# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Documentation Status](https://readthedocs.org/projects/funccraft/badge/?version=latest)](https://funccraft.readthedocs.io/)
[![PyPI version](https://img.shields.io/pypi/v/funccraft.svg)](https://pypi.org/project/funccraft/)
[![PyPI Python Version](https://img.shields.io/pypi/pyversions/funccraft.svg)](https://pypi.org/project/funccraft/)
[![PyPI pip downloads](https://img.shields.io/pypi/dm/funccraft.svg)](https://pypi.org/project/funccraft/)

FuncCraft is a C++17 library with a Python interface for generating scalable
black-box optimization benchmark suites. It builds benchmark functions from
editable YAML files, packaged suite collections, primitive benchmark
functions, coordinate transforms, value transforms, and composition rules.

## Why FuncCraft?

Common black-box optimization benchmark suites are intentionally finite. For
example, CEC2017 has 30 functions, CEC2020 has 10, CEC2022 has 12, and
BBOB/COCO 2009 has 24. They also target fixed dimension sets, such as
10/30/50/100 for CEC2017, 5/10/15/20 for CEC2020, and 2/5/10/20/40 for
BBOB/COCO 2009. These suites are useful standards, but they limit how many
distinct landscapes and dimensions can be used in large-scale experiments.

Many traditional benchmarks also rely mainly on shifting and rotation.
FuncCraft generalizes that idea by combining 34 primitive base functions with
a wider set of coordinate transformations, value transformations, composition
functions, and optional nested composed functions. This makes the number of
possible benchmark-function combinations practically unlimited.

FuncCraft is designed to:

- generate practically unlimited benchmark functions at any dimension;
- define reproducible benchmark suites by editing YAML files;
- define reproducible custom benchmark functions with YAML;
- preserve useful function identity under dimension changes;
- improve cross-platform robustness for generated suites;
- evaluate points in batches, which keeps Python calls efficient;
- expose the same suite-generation model from C++ and Python.

## Mechanism Summary

FuncCraft builds functions from primitive benchmark landscapes, coordinate
transforms, value transforms, and composition rules:

```text
f(x) = assigned_fopt + scale_factor * psi(x, z_1(x), ..., z_m(x))
```

Implemented mechanism families include:

- 34 primitive base functions, including Sphere, Rosenbrock, Ackley,
  Rastrigin, Griewank, Schwefel, Katsuura, Levy, BentCigar, HappyCat,
  HGBat, and StyblinskiTang.
- coordinate transforms: `none`, `rotation`, `affine`, `subspace-rotation`.
- value transforms: `none`, `power`, `oscillatory`, `cosine-zero`.
- compositions: `none`, `cpm-wsum`, `cpm-power-mean`, `cpm-level-well`,
  `dpm-softmax`, `dpm-bgsoftmax`.

Components can also be nested composed functions, up to the configured
`max_nested_composition_depth`.

Names are parsed permissively: case, spaces, hyphens, and underscores are
normalized before matching.

## Install

For supported Python versions and platforms, install FuncCraft from PyPI:

```bash
python -m pip install --upgrade funccraft
```

Optional optimization examples use SciPy or MinionPy:

```bash
python -m pip install scipy minionpy
```

For C++ usage, unsupported platforms, unsupported Python versions, or local
development, build FuncCraft from source. The Python package can be built and
installed from a checkout with:

```bash
python -m pip install .
```

Native C++ builds and detailed source-install instructions are covered in
`docs/source/installation.rst`.

## Python Quick Start

FuncCraft has two central runtime objects. A `BenchmarkSuite` is a materialized
collection of benchmark functions for one chosen dimension. A
`BenchmarkFunction` is one concrete callable benchmark function. All
evaluations are batched: pass a list of points and receive one value per
point.

In Python, the recommended custom-suite workflow is a YAML-shaped dictionary:

```python
import funccraft as fc

dimension = 10
function_index = 1

suite_config = {
    "supported_dimensions": "any",
    "base_functions": list(range(1, 35)),
    "composition_base_functions": [
        4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20,
        21, 23, 28, 30, 34, 33, 2, 3, 5, 9, 26, 27,
    ],
    "coordinate_transforms": [
        {"kind": "none", "probability": 0.0, "parameters": []},
        {"kind": "rotation", "probability": 0.34, "parameters": []},
        {"kind": "affine", "probability": 0.33, "parameters": []},
        {"kind": "subspace-rotation", "probability": 0.33, "parameters": []},
    ],
    "value_transforms": [
        {"kind": "none", "probability": 0.5, "parameters": []},
        {"kind": "power", "probability": 0.25, "parameters": []},
        {"kind": "osc", "probability": 0.25, "parameters": []},
        {"kind": "cosine-zero", "probability": 0.0, "parameters": []},
    ],
    "compositions": [
        {"kind": "cpmsum", "probability": 0.1, "parameters": []},
        {"kind": "cpmpmean", "probability": 0.1, "parameters": [3.0]},
        {"kind": "cpmpmean", "probability": 0.1, "parameters": [0.1]},
        {"kind": "cpmlwell", "probability": 0.2, "parameters": []},
        {"kind": "dpmsoftmax", "probability": 0.25, "parameters": [0.005]},
        {"kind": "dpmbgsoftmax", "probability": 0.25, "parameters": [0.005, 1.0, 0.01]},
    ],
    "min_components": 2,
    "max_components": 5,
    "max_nested_composition_depth": 1,
    "nested_probability": 0.1,
    "requested_number_of_functions": 500,
    "master_seed": 1,
    "lower_bound": -100,
    "upper_bound": 100,
    "assigned_fopt": 100.0,
    "xopt_domain_shrink_factor": 0.8,
    "suite_label": "readme-python-suite",
}

suite = fc.BenchmarkSuite(suite_config, dimension)
f = suite.function(function_index)

points = [[0.0] * dimension, [1.0] * dimension]
values = f.evaluate(points)

print(f.label)
print(f.get_xopt())
print(f.get_fopt())
print(values)
```

For the packaged benchmark suite, use
`fc.SuiteCollection(year=2026, version=1).benchmark_suite(dimension)`.

Warning: the packaged `2026_v1` suite contains 1,000,000 functions. Exporting
the full shipped suite manifest will write every generated function record and
is usually a mistake. To export a smaller reproducible subset, get the
collection configuration dictionary, reduce `requested_number_of_functions`,
and export the smaller suite:

```python
collection = fc.SuiteCollection(year=2026, version=1)
subset_config = collection.config
subset_config["requested_number_of_functions"] = 500

subset_suite = fc.BenchmarkSuite(subset_config, dimension=2)
subset_suite.export_manifest("suite_2026_v1_first_500.yaml")
```

More Python examples are in `docs/source/python/examples.rst` and the
`examples/` folder.

## C++ Quick Start

The C++ API uses the same model: materialize a `BenchmarkSuite` at an explicit
dimension, select a `BenchmarkFunction` by index, and evaluate a batch of
points. For C++, the natural custom-suite workflow is a suite YAML file.

Save this as `my_suite.yaml`:

```yaml
supported_dimensions: any
base_functions: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34]
composition_base_functions: [4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 23, 28, 30, 34, 33, 2, 3, 5, 9, 26, 27]
coordinate_transforms:
  - kind: none
    probability: 0.0
    parameters: []
  - kind: rotation
    probability: 0.34
    parameters: []
  - kind: affine
    probability: 0.33
    parameters: []
  - kind: subspace-rotation
    probability: 0.33
    parameters: []
value_transforms:
  - kind: none
    probability: 0.5
    parameters: []
  - kind: power
    probability: 0.25
    parameters: []
  - kind: osc
    probability: 0.25
    parameters: []
  - kind: cosine-zero
    probability: 0.0
    parameters: []
compositions:
  - kind: cpmsum
    probability: 0.1
    parameters: []
  - kind: cpmpmean
    probability: 0.1
    parameters: [3.0]
  - kind: cpmpmean
    probability: 0.1
    parameters: [0.1]
  - kind: cpmlwell
    probability: 0.2
    parameters: []
  - kind: dpmsoftmax
    probability: 0.25
    parameters: [0.005]
  - kind: dpmbgsoftmax
    probability: 0.25
    parameters: [0.005, 1.0, 0.01]
min_components: 2
max_components: 5
max_nested_composition_depth: 1
nested_probability: 0.1
requested_number_of_functions: 500
master_seed: 1
lower_bound: -100
upper_bound: 100
assigned_fopt: 100.0
xopt_domain_shrink_factor: 0.8
suite_label: readme-cpp-suite
```

Then load it with `#include "funccraft.h"`:

```cpp
#include "funccraft.h"

#include <iostream>
#include <vector>

int main() {
    const int dimension = 10;
    const int function_index = 1;

    FuncCraft::SuiteSpec config = FuncCraft::load_suite_spec("my_suite.yaml");
    FuncCraft::BenchmarkSuite suite(config, dimension);
    const FuncCraft::BenchmarkFunction& f = suite.function(function_index);

    std::vector<std::vector<double>> points = {
        std::vector<double>(dimension, 0.0),
        std::vector<double>(dimension, 1.0),
    };
    std::vector<double> values = f(points);

    std::cout << f.label() << '\n';
    std::cout << f.get_fopt() << '\n';
    std::cout << values.front() << '\n';
}
```

More C++ examples are in `docs/source/cpp/examples.rst` and the `examples/`
folder.

## Documentation

Full documentation is available at https://funccraft.readthedocs.io/.

## Development

Build the native test target and run the test suite:

```bash
cmake --build build --config Release --target funccraft_test
bin/funccraft_test
python tests/test.py
```

The CI workflow builds the C++ library, Python extension, and wheels across
supported platforms. Details are in `docs/source/project/testing_ci.rst`.
