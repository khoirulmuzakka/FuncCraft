"""Runtime benchmark-function wrapper for FuncCraft.

Use :class:`BenchmarkFunction` when you already have a full
``FunctionSpec`` and want a callable object with the derived runtime pieces
materialized. It is the high-level object for evaluating one concrete
benchmark instance.
"""

from . import _funccraft
from .spec import DomainSpec, FunctionSpec, function_spec


def _as_native_function_spec(spec):
    return function_spec(spec)


def make_benchmark_function(spec):
    """Build a :class:`BenchmarkFunction` from a spec object, dict, or file path."""
    return BenchmarkFunction(spec)


def load_function_spec(path):
    """Load a native :class:`FunctionSpec` from a file."""
    return _funccraft.load_function_spec(str(path))


class BenchmarkFunction:
    """Concrete benchmark function.

    Parameters
    ----------
    spec:
        A :class:`~funccraft.spec.FunctionSpec` describing one
        function instance.

    Notes
    -----
    This wrapper delegates all heavy lifting to the compiled extension. It is
    intentionally small, but the attribute names mirror the structure of the
    underlying spec so the object can be inspected without reading C++.
    """

    def __init__(self, spec):
        if isinstance(spec, _funccraft.BenchmarkFunction):
            self._function = spec
            self._spec = self._function.spec
        elif isinstance(spec, str):
            self._function = _funccraft.make_benchmark_function(spec)
            self._spec = self._function.spec
        else:
            self._function = _funccraft.BenchmarkFunction(_as_native_function_spec(spec))
            self._spec = self._function.spec

    @property
    def domain(self):
        """Return the search domain of this function."""
        domain = DomainSpec()
        domain.dimension = self._function.domain.dimension
        domain.lower_bound = list(self._function.domain.lower)
        domain.upper_bound = list(self._function.domain.upper)
        return domain

    @property
    def dimension(self):
        """Return the ambient dimension of this function."""
        return self._function.dimension

    @property
    def scale_factor(self):
        """Return the runtime scale factor used to normalize values."""
        return self._function.scale_factor

    @property
    def scale(self):
        """Alias for :attr:`scale_factor`."""
        return self.scale_factor

    @property
    def assigned_fopt(self):
        """Return the assigned optimum value."""
        return self._function.assigned_fopt

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

    def evaluate(self, points):
        """Evaluate a batch of candidate points."""
        return self._function.evaluate(points)

    def export_spec(self, path):
        """Write the complete function reproducibility spec to a YAML file."""
        self._function.export_spec(str(path))

    def __repr__(self):
        return (
            "BenchmarkFunction("
            f"dimension={self.dimension}, "
            f"label='{self.spec.label}', "
            f"scale_factor={self.scale_factor})"
        )


__all__ = [
    "BenchmarkFunction",
    "DomainSpec",
    "FunctionSpec",
    "load_function_spec",
    "make_benchmark_function",
]
