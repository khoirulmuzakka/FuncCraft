"""Dict-like Python specification models for FuncCraft.

The spec layer is intentionally shaped like nested dictionaries. That means:

- you can inspect it with plain Python mapping tools,
- you can serialize it later without custom adapters,
- and you only convert to the native C++ types at the boundary.

Each spec object behaves like a mutable mapping and also supports attribute
access for convenience. For example:

>>> spec = FunctionSpec()
>>> spec.dimension = 10
>>> spec["dimension"]
10
>>> spec.to_dict()
{"dimension": 10, ...}

The classes are:

- `ChoiceSpec`: weighted family choice used by suite generation.
- `TransformSpec`: coordinate transform description for one component.
- `ValueTransformSpec`: scalar transform description for one component.
- `ComponentSpec`: one primitive + one coordinate transform + one value transform.
- `CompositionSpec`: composition rule for component values.
- `FunctionSpec`: full description of one benchmark function.
- `SuiteSpec`: declarative description of a generated benchmark suite.
"""

from __future__ import annotations

from collections.abc import MutableMapping
from copy import deepcopy

from . import _funccraft


def _copy_list(values):
    return list(values) if values is not None else []


def _default_base_functions():
    return list(range(43))


def _default_composition_base_functions():
    return [0, 2, 4, 8, 9, 10, 11, 12, 15, 16, 19, 20, 21, 22, 23]


def _default_base_function_coord_transforms():
    return [
        ChoiceSpec("rotation", 0.3),
        ChoiceSpec("affine", 0.7),
    ]


def _default_coord_transforms():
    return [
        ChoiceSpec("rotation", 0.3),
        ChoiceSpec("affine", 0.3),
        ChoiceSpec("blockrotation", 0.4),
    ]


def _default_value_transforms():
    return [
        ChoiceSpec("osc", 0.8),
        ChoiceSpec("power", 0.2),
    ]


def _default_composition_functions():
    return [
        ChoiceSpec("cpmlwell", 0.25),
        ChoiceSpec("cpmsum", 0.25),
        ChoiceSpec("dpmsoftmax", 0.5, [0.01, 1.0, 0.01]),
    ]


def _deep_to_plain(value):
    if isinstance(value, SpecMapping):
        return value.to_dict()
    if isinstance(value, list):
        return [_deep_to_plain(item) for item in value]
    if isinstance(value, tuple):
        return [_deep_to_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _deep_to_plain(item) for key, item in value.items()}
    return deepcopy(value)


def _get_mapping(value):
    if isinstance(value, SpecMapping):
        return value
    if isinstance(value, dict):
        return value
    return value


def _as_choice_spec(value):
    if isinstance(value, ChoiceSpec):
        return value
    if hasattr(value, "kind") and hasattr(value, "probability"):
        return ChoiceSpec.from_cpp(value)
    if isinstance(value, dict):
        return ChoiceSpec.from_dict(value)
    raise TypeError("expected ChoiceSpec-like value")


def _as_transform_spec(value):
    if isinstance(value, TransformSpec):
        return value
    if hasattr(value, "kind") and hasattr(value, "dimension"):
        return TransformSpec.from_cpp(value)
    if isinstance(value, dict):
        return TransformSpec.from_dict(value)
    raise TypeError("expected TransformSpec-like value")


def _as_value_transform_spec(value):
    if isinstance(value, ValueTransformSpec):
        return value
    if hasattr(value, "kind") and hasattr(value, "seed"):
        return ValueTransformSpec.from_cpp(value)
    if isinstance(value, dict):
        return ValueTransformSpec.from_dict(value)
    raise TypeError("expected ValueTransformSpec-like value")


def _as_component_spec(value):
    if isinstance(value, ComponentSpec):
        return value
    if hasattr(value, "base_function") and hasattr(value, "coordinate_transform"):
        return ComponentSpec.from_cpp(value)
    if isinstance(value, dict):
        return ComponentSpec.from_dict(value)
    raise TypeError("expected ComponentSpec-like value")


