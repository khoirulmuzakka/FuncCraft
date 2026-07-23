"""Runtime benchmark-function wrapper for FuncCraft.

Use :class:`BenchmarkFunction` when you already have a full
``FunctionSpec`` and want a callable object with the derived runtime pieces
materialized. It is the high-level object for evaluating one concrete
benchmark instance.

Examples
--------
Build and evaluate one function. FuncCraft evaluation is batched, so pass a
list of candidate vectors even when evaluating a single point::

    from funccraft import (
        BenchmarkFunction,
        make_component,
        make_coordinate_transform,
        make_domain,
        make_function_spec,
    )

    spec = make_function_spec(
        dimension=2,
        domain=make_domain(2, -10.0, 10.0),
        components=[
            make_component(
                base_function="Rastrigin",
                coordinate_transform=make_coordinate_transform(
                    "rotation",
                    seed=123,
                ),
            ),
        ],
        assigned_xopt=[3.0, -2.0],
        assigned_fopt=10.0,
        scale_factor=1.0,
        seed=7,
    )

    f = BenchmarkFunction(spec)
    values = f([[0.0, 0.0], [3.0, -2.0]])
    f.export_spec("rastrigin_2d.yaml")

The exported YAML contains the materialized matrix, assigned optimum, scale
factor, and other parameters needed to rebuild the same function later.
"""

from . import _funccraft
from .spec import DomainSpec, FunctionSpec, function_spec


def _as_native_function_spec(spec):
    return function_spec(spec)


def make_benchmark_function(spec):
    """Build a :class:`BenchmarkFunction` from a spec object, dict, or file path.

    This is a convenience alias for ``BenchmarkFunction(spec)``.

    Examples
    --------
    ``spec`` may be a native ``FunctionSpec``, a compatible dictionary, a YAML
    path, or an already-created native C++ benchmark function::

        f = make_benchmark_function("rastrigin_2d.yaml")
        y = f([[0.0, 0.0]])
    """
    return BenchmarkFunction(spec)


def load_function_spec(path):
    """Load a native :class:`FunctionSpec` from a YAML file.

    Examples
    --------
    Load the spec first when you want to inspect or modify it before runtime
    construction::

        spec = load_function_spec("rastrigin_2d.yaml")
        print(spec.dimension, spec.assigned_xopt)
        f = BenchmarkFunction(spec)
    """
    return _funccraft.load_function_spec(str(path))


class BenchmarkFunction:
    """Concrete benchmark function.

    Parameters
    ----------
    spec:
        A :class:`~funccraft.spec.FunctionSpec`, compatible dictionary, YAML
        path, native C++ ``BenchmarkFunction``, or object convertible by
        :func:`funccraft.spec.function_spec`.

    Notes
    -----
    This wrapper delegates all heavy lifting to the compiled extension. It is
    intentionally small, but the attribute names mirror the structure of the
    underlying spec so the object can be inspected without reading C++.

    The callable interface is batched: ``f([[x0, x1]])`` returns a list with
    one value. A bare vector such as ``f([x0, x1])`` is intentionally not the
    public interface because suites and optimizers usually evaluate batches.

    Examples
    --------
    Construct from a dictionary instead of using the helper classes::

        f = BenchmarkFunction({
            "dimension": 2,
            "domain": {"dimension": 2, "lower_bound": [-5, -5], "upper_bound": [5, 5]},
            "components": [
                {
                    "base_function": "Sphere",
                    "coordinate_transform": {
                        "kind": "none",
                        "assigned_xopt": [1.0, -1.0],
                    },
                    "value_transform": {"kind": "none"},
                },
            ],
            "composition": {"kind": "none"},
            "assigned_xopt": [1.0, -1.0],
            "assigned_fopt": 0.0,
            "scale_factor": 1.0,
        })

        print(f.dimension)
        print(f.assigned_fopt)
        print(f([[1.0, -1.0], [0.0, 0.0]]))
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

        Returns
        -------
        list[float]
            One objective value per input vector.

        Examples
        --------
        ``points`` is a list of points, not a single point::

            values = f([[0.0] * f.dimension])
        """
        return self._function(points)

    def evaluate(self, points):
        """Evaluate a batch of candidate points.

        This is the named equivalent of ``f(points)`` and is useful when
        passing a method reference to optimizers.
        """
        return self._function.evaluate(points)

    def export_spec(self, path):
        """Write the complete function reproducibility spec to a YAML file.

        Examples
        --------
        Round-trip a function through YAML::

            before = f([[0.25, -0.5]])
            f.export_spec("function.yaml")
            g = BenchmarkFunction("function.yaml")
            after = g([[0.25, -0.5]])
            assert before == after
        """
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
