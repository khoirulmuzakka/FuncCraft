"""Python entry point for FuncCraft.

Importing :mod:`pyfunccraft` gives you both the plain-data specification types
and the runtime benchmark wrappers:

- spec types live in :mod:`pyfunccraft.function_spec`
- one-function runtime wrappers live in :mod:`pyfunccraft.benchmark_function`
- suite-level runtime wrappers live in :mod:`pyfunccraft.suite`
"""

from .benchmark_function import *  # noqa: F401,F403
from .function_spec import *  # noqa: F401,F403
from .suite import *  # noqa: F401,F403
from ._funccraft import BasicF, BasicFunctionId  # noqa: F401
