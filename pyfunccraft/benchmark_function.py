"""Runtime benchmark-function wrapper for FuncCraft.

Use :class:`BenchmarkFunction` when you already have a full
``FunctionSpec`` and want a callable object with the derived runtime pieces
materialized. It is the high-level object for evaluating one concrete
benchmark instance.
"""

from . import _funccraft
from .function_spec import Domain, FunctionSpec


def _as_native_function_spec(spec):
    if isinstance(spec, FunctionSpec):
        return spec.to_cpp()
    if isinstance(spec, dict):
        return FunctionSpec.from_dict(spec).to_cpp()
    return spec


def make_benchmark_function(spec):
    """Build a :class:`BenchmarkFunction` from a full function specification."""
    return BenchmarkFunction(spec)


class BenchmarkFunction:
    """Concrete benchmark function.

    Parameters
    ----------
    spec:
        A :class:`~pyfunccraft.function_spec.FunctionSpec` describing one
        function instance.

    Notes
    -----
    This wrapper delegates all heavy lifting to the compiled extension. It is
    intentionally small, but the attribute names mirror the structure of the
    underlying spec so the object can be inspected without reading C++.
    """

    def __init__(self, spec):
        self._function = _funccraft.BenchmarkFunction(_as_native_function_spec(spec))
        self._spec = FunctionSpec.from_cpp(self._function.spec)

    @property
    def domain(self):
        """Return the search domain of this function."""
        return Domain.from_cpp(self._function.domain)

    @property
    def dimension(self):
        """Return the ambient dimension of this function."""
        return self._function.dimension

    @property
    def scale(self):
        """Return the runtime lambda used to normalize composed values."""
        return self._function.scale

    @property
    def lambda_(self):
        """Alias for :attr:`scale`."""
        return self._function.scale

    @property
    def spec(self):
        """Return the normalized function specification used to build it."""
        return self._spec

    def __call__(self, points):
        """Evaluate a batch of candidate points.

        Parameters
        ----------
        points:
            A list of vectors, each with length equal to the function
            dimension.
        """
        return self._function(points)

    def __repr__(self):
        return (
            "BenchmarkFunction("
            f"dimension={self.dimension}, "
            f"label='{self.spec.function_class_label}', "
            f"scale={self.scale})"
        )


__all__ = [
    "BenchmarkFunction",
    "Domain",
    "FunctionSpec",
    "make_benchmark_function",
]