def _as_composition_spec(value):
    if isinstance(value, CompositionSpec):
        return value
    if hasattr(value, "kind") and hasattr(value, "centers"):
        return CompositionSpec.from_cpp(value)
    if isinstance(value, dict):
        return CompositionSpec.from_dict(value)
    raise TypeError("expected CompositionSpec-like value")


class SpecMapping(MutableMapping):
    """Base class for dict-like spec objects."""

    _fields = ()

    def __init__(self, **kwargs):
        object.__setattr__(self, "_data", {})
        for name, default in self._fields:
            self._data[name] = self._clone_default(default)
        for key, value in kwargs.items():
            self[key] = value

    @staticmethod
    def _clone_default(value):
        if isinstance(value, SpecMapping):
            return value.copy()
        if isinstance(value, list):
            return [_deep_to_plain(item) for item in value]
        if isinstance(value, dict):
            return {key: _deep_to_plain(item) for key, item in value.items()}
        return deepcopy(value)

    def __getitem__(self, key):
        return self._data[key]

    def __setitem__(self, key, value):
        if key not in self._field_names():
            raise KeyError(key)
        self._data[key] = value

    def __delitem__(self, key):
        raise TypeError("spec fields cannot be deleted")

    def __iter__(self):
        return iter(self._data)

    def __len__(self):
        return len(self._data)

    def __getattr__(self, name):
        try:
            return self._data[name]
        except KeyError as exc:
            raise AttributeError(name) from exc

    def __setattr__(self, name, value):
        if name.startswith("_"):
            object.__setattr__(self, name, value)
            return
        if name in self._field_names():
            self._data[name] = value
            return
        object.__setattr__(self, name, value)

    def __repr__(self):
        return f"{self.__class__.__name__}({self.to_dict()!r})"

    @classmethod
    def _field_names(cls):
        return {name for name, _ in cls._fields}

    def copy(self):
        return self.__class__.from_dict(self.to_dict())

    def to_dict(self):
        return {key: _deep_to_plain(value) for key, value in self._data.items()}

    @classmethod
    def from_dict(cls, data):
        obj = cls()
        for key, value in data.items():
            obj[key] = value
        return obj


class Domain(SpecMapping):
    """Axis-aligned search domain."""

    _fields = (
        ("dimension", 0),
        ("lower_bound", []),
        ("upper_bound", []),
    )

    def __init__(self, dimension=0, lower_bound=None, upper_bound=None):
        super().__init__()
        self.dimension = dimension
        self.lower_bound = _copy_list(lower_bound)
        self.upper_bound = _copy_list(upper_bound)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            dimension=int(native.dimension),
            lower_bound=_copy_list(native.lower),
            upper_bound=_copy_list(native.upper),
        )

    def to_cpp(self):
        dimension = int(self.dimension)
        if dimension <= 0:
            dimension = len(self.lower_bound) or len(self.upper_bound)
        lower = _copy_list(self.lower_bound)
        upper = _copy_list(self.upper_bound)
        if lower and len(lower) != dimension:
            raise ValueError("lower_bound dimension mismatch")
        if upper and len(upper) != dimension:
            raise ValueError("upper_bound dimension mismatch")
        lower_fill = lower[0] if lower else -100.0
        upper_fill = upper[0] if upper else 100.0
        native = _funccraft.Domain(dimension, lower_fill, upper_fill)
        native.lower = lower if lower else [lower_fill] * dimension
        native.upper = upper if upper else [upper_fill] * dimension
        return native


