"""Runtime benchmark-suite wrapper for FuncCraft.

`BenchmarkSuite` is the object you use when you want a family of benchmark
functions sampled from a :class:`~pyfunccraft.function_spec.SuiteSpec`.
It stores the generated blueprints lazily and materializes a concrete
:class:`~pyfunccraft.benchmark_function.BenchmarkFunction` only when you ask
for a specific index.
"""

from . import _funccraft
from .benchmark_function import BenchmarkFunction
from .function_spec import ChoiceSpec, SuiteSpec


def _as_native_suite_spec(spec):
    if isinstance(spec, SuiteSpec):
        return spec.to_cpp()
    if isinstance(spec, dict):
        return SuiteSpec.from_dict(spec).to_cpp()
    return spec


def make_benchmark_suite(spec, dimension):
    """Build a :class:`BenchmarkSuite` for the requested dimension."""
    return BenchmarkSuite(spec, dimension)


class BenchmarkSuite:
    """Concrete suite of generated benchmark functions.

    Parameters
    ----------
    spec:
        A :class:`~pyfunccraft.function_spec.SuiteSpec` describing the
        sampling rules and default values.
    dimension:
        The fixed ambient dimension for every function in the suite.

    Notes
    -----
    The suite does not eagerly build every function. It keeps the generated
    blueprint list internally and materializes one function at a time.
    """

    def __init__(self, spec, dimension):
        self._suite = _funccraft.BenchmarkSuite(_as_native_suite_spec(spec), dimension)
        self._spec = SuiteSpec.from_cpp(self._suite.spec)

    @property
    def size(self):
        """Return the number of generated functions currently available."""
        return self._suite.size

    @property
    def dimension(self):
        """Return the fixed ambient dimension of the suite."""
        return self._suite.dimension

    @property
    def spec(self):
        """Return the normalized suite specification."""
        return self._spec

    @property
    def theoretical_max_number_of_functions(self):
        """Return the combinatorial upper bound implied by the spec."""
        return self._suite.theoretical_max_number_of_functions

    @property
    def max_number_of_functions(self):
        """Return the number of lazily generatable functions."""
        return self._suite.max_number_of_functions

    def function(self, index):
        """Materialize one generated function by index.

        This returns a Python :class:`BenchmarkFunction` wrapper built from the
        function's normalized spec.
        """
        return BenchmarkFunction(self._suite.function(index).spec)

    def evaluate(self, index, points):
        """Evaluate one generated function on a batch of candidate points."""
        return self._suite.evaluate(index, points)

    def __call__(self, index, points):
        """Alias for :meth:`evaluate`."""
        return self._suite(index, points)

    def __len__(self):
        return len(self._suite)

    def __repr__(self):
        return (
            "BenchmarkSuite("
            f"size={self.size}, "
            f"dimension={self.dimension})"
        )


__all__ = [
    "BenchmarkSuite",
    "ChoiceSpec",
    "SuiteSpec",
    "make_benchmark_suite",
]
