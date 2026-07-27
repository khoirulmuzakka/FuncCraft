"""FuncCraft Python API."""

from .funccraft import *  # noqa: F401,F403
from .funccraft import __all__

try:
    del funccraft
except NameError:
    pass