class ChoiceSpec(SpecMapping):
    """Weighted choice used by suite generation."""

    _fields = (
        ("kind", ""),
        ("probability", 1.0),
        ("parameters", []),
    )

    def __init__(self, kind="", probability=1.0, parameters=None):
        super().__init__()
        self.kind = kind
        self.probability = probability
        self.parameters = _copy_list(parameters)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            kind=str(native.kind),
            probability=float(native.probability),
            parameters=_copy_list(native.parameters),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            kind=data.get("kind", ""),
            probability=data.get("probability", 1.0),
            parameters=_copy_list(data.get("parameters", [])),
        )

    def to_cpp(self):
        native = _funccraft.ChoiceSpec()
        native.kind = self.kind
        native.probability = self.probability
        native.parameters = _copy_list(self.parameters)
        return native


class TransformSpec(SpecMapping):
    """Coordinate-transform specification for one component."""

    _fields = (
        ("kind", ""),
        ("dimension", 0),
        ("seed", 0),
        ("selected_indices", []),
        ("source_point", []),
        ("target_point", []),
        ("parameters", []),
    )

    def __init__(self, kind="", dimension=0, seed=0, selected_indices=None, source_point=None, target_point=None, parameters=None):
        super().__init__()
        self.kind = kind
        self.dimension = dimension
        self.seed = seed
        self.selected_indices = _copy_list(selected_indices)
        self.source_point = _copy_list(source_point)
        self.target_point = _copy_list(target_point)
        self.parameters = _copy_list(parameters)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            kind=str(native.kind),
            dimension=int(native.dimension),
            seed=int(native.seed),
            selected_indices=_copy_list(native.selected_indices),
            source_point=_copy_list(native.source_point),
            target_point=_copy_list(native.target_point),
            parameters=_copy_list(native.parameters),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            kind=data.get("kind", ""),
            dimension=data.get("dimension", 0),
            seed=data.get("seed", 0),
            selected_indices=_copy_list(data.get("selected_indices", [])),
            source_point=_copy_list(data.get("source_point", [])),
            target_point=_copy_list(data.get("target_point", [])),
            parameters=_copy_list(data.get("parameters", [])),
        )

    def to_cpp(self):
        native = _funccraft.TransformSpec()
        native.kind = self.kind
        native.dimension = self.dimension
        native.seed = self.seed
        native.selected_indices = _copy_list(self.selected_indices)
        native.source_point = _copy_list(self.source_point)
        native.target_point = _copy_list(self.target_point)
        native.parameters = _copy_list(self.parameters)
        return native


class ValueTransformSpec(SpecMapping):
    """Scalar value-transform specification."""

    _fields = (
        ("kind", ""),
        ("seed", 0),
        ("parameters", []),
    )

    def __init__(self, kind="", seed=0, parameters=None):
        super().__init__()
        self.kind = kind
        self.seed = seed
        self.parameters = _copy_list(parameters)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            kind=str(native.kind),
            seed=int(native.seed),
            parameters=_copy_list(native.parameters),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            kind=data.get("kind", ""),
            seed=data.get("seed", 0),
            parameters=_copy_list(data.get("parameters", [])),
        )

    def to_cpp(self):
        native = _funccraft.ValueTransformSpec()
        native.kind = self.kind
        native.seed = self.seed
        native.parameters = _copy_list(self.parameters)
        return native


