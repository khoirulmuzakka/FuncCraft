"""Public Python API for FuncCraft."""

from pathlib import Path as _Path
import os as _os
from types import SimpleNamespace as _SimpleNamespace

from . import _funccraft
from ._private import function_spec as _function_config
from ._private import suite_spec as _suite_config
from ._private import _spec_to_dict as _config_dict

BasicF = _funccraft.BasicF
BasicFunctionId = _funccraft.BasicFunctionId

_PACKAGE_SUITE_DIR = _Path(__file__).resolve().parent / "suites"
if _PACKAGE_SUITE_DIR.is_dir() and not _os.environ.get("FUNCCRAFT_SUITE_DIR"):
    _os.environ["FUNCCRAFT_SUITE_DIR"] = str(_PACKAGE_SUITE_DIR)


def _as_native_function_config(config):
    return _function_config(config)


def _as_native_suite_config(config):
    return _suite_config(config)


class BenchmarkFunction:
    """Concrete benchmark function.

    Build one function from a YAML-shaped dictionary, a YAML file path, or a
    native function returned by :meth:`BenchmarkSuite.function`. The
    recommended workflow is to instantiate the function first, then evaluate
    batches with :meth:`evaluate`.

    Parameters
    ----------
    config
        One of:

        - a dictionary with the same structure as a function YAML file;
        - a path to an exported function YAML file;
        - an internal native function object returned by ``BenchmarkSuite``.

    Function dictionary fields
    --------------------------
    ``dimension``
        Ambient dimension of the function.
    ``domain``
        Dictionary with ``dimension``, ``lower_bound``, and ``upper_bound``.
        Bounds may be scalars or per-coordinate lists.
    ``components``
        List of component dictionaries. Each component must provide either
        ``base_function`` or ``composed_function``. ``base_function`` accepts
        a primitive ID, primitive name, or ``BasicFunctionId`` value.
        ``composed_function`` is another function dictionary, allowing nested
        composed functions up to the configured nested depth in suite
        generation.
    ``coordinate_transform``
        Component transform dictionary. Available ``kind`` values are
        ``none``, ``rotation``, ``affine``, and ``subspace-rotation``. Common fields are
        ``input_dimension``, ``output_dimension``, ``assigned_xopt``,
        ``seed``, ``parameters``, ``matrix``, ``offset``, and
        ``selected_indices``. Generated/exported YAML records may also contain
        resolved transform data.
    ``value_transform``
        Scalar value transform dictionary. Available ``kind`` values are
        ``none``, ``power``, ``osc``/``oscillatory``, and ``cosine-zero``.
        Optional ``parameters`` control the transform family.
    component ``scale_factor``
        Optional positive multiplier applied to the transformed component
        value before composition. Omit it to let FuncCraft estimate one.
    ``composition``
        Composition dictionary. Available ``kind`` values are ``none``,
        ``cpmsum``/``cpm-wsum``, ``cpmpmean``/``cpm-power-mean``,
        ``cpmlwell``/``cpm-level-well``, ``dpmsoftmax``/``dpm-softmax``, and
        ``dpmbgsoftmax``/``dpm-bgsoftmax``. Optional fields include
        ``parameters``, ``biases``, and ``centers``.
    ``assigned_xopt`` and ``assigned_fopt``
        Desired optimum location and optimum value.
    ``scale_factor``
        Positive final scale factor. Omit it to let FuncCraft estimate one.
    ``seed``
        Optional generation seed for generated missing details.
    ``label`` and ``metadata``
        Optional bookkeeping fields stored with exported YAML.

    Notes
    -----
    Name parsing is permissive for mechanism names: case, spaces, hyphens,
    and underscores are normalized before matching.

    Examples
    --------
    >>> import funccraft as fc
    >>> f = fc.SuiteCollection(2026, 1).benchmark_suite(10).function(1)
    >>> values = f.evaluate([[0.0] * 10, [1.0] * 10])
    """

    def __init__(self, config):
        if isinstance(config, _funccraft.BenchmarkFunction):
            self._function = config
            self._native_record = getattr(self._function, "spec")
        elif isinstance(config, str):
            self._function = _funccraft.make_benchmark_function(config)
            self._native_record = getattr(self._function, "spec")
        else:
            self._function = _funccraft.BenchmarkFunction(_as_native_function_config(config))
            self._native_record = getattr(self._function, "spec")

    @property
    def domain(self):
        """Return the search domain.

        The returned object has ``dimension``, ``lower_bound``, and
        ``upper_bound`` attributes.
        """
        return _SimpleNamespace(
            dimension=self._function.domain.dimension,
            lower_bound=list(self._function.domain.lower),
            upper_bound=list(self._function.domain.upper),
        )

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

    def get_fopt(self):
        """Return the assigned optimum value."""
        return self._function.get_fopt()

    def get_xopt(self):
        """Return the assigned optimum location."""
        return list(self._function.get_xopt())

    @property
    def component_types(self):
        """Return a compact summary of immediate component source types."""
        return self._function.component_types

    @property
    def label(self):
        """Return the human-readable function label."""
        return self._function.label

    def __call__(self, points):
        """Evaluate a batch of candidate points.

        ``points`` must be a sequence of points, where each point has length
        ``dimension``. The return value has one function value per point.
        """
        return self._function(points)

    def evaluate(self, points):
        """Evaluate a batch of candidate points.

        This is the preferred spelling for Python examples:

        >>> f = suite.function(1)
        >>> values = f.evaluate([[0.0] * suite.dimension])
        """
        return self._function.evaluate(points)

    def export_yaml(self, path):
        """Write the materialized function YAML record."""
        self._function.export_spec(str(path))

    def __repr__(self):
        return (
            "BenchmarkFunction("
            f"dimension={self.dimension}, "
            f"label='{self.label}', "
            f"component_types='{self.component_types}', "
            f"fopt={self.get_fopt()}, "
            f"scale_factor={self.scale_factor})"
        )


