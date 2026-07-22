"""Runtime benchmark-suite wrapper for FuncCraft.

`BenchmarkSuite` is the object you use when you want a family of benchmark
functions sampled from a :class:`~funccraft.spec.SuiteSpec`.
It stores the generated blueprints lazily and materializes a concrete
:class:`~funccraft.benchmark_function.BenchmarkFunction` only when you ask
for a specific index.
"""

from . import _funccraft
from .benchmark_function import BenchmarkFunction
from .spec import ChoiceSpec, SuiteSpec, suite_spec


def _as_native_suite_spec(spec):
    return suite_spec(spec)


def make_benchmark_suite(spec, dimension):
    """Build a :class:`BenchmarkSuite` for the requested dimension."""
    return BenchmarkSuite(spec, dimension)


def load_suite_spec_yaml(path):
    """Load a native :class:`SuiteSpec` from a YAML file."""
    return _funccraft.load_suite_spec_yaml(str(path))


def make_benchmark_suite_from_yaml(path, dimension):
    """Build a :class:`BenchmarkSuite` directly from a YAML file."""
    return BenchmarkSuite(path, dimension)


class BenchmarkSuite:
    """Concrete suite of generated benchmark functions.

    Parameters
    ----------
    spec:
        A :class:`~funccraft.spec.SuiteSpec` describing the
        sampling rules and default values.
    dimension:
        The fixed ambient dimension for every function in the suite.

    Notes
    -----
    The suite does not eagerly build every function. It keeps the generated
    blueprint list internally and materializes one function at a time.
    """

    def __init__(self, spec, dimension):
        if isinstance(spec, str):
            self._suite = _funccraft.BenchmarkSuite(spec, dimension)
        else:
            self._suite = _funccraft.BenchmarkSuite(_as_native_suite_spec(spec), dimension)
        self._spec = self._suite.spec

    @classmethod
    def from_cpp(cls, native):
        """Wrap an already-built native benchmark suite."""
        obj = cls.__new__(cls)
        obj._suite = native
        obj._spec = native.spec
        return obj

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
        return BenchmarkFunction(self._suite.function(index))

    def evaluate(self, index, points):
        """Evaluate one generated function on a batch of candidate points."""
        return self._suite.evaluate(index, points)

    def __call__(self, index, points):
        """Alias for :meth:`evaluate`."""
        return self._suite(index, points)

    def export_manifest(self, path):
        """Write the suite spec and all generated function specs to a YAML file."""
        self._suite.export_manifest(str(path))

    def export_spec(self, path):
        """Alias for :meth:`export_manifest`."""
        self._suite.export_spec(str(path))

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
    "load_suite_spec_yaml",
    "SuiteSpec",
    "make_benchmark_suite",
    "make_benchmark_suite_from_yaml",
]
