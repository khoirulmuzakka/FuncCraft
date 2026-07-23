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

import os
from pathlib import Path


_PACKAGE_SUITE_DIR = Path(__file__).resolve().parent / "suites"
if _PACKAGE_SUITE_DIR.is_dir() and not os.environ.get("FUNCCRAFT_SUITE_DIR"):
    os.environ["FUNCCRAFT_SUITE_DIR"] = str(_PACKAGE_SUITE_DIR)


def _as_native_suite_spec(spec):
    return suite_spec(spec)


def make_benchmark_suite(spec, dimension):
    """Build a :class:`BenchmarkSuite` for the requested dimension."""
    return BenchmarkSuite(spec, dimension)


def load_suite_spec(path):
    """Load a native :class:`SuiteSpec` from a file."""
    return _funccraft.load_suite_spec(str(path))


SuiteCollectionId = _funccraft.SuiteCollectionId


def list_suite_collections():
    """Return the built-in suite collections available in this package."""
    return _funccraft.list_suite_collections()


def suite_collection_spec(year, version):
    """Return the native :class:`SuiteSpec` for a built-in collection."""
    return _funccraft.suite_collection_spec(int(year), int(version))


def suite_collection_number_of_functions(year, version):
    """Return the default function count for a built-in collection."""
    return _funccraft.suite_collection_number_of_functions(int(year), int(version))


def suite_collection_number_of_function(year, version):
    """Alias for :func:`suite_collection_number_of_functions`."""
    return suite_collection_number_of_functions(year, version)


def suite_collection(year, version):
    """Return a :class:`SuiteCollection` wrapper for a built-in collection."""
    return SuiteCollection(year, version)


def suite_collection_benchmark_suite(
    year,
    version,
    dimension,
):
    """Build a :class:`BenchmarkSuite` from a built-in suite collection."""
    native = _funccraft.suite_collection_benchmark_suite(
        int(year),
        int(version),
        int(dimension),
    )
    return BenchmarkSuite.from_cpp(native)


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


class SuiteCollection:
    """Built-in, versioned benchmark-suite collection."""

    def __init__(self, year, version):
        self._collection = _funccraft.SuiteCollection(int(year), int(version))

    @property
    def year(self):
        return self._collection.year

    @property
    def version(self):
        return self._collection.version

    @property
    def name(self):
        return self._collection.name

    def number_of_functions(self):
        return self._collection.number_of_functions

    def number_of_function(self):
        return self.number_of_functions()

    def spec(self):
        return self._collection.spec()

    def benchmark_suite(
        self,
        dimension,
    ):
        native = self._collection.benchmark_suite(
            int(dimension),
        )
        return BenchmarkSuite.from_cpp(native)

    def __repr__(self):
        return (
            "SuiteCollection("
            f"year={self.year}, "
            f"version={self.version}, "
            f"name='{self.name}')"
        )


__all__ = [
    "BenchmarkSuite",
    "ChoiceSpec",
    "list_suite_collections",
    "load_suite_spec",
    "suite_collection",
    "suite_collection_benchmark_suite",
    "suite_collection_number_of_function",
    "suite_collection_number_of_functions",
    "suite_collection_spec",
    "SuiteCollection",
    "SuiteCollectionId",
    "SuiteSpec",
    "make_benchmark_suite",
]