class BenchmarkSuite:
    """Concrete suite of generated benchmark functions.

    A suite is built from a YAML-shaped suite dictionary or a YAML file, plus
    an explicit evaluation dimension. Retrieve individual
    :class:`BenchmarkFunction` objects with :meth:`function`.

    Parameters
    ----------
    config
        Dictionary matching a suite YAML file, or a path to a suite YAML or
        exported suite manifest.
    dimension
        Positive integer ambient dimension for every generated function in
        the suite.

    Suite dictionary fields
    -----------------------
    ``supported_dimensions``
        ``"any"`` or a comma-separated/list representation of supported
        dimensions.
    ``base_functions``
        Primitive functions included as mandatory single-function benchmarks.
        Entries may be primitive IDs, names, or ``BasicFunctionId`` values.
    ``composition_base_functions``
        Primitive pool used inside composed and nested composed functions.
    ``coordinate_transforms``
        Choice table with entries containing ``kind``, ``probability``, and
        optional ``parameters``. Available kinds are ``none``, ``rotation``,
        ``affine``, and ``subspace-rotation``.
    ``value_transforms``
        Choice table with entries containing ``kind``, ``probability``, and
        optional ``parameters``. Available kinds are ``none``, ``power``,
        ``osc``/``oscillatory``, and ``cosine-zero``.
    ``compositions``
        Choice table with entries containing ``kind``, ``probability``, and
        optional ``parameters``. Available kinds are ``cpmsum``/``cpm-wsum``,
        ``cpmpmean``/``cpm-power-mean``, ``cpmlwell``/``cpm-level-well``,
        ``dpmsoftmax``/``dpm-softmax``, and
        ``dpmbgsoftmax``/``dpm-bgsoftmax``. ``none`` is also accepted for
        direct single-component composition.
    ``min_components`` and ``max_components``
        Inclusive range for the number of components in generated composed
        functions.
    ``max_nested_composition_depth``
        Maximum depth for nested composed components. ``0`` disables nested
        composed components.
    ``nested_probability``
        Probability that an eligible component is itself a nested composed
        function.
    ``requested_number_of_functions``
        Number of generated functions requested from the suite.
    ``master_seed``
        Seed controlling deterministic suite generation.
    ``lower_bound`` and ``upper_bound``
        Search-domain bounds. Values may be scalars or per-coordinate lists.
    ``assigned_fopt``
        Assigned optimum value for generated functions.
    ``xopt_domain_shrink_factor``
        Fraction of the domain used for generated optimum locations and DPM
        centers.
    ``suite_label``
        Human-readable label stored with generated YAML records.

    Notes
    -----
    Set a choice-table probability to ``0`` to keep an option visible in a
    YAML file while disabling it. Function indices are one-based: valid
    indices are ``1`` through ``suite.size``.

    Examples
    --------
    >>> import funccraft as fc
    >>> suite = fc.SuiteCollection(2026, 1).benchmark_suite(10)
    >>> f = suite.function(1)
    >>> values = f.evaluate([[0.0] * 10, [1.0] * 10])
    """

    def __init__(self, config, dimension):
        if isinstance(config, str):
            self._suite = _funccraft.BenchmarkSuite(config, dimension)
        else:
            self._suite = _funccraft.BenchmarkSuite(_as_native_suite_config(config), dimension)
        self._native_record = getattr(self._suite, "spec")

    @classmethod
    def from_cpp(cls, native):
        """Wrap an already-built native benchmark suite."""
        obj = cls.__new__(cls)
        obj._suite = native
        obj._native_record = getattr(native, "spec")
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
    def theoretical_max_number_of_functions(self):
        """Return the top-level combinatorial capacity implied by the YAML."""
        return self._suite.theoretical_max_number_of_functions

    @property
    def config(self):
        """Return this suite recipe as a plain configuration dictionary.

        The returned dictionary can be edited and passed to
        :class:`BenchmarkSuite` to build a related suite. For example, reduce
        ``requested_number_of_functions`` before exporting a smaller manifest.
        """
        return _config_dict(self._native_record)

    def function(self, index):
        """Materialize one generated function by one-based index.

        ``index=1`` returns F1. ``0`` and negative indices are invalid.
        """
        return BenchmarkFunction(self._suite.function(index))

    def export_manifest(self, path):
        """Write the materialized suite YAML manifest."""
        self._suite.export_manifest(str(path))

    def __len__(self):
        return len(self._suite)

    def __repr__(self):
        return (
            "BenchmarkSuite("
            f"size={self.size}, "
            f"dimension={self.dimension})"
        )