class ComponentSpec(SpecMapping):
    """One primitive component of a benchmark function."""

    _fields = (
        ("base_function", ""),
        ("component_dimension", 0),
        ("coordinate_transform", TransformSpec()),
        ("value_transform", ValueTransformSpec()),
        ("seed", 0),
    )

    def __init__(self, base_function="", component_dimension=0, coordinate_transform=None, value_transform=None, seed=0):
        super().__init__()
        self.base_function = base_function
        self.component_dimension = component_dimension
        self.coordinate_transform = _as_transform_spec(coordinate_transform) if coordinate_transform is not None else TransformSpec()
        self.value_transform = _as_value_transform_spec(value_transform) if value_transform is not None else ValueTransformSpec()
        self.seed = seed

    @classmethod
    def from_cpp(cls, native):
        return cls(
            base_function=str(native.base_function),
            component_dimension=int(native.component_dimension),
            coordinate_transform=TransformSpec.from_cpp(native.coordinate_transform),
            value_transform=ValueTransformSpec.from_cpp(native.value_transform),
            seed=int(native.seed),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            base_function=data.get("base_function", ""),
            component_dimension=data.get("component_dimension", 0),
            coordinate_transform=_as_transform_spec(data.get("coordinate_transform", TransformSpec())),
            value_transform=_as_value_transform_spec(data.get("value_transform", ValueTransformSpec())),
            seed=data.get("seed", 0),
        )

    def to_cpp(self):
        native = _funccraft.ComponentSpec()
        native.base_function = self.base_function
        native.component_dimension = self.component_dimension
        native.coordinate_transform = _as_transform_spec(self.coordinate_transform).to_cpp()
        native.value_transform = _as_value_transform_spec(self.value_transform).to_cpp()
        native.seed = self.seed
        return native


class CompositionSpec(SpecMapping):
    """Composition rule for combining component values."""

    _fields = (
        ("kind", ""),
        ("seed", 0),
        ("parameters", []),
        ("centers", []),
        ("offsets", []),
        ("weights", []),
    )

    def __init__(self, kind="", seed=0, parameters=None, centers=None, offsets=None, weights=None):
        super().__init__()
        self.kind = kind
        self.seed = seed
        self.parameters = _copy_list(parameters)
        self.centers = [_copy_list(center) for center in (centers or [])]
        self.offsets = _copy_list(offsets)
        self.weights = _copy_list(weights)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            kind=str(native.kind),
            seed=int(native.seed),
            parameters=_copy_list(native.parameters),
            centers=[_copy_list(center) for center in native.centers],
            offsets=_copy_list(native.offsets),
            weights=_copy_list(native.weights),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            kind=data.get("kind", ""),
            seed=data.get("seed", 0),
            parameters=_copy_list(data.get("parameters", [])),
            centers=[_copy_list(center) for center in data.get("centers", [])],
            offsets=_copy_list(data.get("offsets", [])),
            weights=_copy_list(data.get("weights", [])),
        )

    def to_cpp(self):
        native = _funccraft.CompositionSpec()
        native.kind = self.kind
        native.seed = self.seed
        native.parameters = _copy_list(self.parameters)
        native.centers = [_copy_list(center) for center in self.centers]
        native.offsets = _copy_list(self.offsets)
        native.weights = _copy_list(self.weights)
        return native


