"""Runtime benchmark-suite wrapper for FuncCraft.

``BenchmarkSuite`` is the object you use when you want a family of benchmark
functions sampled from a :class:`~funccraft.spec.SuiteSpec`.
It stores the generated blueprints lazily and materializes a concrete
:class:`~funccraft.benchmark_function.BenchmarkFunction` only when you ask
for a specific index.

Examples
--------
Use the built-in 2026 v1 collection when you want the standard FuncCraft
suite definition shipped with the package::

    from funccraft import suite_collection

    year = 2026
    version = 1
    collection = suite_collection(year, version)
    print(collection.name, collection.number_of_functions)

    suite = collection.benchmark_suite(dimension=10)
    f1 = suite.function(1)
    values = f1.evaluate([[0.0] * suite.dimension])

Build a custom suite from Python specs when you want to control the sampling
rules directly::

    from funccraft import (
        BenchmarkSuite,
        make_composition_choice,
        make_suite_spec,
    )

    spec = make_suite_spec(
        requested_number_of_functions=100,
        max_components=4,
        master_seed=1,
        compositions=[
            make_composition_choice("cpm-wsum", 0.5),
            make_composition_choice("dpm-softmax", 0.5, [0.01]),
        ],
    )
    suite = BenchmarkSuite(spec, dimension=5)
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
    """Build a :class:`BenchmarkSuite` for the requested dimension.

    Parameters
    ----------
    spec:
        One of:

        - native :class:`SuiteSpec`;
        - compatible dictionary with the ``make_suite_spec`` fields:
          ``supported_dimensions``, ``base_functions``,
          ``composition_base_functions``, ``coordinate_transforms``,
          ``value_transforms``, ``compositions``, ``min_components``,
          ``max_components``, ``max_nested_composition_depth``,
          ``nested_probability``, ``requested_number_of_functions``,
          ``max_number_of_functions``, ``master_seed``, ``lower_bound``,
          ``upper_bound``, ``assigned_fopt``,
          ``xopt_domain_shrink_factor``, and ``suite_label``;
        - YAML path containing those same fields.
    dimension:
        Explicit runtime dimension for every generated benchmark function.

    Examples
    --------
    ``spec`` may be a native ``SuiteSpec``, compatible dictionary, or YAML
    path::

        suite = make_benchmark_suite("default_suite.yaml", dimension=10)
        f = suite.function(1)
        y = f.evaluate([[0.0] * suite.dimension])

    Dictionary input can expose all suite options directly::

        suite = make_benchmark_suite(
            {
                "supported_dimensions": "any",
                "base_functions": list(range(1, 35)),
                "composition_base_functions": [
                    4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20,
                    21, 23, 28, 30, 34, 33, 2, 3, 5, 9, 26, 27,
                ],
                "coordinate_transforms": [
                    {"kind": "none", "probability": 0.0, "parameters": []},
                    {"kind": "rotation", "probability": 0.34, "parameters": []},
                    {"kind": "affine", "probability": 0.33, "parameters": []},
                    {"kind": "block-rotation", "probability": 0.33, "parameters": []},
                ],
                "value_transforms": [
                    {"kind": "none", "probability": 0.5, "parameters": []},
                    {"kind": "power", "probability": 0.25, "parameters": []},
                    {"kind": "oscillatory", "probability": 0.25, "parameters": []},
                    {"kind": "cosine-zero", "probability": 0.0, "parameters": []},
                ],
                "compositions": [
                    {"kind": "cpm-wsum", "probability": 0.1, "parameters": []},
                    {"kind": "cpm-power-mean", "probability": 0.1, "parameters": [3.0]},
                    {"kind": "cpm-power-mean", "probability": 0.1, "parameters": [0.1]},
                    {"kind": "cpm-level-well", "probability": 0.2, "parameters": []},
                    {"kind": "dpm-softmax", "probability": 0.25, "parameters": [0.005]},
                    {"kind": "dpm-bgsoftmax", "probability": 0.25, "parameters": [0.005, 1.0, 0.01]},
                ],
                "min_components": 2,
                "max_components": 5,
                "max_nested_composition_depth": 1,
                "nested_probability": 0.1,
                "requested_number_of_functions": 1_000_000,
                "max_number_of_functions": 0,
                "master_seed": 1,
                "lower_bound": -100.0,
                "upper_bound": 100.0,
                "assigned_fopt": 100.0,
                "xopt_domain_shrink_factor": 0.8,
                "suite_label": "Funccraft-2026-benchmark-suite-v1",
            },
            dimension=10,
        )
    """
    return BenchmarkSuite(spec, dimension)


def load_suite_spec(path):
    """Load a native :class:`SuiteSpec` from a YAML file.

    Examples
    --------
    Load a suite definition, inspect it, then build a runtime suite::

        spec = load_suite_spec("suites/2026_v1.yaml")
        print(spec.requested_number_of_functions)
        suite = BenchmarkSuite(spec, dimension=10)
    """
    return _funccraft.load_suite_spec(str(path))


SuiteCollectionId = _funccraft.SuiteCollectionId


def list_suite_collections():
    """Return the built-in suite collections available in this package.

    Examples
    --------
    Each item identifies a versioned collection such as 2026 v1::

        for item in list_suite_collections():
            print(item.year, item.version, item.name)
    """
    return _funccraft.list_suite_collections()


def suite_collection_spec(year, version):
    """Return the native :class:`SuiteSpec` for a built-in collection."""
    return _funccraft.suite_collection_spec(int(year), int(version))


def suite_collection_number_of_functions(year, version):
    """Return the default function count for a built-in collection."""
    return _funccraft.suite_collection_number_of_functions(int(year), int(version))


def suite_collection(year, version):
    """Return a :class:`SuiteCollection` wrapper for a built-in collection.

    Examples
    --------
    Build a standard suite at any supported dimension::

        year = 2026
        version = 1
        collection = suite_collection(year, version)
        suite_2d = collection.benchmark_suite(2)
        suite_20d = collection.benchmark_suite(20)
    """
    return SuiteCollection(year, version)


def suite_collection_benchmark_suite(
    year,
    version,
    dimension,
):
    """Build a :class:`BenchmarkSuite` from a built-in suite collection.

    This is the functional shortcut for
    ``suite_collection(year, version).benchmark_suite(dimension)``.
    """
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
        A :class:`~funccraft.spec.SuiteSpec`, compatible dictionary, or YAML
        path describing the sampling rules and default values.
    dimension:
        The fixed ambient dimension for every function in the suite.

    Notes
    -----
    The suite does not eagerly build every function. It keeps the generated
    blueprint list internally and materializes one function at a time.

    The recommended workflow is to materialize a
    :class:`BenchmarkFunction` first with ``suite.function(i)``, then evaluate
    that function on a batch of points.

    Examples
    --------
    Evaluate and export the generated suite manifest::

        suite = BenchmarkSuite("suites/2026_v1.yaml", dimension=10)
        f10 = suite.function(10)
        y = f10.evaluate([[0.0] * 10, list(f10.spec.assigned_xopt)])
        suite.export_manifest("suite_10d_manifest.yaml")
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
        """Materialize one generated function by one-based index.

        This returns a Python :class:`BenchmarkFunction` wrapper built from the
        function's normalized spec.

        Examples
        --------
        ``BenchmarkFunction`` exposes the materialized single-function spec::

            f = suite.function(42)
            print(f.spec.label)
            print(f.get_xopt())
        """
        return BenchmarkFunction(self._suite.function(index))

    def evaluate(self, index, points):
        """Shortcut for evaluating one generated function by index.

        Prefer materializing the benchmark function first when writing new
        code::

            f = suite.function(index)
            values = f.evaluate(points)

        Parameters
        ----------
        index:
            One-based function index.
        points:
            List of vectors with length equal to ``suite.dimension``.
        """
        return self._suite.evaluate(index, points)

    def __call__(self, index, points):
        """Alias for :meth:`evaluate`.

        Prefer ``suite.function(index).evaluate(points)`` in new code.
        """
        return self._suite(index, points)

    def export_manifest(self, path):
        """Write the suite spec and all generated function specs to a YAML file.

        The manifest is a materialized record of the generated suite: it
        contains the suite-level spec and every generated function spec.
        """
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
    """Built-in, versioned benchmark-suite collection.

    A collection is a named suite definition stored in the package ``suites``
    directory, for example ``2026_v1.yaml``. The YAML controls how many
    functions are in the collection; you choose the dimension when building a
    runtime :class:`BenchmarkSuite`.

    Examples
    --------
    Create the standard 2026 v1 suite at two and ten dimensions::

        year = 2026
        version = 1
        collection = SuiteCollection(year, version)
        print(collection.name)
        print(collection.number_of_functions)

        suite_2d = collection.benchmark_suite(2)
        suite_10d = collection.benchmark_suite(10)
        f = suite_10d.function(1)
        y = f.evaluate([[0.0] * 10])

    Get the underlying suite spec when you want to inspect the YAML-derived
    defaults before constructing a runtime suite::

        spec = collection.spec()
        print(spec.max_components, spec.master_seed)
    """

    def __init__(self, year, version):
        self._collection = _funccraft.SuiteCollection(int(year), int(version))

    @property
    def year(self):
        """Collection year, for example ``2026``."""
        return self._collection.year

    @property
    def version(self):
        """Collection version within the year, for example ``1``."""
        return self._collection.version

    @property
    def name(self):
        """Human-readable collection name from the YAML file."""
        return self._collection.name

    @property
    def number_of_functions(self):
        """Number of functions defined by the collection YAML."""
        return self._collection.number_of_functions

    def spec(self):
        """Return the native :class:`SuiteSpec` for this collection."""
        return self._collection.spec()

    def benchmark_suite(
        self,
        dimension,
    ):
        """Build a runtime :class:`BenchmarkSuite` at ``dimension``."""
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
    "suite_collection_number_of_functions",
    "suite_collection_spec",
    "SuiteCollection",
    "SuiteCollectionId",
    "SuiteSpec",
    "make_benchmark_suite",
]