class SuiteCollection:
    """Packaged benchmark suite collection.

    Instantiate this with a collection year and version, then materialize a
    :class:`BenchmarkSuite` at an explicit dimension. The packaged
    ``2026_v1`` suite is defined by ``suites/2026_v1.yaml`` and exposes the
    same mechanisms available to custom suite dictionaries: all 34 primitive
    base functions, coordinate transforms, value transforms, composition
    rules, optional nested composed functions, deterministic generation seeds,
    bounds, assigned optimum values, and suite labels.

    Parameters
    ----------
    year
        Published suite year, for example ``2026``.
    version
        Published suite version within that year, for example ``1``.

    Examples
    --------
    >>> import funccraft as fc
    >>> collection = fc.SuiteCollection(2026, 1)
    >>> suite = collection.benchmark_suite(10)
    >>> f45 = suite.function(45)
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

    @property
    def config(self):
        """Return the packaged suite recipe as a plain configuration dictionary.

        The returned dictionary is independent of the collection object. Edit
        it and pass it to :class:`BenchmarkSuite` when you want a smaller or
        otherwise modified suite recipe.
        """
        return _config_dict(self._collection.spec())

    def benchmark_suite(self, dimension):
        """Build a runtime :class:`BenchmarkSuite` at ``dimension``.

        The dimension must be explicit so the same collection can be
        materialized at different dimensions reproducibly.
        """
        native = self._collection.benchmark_suite(int(dimension))
        return BenchmarkSuite.from_cpp(native)

    def __repr__(self):
        return (
            "SuiteCollection("
            f"year={self.year}, "
            f"version={self.version}, "
            f"name='{self.name}')"
        )


def listSuiteCollections():
    """Return the built-in suite collections available in this package.

    Each item describes a packaged collection that can be loaded with
    :class:`SuiteCollection`.
    """
    return _funccraft.list_suite_collections()


__all__ = [
    "BasicF",
    "BasicFunctionId",
    "BenchmarkFunction",
    "BenchmarkSuite",
    "SuiteCollection",
    "listSuiteCollections",
]
