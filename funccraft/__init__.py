"""Python entry point for FuncCraft.

Importing :mod:`funccraft` gives you both the plain-data specification types
and the runtime benchmark wrappers:

- spec types live in :mod:`funccraft.spec`
- one-function runtime wrappers live in :mod:`funccraft.benchmark_function`
- suite-level runtime wrappers live in :mod:`funccraft.suite`
"""

from .benchmark_function import *  # noqa: F401,F403
from .spec import *  # noqa: F401,F403
from .suite import *  # noqa: F401,F403
from ._funccraft import BasicF, BasicFunctionId  # noqa: F401
