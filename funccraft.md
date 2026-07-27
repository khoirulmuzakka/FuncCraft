# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)
[![Documentation Status](https://readthedocs.org/projects/funccraft/badge/?version=latest)](https://funccraft.readthedocs.io/)

FuncCraft is a Python package backed by a C++17 benchmark-function generator
for black-box optimization research. It is designed for scalable benchmark
suite generation across dimensions: one editable suite YAML file can
generate hundreds, thousands, or practically unlimited numbers of distinct
benchmark functions while controlling the constructed optimum location and
optimum value.

## Install

```bash
python -m pip install --upgrade funccraft
python -m pip install numpy scipy minionpy
```

## YAML-First Workflow

Suite YAML is the easiest way to configure FuncCraft from files. The same
structure can also be passed as a Python dictionary to `BenchmarkSuite`.

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
suite_label: my-suite
```

Load the YAML and evaluate a function. Function indices are one-based:

```python
import funccraft as fc

dimension = 10
function_index = 1

suite = fc.BenchmarkSuite("my_suite.yaml", dimension=dimension)
f = suite.function(function_index)

points = [[0.0] * dimension, [1.0] * dimension]
values = f.evaluate(points)
print(values)
```

The packaged 2026 suite is also YAML-backed and exposed through a shortcut:

```python
import funccraft as fc

dimension = 10
function_index = 1
year = 2026
version = 1
suite = fc.SuiteCollection(year=year, version=version).benchmark_suite(dimension)
f = suite.function(function_index)
values = f.evaluate([[0.0] * dimension])
```

All evaluations are batched: pass a list of points, not one flat point vector.

## Optimization

SciPy:

```python
import numpy as np
from scipy.optimize import differential_evolution
import funccraft as fc

dimension = 10
function_index = 1
year = 2026
version = 1
suite = fc.SuiteCollection(year=year, version=version).benchmark_suite(dimension)
f = suite.function(function_index)
domain = f.domain
bounds = list(zip(domain.lower_bound, domain.upper_bound))

def objective(x):
    return f.evaluate([np.asarray(x, dtype=float).tolist()])[0]

result = differential_evolution(objective, bounds, seed=1, polish=False)
print(result.x, result.fun)
```

MinionPy:

```python
import funccraft as fc
import minionpy as mpy

dimension = 10
function_index = 1
year = 2026
version = 1
suite = fc.SuiteCollection(year=year, version=version).benchmark_suite(dimension)
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

## Exported Manifests

Input YAML is for configuration and editing. Exported YAML is a materialized
record of what FuncCraft built:

```python
f.export_yaml("function_materialized.yaml")
suite.export_manifest("suite_manifest.yaml")
```

Exported YAML records include generated matrices, selected subspaces, DPM centers and
biases, assigned optima, scale factors, labels, and metadata.

Warning: the packaged `2026_v1` suite contains 1,000,000 functions. Exporting
the full shipped suite manifest will write every generated function record and
is usually a mistake. To export a smaller reproducible subset, copy the
collection configuration dictionary, reduce `requested_number_of_functions`,
build the smaller suite, and export that suite:

```python
import funccraft as fc

collection = fc.SuiteCollection(year=2026, version=1)
config = collection.config
config["requested_number_of_functions"] = 500

suite = fc.BenchmarkSuite(config, dimension=2)
suite.export_manifest("suite_2026_v1_first_500.yaml")
```

## Links

- Documentation: https://funccraft.readthedocs.io/
- Python examples: `docs/source/python/examples.rst`
- C++ examples: `docs/source/cpp/examples.rst`
- Source: https://github.com/khoirulmuzakka/FuncCraft
- Issues: https://github.com/khoirulmuzakka/FuncCraft/issues