class FunctionSpec(SpecMapping):
    """Full specification for one concrete benchmark function."""

    _fields = (
        ("dimension", 0),
        ("lower_bound", []),
        ("upper_bound", []),
        ("component_specs", []),
        ("composition_spec", CompositionSpec()),
        ("seed", 0),
        ("function_class_label", ""),
        ("known_global_minimizer", []),
        ("known_global_value", 0.0),
        ("parameters", []),
    )

    def __init__(self, dimension=0, lower_bound=None, upper_bound=None, component_specs=None, composition_spec=None, seed=0, function_class_label="", known_global_minimizer=None, known_global_value=0.0, parameters=None):
        super().__init__()
        self.dimension = dimension
        self.lower_bound = _copy_list(lower_bound)
        self.upper_bound = _copy_list(upper_bound)
        self.component_specs = [
            _as_component_spec(item)
            for item in (component_specs or [])
        ]
        self.composition_spec = _as_composition_spec(composition_spec) if composition_spec is not None else CompositionSpec()
        self.seed = seed
        self.function_class_label = function_class_label
        self.known_global_minimizer = _copy_list(known_global_minimizer)
        self.known_global_value = known_global_value
        self.parameters = _copy_list(parameters)

    @classmethod
    def from_cpp(cls, native):
        return cls(
            dimension=int(native.dimension),
            lower_bound=_copy_list(native.lower_bound),
            upper_bound=_copy_list(native.upper_bound),
            component_specs=[ComponentSpec.from_cpp(component) for component in native.component_specs],
            composition_spec=CompositionSpec.from_cpp(native.composition_spec),
            seed=int(native.seed),
            function_class_label=str(native.function_class_label),
            known_global_minimizer=_copy_list(native.known_global_minimizer),
            known_global_value=float(native.known_global_value),
            parameters=_copy_list(native.parameters),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            dimension=data.get("dimension", 0),
            lower_bound=_copy_list(data.get("lower_bound", [])),
            upper_bound=_copy_list(data.get("upper_bound", [])),
            component_specs=[_as_component_spec(item) for item in data.get("component_specs", [])],
            composition_spec=_as_composition_spec(data.get("composition_spec", CompositionSpec())),
            seed=data.get("seed", 0),
            function_class_label=data.get("function_class_label", ""),
            known_global_minimizer=_copy_list(data.get("known_global_minimizer", [])),
            known_global_value=data.get("known_global_value", 0.0),
            parameters=_copy_list(data.get("parameters", [])),
        )

    def to_cpp(self):
        native = _funccraft.FunctionSpec()
        native.dimension = self.dimension
        native.lower_bound = _copy_list(self.lower_bound)
        native.upper_bound = _copy_list(self.upper_bound)
        native.component_specs = [_as_component_spec(component).to_cpp() for component in self.component_specs]
        native.composition_spec = _as_composition_spec(self.composition_spec).to_cpp()
        native.seed = self.seed
        native.function_class_label = self.function_class_label
        native.known_global_minimizer = _copy_list(self.known_global_minimizer)
        native.known_global_value = self.known_global_value
        native.parameters = _copy_list(self.parameters)
        return native


