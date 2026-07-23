"""Python entry point for FuncCraft.

Importing :mod:`funccraft` gives you both the plain-data specification types
and the runtime benchmark wrappers:

- spec types and ``make_*`` helpers live in :mod:`funccraft.spec`;
- one-function runtime wrappers live in :mod:`funccraft.benchmark_function`;
- suite-level runtime wrappers live in :mod:`funccraft.suite`.

Examples
--------
Use the built-in suite collection::

    import funccraft as fc

    collection = fc.suite_collection(2026, 1)
    suite = collection.benchmark_suite(dimension=10)
    values = suite.evaluate(0, [[0.0] * 10])

Create one function manually::

    import funccraft as fc

    spec = fc.make_function_spec(
        dimension=2,
        domain=fc.make_domain(2, -10.0, 10.0),
        components=[fc.make_component(base_function="Ackley")],
        assigned_xopt=[0.0, 0.0],
        scale_factor=1.0,
    )
    f = fc.BenchmarkFunction(spec)
    y = f([[0.0, 0.0]])
"""

from .benchmark_function import *  # noqa: F401,F403
from .spec import *  # noqa: F401,F403
from .suite import *  # noqa: F401,F403
from ._funccraft import BasicF, BasicFunctionId  # noqa: F401
