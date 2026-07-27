"""Python entry point for FuncCraft.

Importing :mod:`funccraft` gives you both the plain-data specification types
and the runtime benchmark wrappers:

- spec types and dictionary/YAML conversion live in :mod:`funccraft.spec`;
- one-function runtime wrappers live in :mod:`funccraft.benchmark_function`;
- suite-level runtime wrappers live in :mod:`funccraft.suite`.

Examples
--------
Use the built-in suite collection::

    import funccraft as fc

    year = 2026
    version = 1
    collection = fc.suite_collection(year, version)
    suite = collection.benchmark_suite(dimension=10)
    f = suite.function(1)
    values = f.evaluate([[0.0] * 10])

Create one function manually::

    import funccraft as fc

    spec = {
        "dimension": 2,
        "domain": {
            "dimension": 2,
            "lower_bound": [-10.0, -10.0],
            "upper_bound": [10.0, 10.0],
        },
        "components": [
            {
                "base_function": "Ackley",
                "coordinate_transform": {
                    "kind": "none",
                    "input_dimension": 2,
                    "output_dimension": 2,
                    "assigned_xopt": [0.0, 0.0],
                },
                "value_transform": {"kind": "none"},
            },
        ],
        "composition": {"kind": "none"},
        "assigned_xopt": [0.0, 0.0],
        "scale_factor": 1.0,
    }
    f = fc.BenchmarkFunction(spec)
    y = f([[0.0, 0.0]])
"""

from .benchmark_function import *  # noqa: F401,F403
from .spec import *  # noqa: F401,F403
from .suite import *  # noqa: F401,F403
from ._funccraft import BasicF, BasicFunctionId  # noqa: F401