class SuiteSpec(SpecMapping):
    """Declarative description of a benchmark suite."""

    _fields = (
        ("supported_dimensions", "any"),
        ("base_functions", _default_base_functions()),
        ("base_function_coord_transforms", _default_base_function_coord_transforms()),
        ("coord_transforms", _default_coord_transforms()),
        ("value_transforms", _default_value_transforms()),
        ("composition_functions", _default_composition_functions()),
        ("base_functions_for_compositions", _default_composition_base_functions()),
        ("max_components", 10),
        ("requested_number_of_functions", 0),
        ("max_number_of_functions", 0),
        ("master_seed", 1),
        ("lower_bound", -100.0),
        ("upper_bound", 100.0),
        ("f_opt", 100.0),
        ("suite_label", ""),
    )

    def __init__(
        self,
        supported_dimensions="any",
        base_functions=None,
        base_function_coord_transforms=None,
        coord_transforms=None,
        value_transforms=None,
        composition_functions=None,
        base_functions_for_compositions=None,
        max_components=10,
        requested_number_of_functions=0,
        max_number_of_functions=0,
        master_seed=1,
        lower_bound=-100.0,
        upper_bound=100.0,
        f_opt=100.0,
        suite_label="",
    ):
        super().__init__()
        self.supported_dimensions = supported_dimensions
        self.base_functions = _copy_list(base_functions) if base_functions is not None else _default_base_functions()
        self.base_function_coord_transforms = [
            _as_choice_spec(item)
            for item in (base_function_coord_transforms or _default_base_function_coord_transforms())
        ]
        self.coord_transforms = [
            _as_choice_spec(item)
            for item in (coord_transforms or _default_coord_transforms())
        ]
        self.value_transforms = [
            _as_choice_spec(item)
            for item in (value_transforms or _default_value_transforms())
        ]
        self.composition_functions = [
            _as_choice_spec(item)
            for item in (composition_functions or _default_composition_functions())
        ]
        self.base_functions_for_compositions = (
            _copy_list(base_functions_for_compositions)
            if base_functions_for_compositions is not None
            else _default_composition_base_functions()
        )
        self.max_components = max_components
        self.requested_number_of_functions = requested_number_of_functions
        self.max_number_of_functions = max_number_of_functions
        self.master_seed = master_seed
        self.lower_bound = lower_bound
        self.upper_bound = upper_bound
        self.f_opt = f_opt
        self.suite_label = suite_label

    @classmethod
    def from_cpp(cls, native):
        return cls(
            supported_dimensions=str(native.supported_dimensions),
            base_functions=_copy_list(native.base_functions),
            base_function_coord_transforms=[ChoiceSpec.from_cpp(choice) for choice in native.base_function_coord_transforms],
            coord_transforms=[ChoiceSpec.from_cpp(choice) for choice in native.coord_transforms],
            value_transforms=[ChoiceSpec.from_cpp(choice) for choice in native.value_transforms],
            composition_functions=[ChoiceSpec.from_cpp(choice) for choice in native.composition_functions],
            base_functions_for_compositions=_copy_list(native.base_functions_for_compositions),
            max_components=int(native.max_components),
            requested_number_of_functions=int(native.requested_number_of_functions),
            max_number_of_functions=int(native.max_number_of_functions),
            master_seed=int(native.master_seed),
            lower_bound=float(native.lower_bound),
            upper_bound=float(native.upper_bound),
            f_opt=float(native.f_opt),
            suite_label=str(native.suite_label),
        )

    @classmethod
    def from_dict(cls, data):
        return cls(
            supported_dimensions=data.get("supported_dimensions", "any"),
            base_functions=_copy_list(data.get("base_functions", _default_base_functions())),
            base_function_coord_transforms=[_as_choice_spec(item) for item in data.get("base_function_coord_transforms", _default_base_function_coord_transforms())],
            coord_transforms=[_as_choice_spec(item) for item in data.get("coord_transforms", _default_coord_transforms())],
            value_transforms=[_as_choice_spec(item) for item in data.get("value_transforms", _default_value_transforms())],
            composition_functions=[_as_choice_spec(item) for item in data.get("composition_functions", _default_composition_functions())],
            base_functions_for_compositions=_copy_list(data.get("base_functions_for_compositions", _default_composition_base_functions())),
            max_components=data.get("max_components", 10),
            requested_number_of_functions=data.get("requested_number_of_functions", 0),
            max_number_of_functions=data.get("max_number_of_functions", 0),
            master_seed=data.get("master_seed", 1),
            lower_bound=data.get("lower_bound", -100.0),
            upper_bound=data.get("upper_bound", 100.0),
            f_opt=data.get("f_opt", 100.0),
            suite_label=data.get("suite_label", ""),
        )

    def to_cpp(self):
        native = _funccraft.SuiteSpec()
        native.supported_dimensions = self.supported_dimensions
        native.base_functions = _copy_list(self.base_functions)
        native.base_function_coord_transforms = [_as_choice_spec(choice).to_cpp() for choice in self.base_function_coord_transforms]
        native.coord_transforms = [_as_choice_spec(choice).to_cpp() for choice in self.coord_transforms]
        native.value_transforms = [_as_choice_spec(choice).to_cpp() for choice in self.value_transforms]
        native.composition_functions = [_as_choice_spec(choice).to_cpp() for choice in self.composition_functions]
        native.base_functions_for_compositions = _copy_list(self.base_functions_for_compositions)
        native.max_components = self.max_components
        native.requested_number_of_functions = self.requested_number_of_functions
        native.max_number_of_functions = self.max_number_of_functions
        native.master_seed = self.master_seed
        native.lower_bound = self.lower_bound
        native.upper_bound = self.upper_bound
        native.f_opt = self.f_opt
        native.suite_label = self.suite_label
        return native


__all__ = [
    "ChoiceSpec",
    "ComponentSpec",
    "CompositionSpec",
    "Domain",
    "FunctionSpec",
    "SpecMapping",
    "SuiteSpec",
    "TransformSpec",
    "ValueTransformSpec",
]
