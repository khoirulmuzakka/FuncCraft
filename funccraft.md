# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)

FuncCraft is a Python package backed by a C++17 benchmark-function generator
for continuous optimization research. It is designed for scalable generation
of reproducible benchmark suites across dimensions: one suite specification can
generate hundreds, thousands, or practically unbounded numbers of distinct
benchmark functions while preserving the metadata needed to reproduce each
function and control the constructed optimum location.

FuncCraft functions are built from primitive benchmark landscapes, coordinate
transforms, value transforms, and composition rules. Each generated function can
be exported to YAML and rebuilt later with the same parameters, including
transform matrices, selected subspaces, component centers, DPM composition
biases, and scale factors.

## Installation

```bash
python -m pip install --upgrade funccraft
```

For optional optimization examples:

```bash
python -m pip install funccraft numpy scipy
```

## Core Idea

FuncCraft follows a general composition framework:

```text
f(x) = psi(phi_1(g_1(T_1(x))), ..., phi_m(g_m(T_m(x))))
```

where `g_i` is a primitive benchmark function, `T_i` is a coordinate transform,
`phi_i` is a value transform, and `psi` combines the transformed component
values.

For implemented linear coordinate transforms:

```text
T(x) = target_xopt + M * (x - assigned_xopt)
```

`assigned_xopt` is the desired optimum location in generated/search
coordinates. `target_xopt` is runtime-only and is determined internally from
the selected base function optimum and active benchmark domain scaling.

## Implemented Mechanisms

Primitive functions are exposed through `BasicFunctionId`. The package includes
36 base landscapes such as Sphere, Ellipsoidal, Rosenbrock, Ackley, Rastrigin,
Griewank, Schwefel, Schaffer variants, Gallagher21, Katsuura, Levy,
Michalewicz, BentCigar, Discus, Step, Quartic, Exponential, and
StyblinskiTang.

Coordinate transforms:

- `none`
- `rotation`
- `affine`
- `block-rotation`

Value transforms:

- `none`
- `power`
- `oscillatory`
- `cosine-zero`

Composition functions:

- `none`: no composition for exactly one component.
- `cpm-wsum`: common-point weighted sum.
- `cpm-power-mean`: common-point power-mean aggregation.
- `cpm-level-well`: common-point level-well composition.
- `dpm-softmax`: deceptive-point softmax composition.
- `dpm-bgsoftmax`: deceptive-point softmax with a smooth background term.

Names are parsed permissively: case, spaces, hyphens, and underscores are
normalized before matching, so `DPM BG Softmax`, `dpm-bgsoftmax`, and
`dpmbgsoftmax` are equivalent.

## Batched Evaluation

All FuncCraft evaluations are batched over points. Pass a list of points, not
one flat point vector:

```python
values = f.evaluate([[0.0] * dimension, [1.0] * dimension])
```

The return value contains one function value per input point.

## Generate A Suite

```python
from funccraft import suite_collection

collection = suite_collection(2026, 1)
print(collection.name)
print(collection.number_of_functions)

suite = collection.benchmark_suite(
    dimension=10,
)
points = [[0.0] * 10, [1.0] * 10]

for i in range(suite.size):
    f = suite.function(i)
    values = f.evaluate(points)
    x_star = f.spec.assigned_xopt
    f_star = f.spec.assigned_fopt
    label = f.spec.label
```

`max_nested_composition_depth = 0` means composed suite functions use only
primitive components. Larger values allow components to be composed functions;
`nested_probability` controls how often a component is nested.

Export the exact suite used in an experiment:

```python
suite.export_manifest("suite_manifest.yaml")
```

## Create One Function Manually

```python
import numpy as np
from funccraft import (
    BenchmarkFunction,
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
values = f.evaluate([x_star, [1.0, 1.0]])
f.export_spec("function.yaml")
```

## Minimize With SciPy

```python
import numpy as np
from scipy.optimize import minimize
from funccraft import BenchmarkSuite, load_suite_spec

spec = load_suite_spec("suites/2026_v1.yaml")
spec.requested_number_of_functions = 10
suite = BenchmarkSuite(spec, 10)

f = suite.function(0)
domain = f.domain
bounds = list(zip(domain.lower_bound, domain.upper_bound))
x0 = np.zeros(domain.dimension)

def objective(x):
    return f.evaluate([x.tolist()])[0]

result = minimize(objective, x0, method="L-BFGS-B", bounds=bounds)
print(result.fun, result.x)
```

## YAML Reproducibility

One function:

```python
from funccraft import BenchmarkFunction, load_function_spec

f.export_spec("function.yaml")
same_f = BenchmarkFunction(load_function_spec("function.yaml"))
```

One suite:

```python
suite.export_manifest("suite_manifest.yaml")
```

The exported YAML stores the normalized suite/function specification and the
runtime values needed to reproduce evaluations.

## Links

- Source: https://github.com/khoirulmuzakka/FuncCraft
- Issues: https://github.com/khoirulmuzakka/FuncCraft/issues
