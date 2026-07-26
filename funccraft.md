# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)
[![Documentation Status](https://readthedocs.org/projects/funccraft/badge/?version=latest)](https://funccraft.readthedocs.io/)

FuncCraft is a Python package backed by a C++17 benchmark-function generator
for continuous optimization research. It is designed for scalable benchmark
suite generation across dimensions: one editable suite specification can
generate hundreds, thousands, or practically unlimited numbers of distinct
benchmark functions while controlling the constructed optimum location and
optimum value.

## Install

```bash
python -m pip install --upgrade funccraft
python -m pip install numpy scipy minionpy
```

## YAML-First Workflow

Suite YAML is the easiest way to configure FuncCraft:

```yaml
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
requested_number_of_functions: 500
master_seed: 1
lower_bound: -100
upper_bound: 100
assigned_fopt: 100.0
```

Load the YAML and evaluate a function:

```python
import funccraft as fc

dimension = 10
function_index = 0

spec = fc.load_suite_spec("my_suite.yaml")
suite = fc.BenchmarkSuite(spec, dimension)
f = suite.function(function_index)

points = [[0.0] * dimension, [1.0] * dimension]
values = f.evaluate(points)
print(values)
```

The packaged 2026 suite is also YAML-backed and exposed through a shortcut:

```python
import funccraft as fc

dimension = 10
function_index = 0
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
values = suite.evaluate(function_index, [[0.0] * dimension])
```

All evaluations are batched: pass a list of points, not one flat point vector.

## Optimization

SciPy:

```python
import numpy as np
from scipy.optimize import differential_evolution
import funccraft as fc

dimension = 10
function_index = 0
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
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
function_index = 0
year = 2026
version = 1
suite = fc.suite_collection(year, version).benchmark_suite(dimension)
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
- coordinate transforms: `none`, `rotation`, `affine`, `block-rotation`.
- value transforms: `none`, `power`, `oscillatory`, `cosine-zero`.
- compositions: `none`, `cpm-wsum`, `cpm-power-mean`, `cpm-level-well`,
  `dpm-softmax`, `dpm-bgsoftmax`.

Names are parsed permissively: case, spaces, hyphens, and underscores are
normalized before matching.

## Exported Manifests

Input YAML is for configuration and editing. Exported YAML is a materialized
record of what FuncCraft built:

```python
f.export_spec("function_materialized.yaml")
suite.export_manifest("suite_manifest.yaml")
```

Exported specs include generated matrices, selected subspaces, DPM centers and
biases, assigned optima, scale factors, labels, and metadata.

## Links

- Documentation: https://funccraft.readthedocs.io/
- Source: https://github.com/khoirulmuzakka/FuncCraft
- Issues: https://github.com/khoirulmuzakka/FuncCraft/issues
