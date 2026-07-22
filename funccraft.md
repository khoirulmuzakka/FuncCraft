# FuncCraft

[![CI](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/ci.yml)
[![Wheel](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml/badge.svg)](https://github.com/khoirulmuzakka/FuncCraft/actions/workflows/wheel.yaml)

FuncCraft is a Python package backed by a C++17 benchmark-function generator
for continuous optimization research. It is designed to generate large,
reproducible benchmark suites across dimensions: from one suite specification
you can create hundreds, thousands, or practically unbounded numbers of
distinct benchmark functions while keeping control of the constructed
function's known minimum location.

FuncCraft functions are built from primitive benchmark landscapes, coordinate
transforms, value transforms, and composition rules. Each generated function can
be exported to YAML and rebuilt later with the same parameters, including
transform matrices, selected subspaces, component centers, offsets, scale, and
bias.

## Installation

Install from PyPI:

```bash
python -m pip install --upgrade funccraft
```

For optional optimization examples with SciPy:

```bash
python -m pip install funccraft scipy numpy
```

## Core Idea

FuncCraft follows a general composition framework:

```text
f(x) = psi(x, phi_1(g_1(T_1(x))), ..., phi_m(g_m(T_m(x))))
```

where `g_i` is a primitive benchmark function, `T_i` is a coordinate transform,
`phi_i` is a value transform, and `psi` combines the transformed component
values.

For implemented linear coordinate transforms:

```text
T(x) = target_point + M * (x - source_point)
```

`source_point` is the desired minimum location in generated/search
coordinates. `target_point` is the corresponding primitive-coordinate minimum,
which must be the base function's `x_opt` by construction.

## Implemented Mechanisms

Primitive functions are exposed through `BasicFunctionId`. The package includes
36 base landscapes such as Sphere, Ellipsoidal, Rosenbrock, Ackley, Rastrigin,
Griewank, Schwefel, Schaffer variants, Gallagher21, Katsuura, Levy,
Michalewicz, BentCigar, Discus, Step, Quartic, Exponential, and
StyblinskiTang.

Coordinate transforms:

- `identity` / `none`
- `rotation` / `rot`
- `affine` / `aff`
- `block_rotation` / `blockrotation` / `blockrot` / `brot`

Value transforms:

- `identity` / `none`
- `power`
- `oscillatory` / `osc`
- `cosine_zero` / `coszero`

Composition functions:

- `single_component` / `single`
- `weighted_sum` / `sum` / `cpm`
- `power_mean` / `powermean`
- `level_well` / `levelwell` / `lvlwell`
- `deceptive_softmax` / `dpm`
- `deceptive_bg_softmax` / `dpmbgsoftmax`

## Batched Evaluation

All FuncCraft evaluations are vectorized over a batch of points. Pass a list of
points, not one flat point vector:

```python
values = f.evaluate([[0.0] * dimension, [1.0] * dimension])
```

The return value contains one function value per input point.

## Generate A Suite

```python
from funccraft import BenchmarkSuite, SuiteSpec

spec = SuiteSpec()
spec.requested_number_of_functions = 500
spec.master_seed = 1
spec.lower_bound = -100.0
spec.upper_bound = 100.0

suite = BenchmarkSuite(spec, 10)
points = [[0.0] * 10, [1.0] * 10]

for i in range(suite.size):
    f = suite.function(i)
    values = f.evaluate(points)
    x_star = f.spec.known_global_minimizer
    f_star = f.spec.known_global_value
    label = f.spec.function_class_label
```

Export the exact suite used in an experiment:

```python
suite.export_manifest("suite_manifest.yaml")
```

## Create One Function Manually

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
)

sphere_base = BasicF(BasicFunctionId.Sphere, 2)
rastrigin_base = BasicF(BasicFunctionId.Rastrigin, 2)

rng = np.random.default_rng(1)
x_star = rng.uniform(-4.0, 4.0, size=2).tolist()

sphere_transform = TransformSpec(
    kind="identity",
    dimension=2,
    source_point=x_star,
    target_point=list(sphere_base.x_opt),
)

rastrigin_transform = TransformSpec(
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
            coordinate_transform=sphere_transform,
            value_transform=ValueTransformSpec(kind="identity"),
        ),
        ComponentSpec(
            base_function="Rastrigin",
            component_dimension=2,
            coordinate_transform=rastrigin_transform,
            value_transform=ValueTransformSpec(kind="power", parameters=[1.25, 1.0]),
        ),
    ],
    composition_spec=CompositionSpec(kind="weighted_sum", weights=[0.5, 0.5]),
    known_global_minimizer=x_star,
    known_global_value=0.0,
)

f = BenchmarkFunction(spec)
values = f.evaluate([x_star, [1.0, 1.0]])
f.export_spec("function.yaml")
```

## Minimize With SciPy

```python
import numpy as np
from scipy.optimize import minimize
from funccraft import BenchmarkSuite, SuiteSpec

spec = SuiteSpec()
spec.requested_number_of_functions = 10
spec.master_seed = 1

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
from funccraft import BenchmarkFunction, load_function_spec_yaml

f.export_spec("function.yaml")
same_f = BenchmarkFunction(load_function_spec_yaml("function.yaml"))
```

One suite:

```python
suite.export_manifest("suite_manifest.yaml")
```

The exported YAML stores the normalized suite/function specification and the
runtime normalization values needed to reproduce evaluations.

## Links

- Source: https://github.com/khoirulmuzakka/FuncCraft
- Issues: https://github.com/khoirulmuzakka/FuncCraft/issues
