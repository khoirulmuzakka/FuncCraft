"""Thin Python helpers for FuncCraft specs.

The C++ bindings expose the actual spec structs. This module adds only two
Python conveniences:

- readable ``make_*`` factory functions that return native C++ spec objects;
- dict-to-spec conversion used by ``BenchmarkFunction`` and ``BenchmarkSuite``.

Typical manual function:

    from funccraft import *

    xopt = [1.0, -2.0]

    spec = make_function_spec(
        dimension=2,
        domain=make_domain(2, -10.0, 10.0),
        components=[
            make_component(
                base_function="Sphere",
                coordinate_transform=make_coordinate_transform(
                    kind="rotation",
                    dimension=2,
                    assigned_xopt=xopt,
                    seed=1,
                ),
            ),
        ],
        composition=make_composition("none"),
        assigned_xopt=xopt,
        assigned_fopt=0.0,
    )

    f = BenchmarkFunction(spec)

Typical suite:

    suite_spec = make_suite_spec(
        requested_number_of_functions=500,
        max_components=4,
        master_seed=1,
        compositions=[
            make_composition_choice("dpm-bgsoftmax", 0.5, [0.01, 1.0, 0.01]),
            make_composition_choice("dpm-softmax", 0.5, [0.01]),
        ],
    )

    suite = BenchmarkSuite(suite_spec, dimension=10)
"""

from __future__ import annotations

from collections.abc import Mapping

from . import _funccraft

BasicFunctionId = _funccraft.BasicFunctionId
CoordinateTransformKind = _funccraft.CoordinateTransformKind
ValueTransformKind = _funccraft.ValueTransformKind
CompositionKind = _funccraft.CompositionKind

DomainSpec = _funccraft.DomainSpec
CoordinateTransformSpec = _funccraft.CoordinateTransformSpec
ValueTransformSpec = _funccraft.ValueTransformSpec
ComponentSpec = _funccraft.ComponentSpec
CompositionSpec = _funccraft.CompositionSpec
FunctionSpec = _funccraft.FunctionSpec
SuiteSpec = _funccraft.SuiteSpec

CoordinateTransformChoice = _funccraft.CoordinateTransformChoice
ValueTransformChoice = _funccraft.ValueTransformChoice
CompositionChoice = _funccraft.CompositionChoice

Domain = DomainSpec
TransformSpec = CoordinateTransformSpec
ChoiceSpec = CompositionChoice


def make_domain(dimension, lower_bound=-100.0, upper_bound=100.0):
    """Create a native ``DomainSpec``.

    ``lower_bound`` and ``upper_bound`` may be scalars or length-``dimension``
    sequences.
    """
    spec = DomainSpec()
    spec.dimension = int(dimension)
    if isinstance(lower_bound, (int, float)):
        spec.lower_bound = [float(lower_bound)] * spec.dimension
    else:
        spec.lower_bound = _list(lower_bound)
    if isinstance(upper_bound, (int, float)):
        spec.upper_bound = [float(upper_bound)] * spec.dimension
    else:
        spec.upper_bound = _list(upper_bound)
    return spec


def make_coordinate_transform(
    kind="none",
    dimension=0,
    assigned_xopt=None,
    selected_indices=None,
    parameters=None,
    matrix=None,
    seed=0,
):
    """Create a native ``CoordinateTransformSpec``.

    ``assigned_xopt`` is the desired optimum location in generated/search
    coordinates. The transform target is determined internally from the
    selected base function and domain scaling.
    """
    spec = CoordinateTransformSpec()
    spec.kind = coordinate_transform_kind(kind)
    spec.dimension = int(dimension)
    spec.assigned_xopt = _list(assigned_xopt)
    spec.selected_indices = _list(selected_indices)
    spec.parameters = _list(parameters)
    spec.matrix = _matrix(matrix)
    spec.seed = int(seed)
    return spec


def make_value_transform(kind="none", parameters=None, seed=0):
    """Create a native ``ValueTransformSpec``."""
    spec = ValueTransformSpec()
    spec.kind = value_transform_kind(kind)
    spec.parameters = _list(parameters)
    spec.seed = int(seed)
    return spec


def make_component(
    base_function=None,
    composed_function=None,
    coordinate_transform=None,
    value_transform=None,
    seed=0,
):
    """Create a native ``ComponentSpec``.

    ``base_function`` accepts a ``BasicFunctionId``, integer id, or name.
    It is required only when ``composed_function`` is not set. A composed
    component may be a ``FunctionSpec``, compatible dict, or object exposing
    a ``spec`` attribute such as ``BenchmarkFunction``.
    ``coordinate_transform`` and ``value_transform`` may be native specs or
    dictionaries with the same field names.
    """
    spec = ComponentSpec()
    if composed_function is not None:
        spec.composed_function = _owned_function_spec(composed_function)
    else:
        if base_function is None:
            raise ValueError("basic component requires base_function")
        spec.base_function = basic_function_id(base_function)
    spec.coordinate_transform = coordinate_transform_spec(coordinate_transform or {})
    spec.value_transform = value_transform_spec(value_transform or {})
    spec.seed = int(seed)
    return spec


def make_composition(kind="none", parameters=None, biases=None, seed=0):
    """Create a native ``CompositionSpec``."""
    spec = CompositionSpec()
    kind = composition_kind(kind)
    bias_values = _list(biases)
    if bias_values and kind not in (CompositionKind.DpmSoftmax, CompositionKind.DpmBgSoftmax):
        raise ValueError("composition biases are only valid for DPM compositions")
    spec.kind = kind
    spec.parameters = _list(parameters)
    spec.biases = bias_values
    spec.seed = int(seed)
    return spec


def make_function_spec(
    dimension,
    domain=None,
    components=None,
    composition=None,
    assigned_xopt=None,
    assigned_fopt=0.0,
    scale_factor=None,
    seed=0,
    label="",
    metadata=None,
):
    """Create a native ``FunctionSpec``.

    ``components`` may contain native ``ComponentSpec`` objects or dictionaries.
    ``scale_factor=None`` lets FuncCraft choose the scale internally.
    """
    spec = FunctionSpec()
    spec.dimension = int(dimension)
    spec.domain = domain_spec(domain or {})
    spec.components = [component_spec(item) for item in (components or [])]
    spec.composition = composition_spec(composition or {})
    spec.assigned_xopt = _list(assigned_xopt)
    spec.assigned_fopt = float(assigned_fopt)
    spec.scale_factor = scale_factor
    spec.seed = int(seed)
    spec.label = str(label)
    spec.metadata = _list(metadata)
    return spec


def make_coordinate_transform_choice(kind, probability=1.0, parameters=None):
    """Create a weighted coordinate-transform choice for ``SuiteSpec``."""
    spec = CoordinateTransformChoice()
    spec.kind = coordinate_transform_kind(kind)
    spec.probability = float(probability)
    spec.parameters = _list(parameters)
    return spec


def make_value_transform_choice(kind, probability=1.0, parameters=None):
    """Create a weighted value-transform choice for ``SuiteSpec``."""
    spec = ValueTransformChoice()
    spec.kind = value_transform_kind(kind)
    spec.probability = float(probability)
    spec.parameters = _list(parameters)
    return spec


def make_composition_choice(kind, probability=1.0, parameters=None):
    """Create a weighted composition choice for ``SuiteSpec``."""
    spec = CompositionChoice()
    spec.kind = composition_kind(kind)
    spec.probability = float(probability)
    spec.parameters = _list(parameters)
    return spec


def make_choice(kind, probability=1.0, parameters=None):
    """Alias for ``make_composition_choice``.

    Use the explicit ``make_coordinate_transform_choice`` or
    ``make_value_transform_choice`` when constructing those choice tables.
    """
    return make_composition_choice(kind, probability, parameters)


def make_suite_spec(
    supported_dimensions=None,
    base_functions=None,
    composition_base_functions=None,
    coordinate_transforms=None,
    value_transforms=None,
    compositions=None,
    min_components=None,
    max_components=None,
    max_nested_composition_depth=None,
    nested_probability=None,
    requested_number_of_functions=None,
    max_number_of_functions=None,
    master_seed=None,
    lower_bound=None,
    upper_bound=None,
    assigned_fopt=None,
    xopt_domain_shrink_factor=None,
    suite_label=None,
):
    """Create a native ``SuiteSpec``.

    Omitted fields keep the C++ defaults. Choice tables accept native choice
    objects or dictionaries with ``kind``, ``probability``, and ``parameters``.
    ``max_nested_composition_depth=0`` means composed suite functions use only
    primitive components. Larger values allow composed functions as components;
    ``nested_probability`` controls how often each component is nested.
    """
    spec = SuiteSpec()
    if supported_dimensions is not None:
        spec.supported_dimensions = str(supported_dimensions)
    if base_functions is not None:
        spec.base_functions = [basic_function_id(item) for item in base_functions]
    if composition_base_functions is not None:
        spec.composition_base_functions = [
            basic_function_id(item) for item in composition_base_functions
        ]
    if coordinate_transforms is not None:
        spec.coordinate_transforms = [
            coordinate_transform_choice(item) for item in coordinate_transforms
        ]
    if value_transforms is not None:
        spec.value_transforms = [value_transform_choice(item) for item in value_transforms]
    if compositions is not None:
        spec.compositions = [composition_choice(item) for item in compositions]
    if min_components is not None:
        spec.min_components = int(min_components)
    if max_components is not None:
        spec.max_components = int(max_components)
    if max_nested_composition_depth is not None:
        spec.max_nested_composition_depth = int(max_nested_composition_depth)
    if nested_probability is not None:
        spec.nested_probability = float(nested_probability)
    if requested_number_of_functions is not None:
        spec.requested_number_of_functions = int(requested_number_of_functions)
    if max_number_of_functions is not None:
        spec.max_number_of_functions = int(max_number_of_functions)
    if master_seed is not None:
        spec.master_seed = int(master_seed)
    if lower_bound is not None:
        spec.lower_bound = float(lower_bound)
    if upper_bound is not None:
        spec.upper_bound = float(upper_bound)
    if assigned_fopt is not None:
        spec.assigned_fopt = float(assigned_fopt)
    if xopt_domain_shrink_factor is not None:
        spec.xopt_domain_shrink_factor = float(xopt_domain_shrink_factor)
    if suite_label is not None:
        spec.suite_label = str(suite_label)
    return spec


def _list(value):
    return list(value) if value is not None else []


def _matrix(value):
    return [_list(row) for row in (value or [])]


def _normalize_name(value):
    return "".join(ch.lower() for ch in str(value) if ch not in "_- \t\r\n")


def _enum(enum_type, value, aliases=None):
    if isinstance(value, enum_type):
        return value
    if isinstance(value, int):
        return enum_type(value)
    normalized = _normalize_name(value)
    aliases = aliases or {}
    if normalized in aliases:
        return aliases[normalized]
    for item in enum_type.__members__.values():
        if _normalize_name(item.name) == normalized:
            return item
    raise ValueError(f"unknown {enum_type.__name__}: {value!r}")


_NO_COORDINATE_TRANSFORM = getattr(CoordinateTransformKind, "None")
_NO_VALUE_TRANSFORM = getattr(ValueTransformKind, "None")
_NO_COMPOSITION = getattr(CompositionKind, "None")

_COORDINATE_ALIASES = {
    "": _NO_COORDINATE_TRANSFORM,
    "none": _NO_COORDINATE_TRANSFORM,
    "identity": _NO_COORDINATE_TRANSFORM,
    "rot": CoordinateTransformKind.Rotation,
    "aff": CoordinateTransformKind.Affine,
    "blockrot": CoordinateTransformKind.BlockRotation,
    "blockrotation": CoordinateTransformKind.BlockRotation,
    "brot": CoordinateTransformKind.BlockRotation,
}

_VALUE_ALIASES = {
    "": _NO_VALUE_TRANSFORM,
    "none": _NO_VALUE_TRANSFORM,
    "identity": _NO_VALUE_TRANSFORM,
    "osc": ValueTransformKind.Oscillatory,
    "oscillatory": ValueTransformKind.Oscillatory,
    "coszero": ValueTransformKind.CosineZero,
    "cosinezero": ValueTransformKind.CosineZero,
}

_COMPOSITION_ALIASES = {
    "": _NO_COMPOSITION,
    "none": _NO_COMPOSITION,
    "identity": _NO_COMPOSITION,
    "cpm": CompositionKind.CpmWeightedSum,
    "sum": CompositionKind.CpmWeightedSum,
    "cpmsum": CompositionKind.CpmWeightedSum,
    "cpmwsum": CompositionKind.CpmWeightedSum,
    "weightedsum": CompositionKind.CpmWeightedSum,
    "cpmpmean": CompositionKind.CpmPowerMean,
    "cpmpowermean": CompositionKind.CpmPowerMean,
    "powermean": CompositionKind.CpmPowerMean,
    "cpmlwell": CompositionKind.CpmLevelWell,
    "cpmlevelwell": CompositionKind.CpmLevelWell,
    "levelwell": CompositionKind.CpmLevelWell,
    "lwell": CompositionKind.CpmLevelWell,
    "dpm": CompositionKind.DpmSoftmax,
    "softmax": CompositionKind.DpmSoftmax,
    "dpmsoftmax": CompositionKind.DpmSoftmax,
    "dpmbg": CompositionKind.DpmBgSoftmax,
    "bgsoftmax": CompositionKind.DpmBgSoftmax,
    "dpmbgsoftmax": CompositionKind.DpmBgSoftmax,
}


def basic_function_id(value):
    return _enum(BasicFunctionId, value)


def coordinate_transform_kind(value):
    return _enum(CoordinateTransformKind, value, _COORDINATE_ALIASES)


def value_transform_kind(value):
    return _enum(ValueTransformKind, value, _VALUE_ALIASES)


def composition_kind(value):
    return _enum(CompositionKind, value, _COMPOSITION_ALIASES)


def domain_spec(data):
    if isinstance(data, DomainSpec):
        return data
    spec = DomainSpec()
    if data is None:
        return spec
    if hasattr(data, "dimension") and hasattr(data, "lower") and hasattr(data, "upper"):
        spec.dimension = int(data.dimension)
        spec.lower_bound = _list(data.lower)
        spec.upper_bound = _list(data.upper)
        return spec
    spec.dimension = int(data.get("dimension", 0))
    spec.lower_bound = _list(data.get("lower_bound", []))
    spec.upper_bound = _list(data.get("upper_bound", []))
    return spec


def coordinate_transform_spec(data):
    if isinstance(data, CoordinateTransformSpec):
        return data
    data = data or {}
    spec = CoordinateTransformSpec()
    spec.kind = coordinate_transform_kind(data.get("kind", _NO_COORDINATE_TRANSFORM))
    spec.dimension = int(data.get("dimension", 0))
    spec.assigned_xopt = _list(data.get("assigned_xopt", []))
    spec.selected_indices = _list(data.get("selected_indices", []))
    spec.parameters = _list(data.get("parameters", []))
    spec.matrix = _matrix(data.get("matrix", []))
    spec.seed = int(data.get("seed", 0))
    return spec


def value_transform_spec(data):
    if isinstance(data, ValueTransformSpec):
        return data
    data = data or {}
    spec = ValueTransformSpec()
    spec.kind = value_transform_kind(data.get("kind", _NO_VALUE_TRANSFORM))
    spec.parameters = _list(data.get("parameters", []))
    spec.seed = int(data.get("seed", 0))
    return spec


def component_spec(data):
    if isinstance(data, ComponentSpec):
        return data
    data = data or {}
    spec = ComponentSpec()
    if data.get("composed_function") is not None:
        spec.composed_function = function_spec(data["composed_function"])
    else:
        if "base_function" not in data:
            raise ValueError("basic component requires base_function")
        spec.base_function = basic_function_id(data["base_function"])
    spec.coordinate_transform = coordinate_transform_spec(data.get("coordinate_transform", {}))
    spec.value_transform = value_transform_spec(data.get("value_transform", {}))
    spec.seed = int(data.get("seed", 0))
    return spec


def composition_spec(data):
    if isinstance(data, CompositionSpec):
        return data
    data = data or {}
    spec = CompositionSpec()
    kind = composition_kind(data.get("kind", _NO_COMPOSITION))
    bias_values = _list(data.get("biases", []))
    if bias_values and kind not in (CompositionKind.DpmSoftmax, CompositionKind.DpmBgSoftmax):
        raise ValueError("composition biases are only valid for DPM compositions")
    spec.kind = kind
    spec.parameters = _list(data.get("parameters", []))
    spec.biases = bias_values
    spec.seed = int(data.get("seed", 0))
    return spec


def function_spec(data):
    if hasattr(data, "spec"):
        return function_spec(spec_to_dict(data.spec))
    if isinstance(data, FunctionSpec):
        return data
    if not isinstance(data, Mapping):
        return data
    spec = FunctionSpec()
    spec.dimension = int(data.get("dimension", 0))
    spec.domain = domain_spec(data.get("domain", {}))
    spec.components = [component_spec(item) for item in data.get("components", [])]
    spec.composition = composition_spec(data.get("composition", {}))
    spec.assigned_xopt = _list(data.get("assigned_xopt", []))
    spec.assigned_fopt = float(data.get("assigned_fopt", 0.0))
    spec.scale_factor = data.get("scale_factor", None)
    spec.seed = int(data.get("seed", 0))
    spec.label = str(data.get("label", ""))
    spec.metadata = _list(data.get("metadata", []))
    return spec


def _owned_function_spec(data):
    """Return a FunctionSpec instance safe to store in shared_ptr fields."""
    return function_spec(spec_to_dict(function_spec(data)))


def coordinate_transform_choice(data):
    if isinstance(data, CoordinateTransformChoice):
        return data
    spec = CoordinateTransformChoice()
    spec.kind = coordinate_transform_kind(data.get("kind", _NO_COORDINATE_TRANSFORM))
    spec.probability = float(data.get("probability", 1.0))
    spec.parameters = _list(data.get("parameters", []))
    return spec


def value_transform_choice(data):
    if isinstance(data, ValueTransformChoice):
        return data
    spec = ValueTransformChoice()
    spec.kind = value_transform_kind(data.get("kind", _NO_VALUE_TRANSFORM))
    spec.probability = float(data.get("probability", 1.0))
    spec.parameters = _list(data.get("parameters", []))
    return spec


def composition_choice(data):
    if isinstance(data, CompositionChoice):
        return data
    spec = CompositionChoice()
    spec.kind = composition_kind(data.get("kind", CompositionKind.CpmWeightedSum))
    spec.probability = float(data.get("probability", 1.0))
    spec.parameters = _list(data.get("parameters", []))
    return spec


def suite_spec(data):
    if isinstance(data, SuiteSpec):
        return data
    if not isinstance(data, Mapping):
        return data
    spec = SuiteSpec()
    if "supported_dimensions" in data:
        spec.supported_dimensions = str(data["supported_dimensions"])
    if "base_functions" in data:
        spec.base_functions = [basic_function_id(item) for item in data["base_functions"]]
    if "composition_base_functions" in data:
        spec.composition_base_functions = [
            basic_function_id(item) for item in data["composition_base_functions"]
        ]
    if "coordinate_transforms" in data:
        spec.coordinate_transforms = [
            coordinate_transform_choice(item) for item in data["coordinate_transforms"]
        ]
    if "value_transforms" in data:
        spec.value_transforms = [value_transform_choice(item) for item in data["value_transforms"]]
    if "compositions" in data:
        spec.compositions = [composition_choice(item) for item in data["compositions"]]
    if "min_components" in data:
        spec.min_components = int(data["min_components"])
    if "max_components" in data:
        spec.max_components = int(data["max_components"])
    if "max_nested_composition_depth" in data:
        spec.max_nested_composition_depth = int(data["max_nested_composition_depth"])
    if "nested_probability" in data:
        spec.nested_probability = float(data["nested_probability"])
    if "requested_number_of_functions" in data:
        spec.requested_number_of_functions = int(data["requested_number_of_functions"])
    if "max_number_of_functions" in data:
        spec.max_number_of_functions = int(data["max_number_of_functions"])
    if "master_seed" in data:
        spec.master_seed = int(data["master_seed"])
    if "lower_bound" in data:
        spec.lower_bound = float(data["lower_bound"])
    if "upper_bound" in data:
        spec.upper_bound = float(data["upper_bound"])
    if "assigned_fopt" in data:
        spec.assigned_fopt = float(data["assigned_fopt"])
    if "xopt_domain_shrink_factor" in data:
        spec.xopt_domain_shrink_factor = float(data["xopt_domain_shrink_factor"])
    if "suite_label" in data:
        spec.suite_label = str(data["suite_label"])
    return spec


def spec_to_dict(spec):
    """Convert a native C++ spec/choice object to a plain Python dict."""
    if isinstance(spec, DomainSpec):
        return {
            "dimension": spec.dimension,
            "lower_bound": _list(spec.lower_bound),
            "upper_bound": _list(spec.upper_bound),
        }
    if isinstance(spec, CoordinateTransformSpec):
        return {
            "kind": spec.kind.name,
            "dimension": spec.dimension,
            "assigned_xopt": _list(spec.assigned_xopt),
            "selected_indices": _list(spec.selected_indices),
            "parameters": _list(spec.parameters),
            "matrix": _matrix(spec.matrix),
            "seed": spec.seed,
        }
    if isinstance(spec, ValueTransformSpec):
        return {"kind": spec.kind.name, "parameters": _list(spec.parameters), "seed": spec.seed}
    if isinstance(spec, ComponentSpec):
        result = {
            "coordinate_transform": spec_to_dict(spec.coordinate_transform),
            "value_transform": spec_to_dict(spec.value_transform),
            "seed": spec.seed,
        }
        if spec.composed_function is not None:
            result["composed_function"] = spec_to_dict(spec.composed_function)
        else:
            if spec.base_function is None:
                raise ValueError("basic component requires base_function")
            result["base_function"] = spec.base_function.name
        return result
    if isinstance(spec, CompositionSpec):
        result = {
            "kind": spec.kind.name,
            "parameters": _list(spec.parameters),
            "seed": spec.seed,
        }
        biases = _list(spec.biases)
        if biases:
            result["biases"] = biases
        return result
    if isinstance(spec, FunctionSpec):
        return {
            "dimension": spec.dimension,
            "domain": spec_to_dict(spec.domain),
            "components": [spec_to_dict(item) for item in spec.components],
            "composition": spec_to_dict(spec.composition),
            "assigned_xopt": _list(spec.assigned_xopt),
            "assigned_fopt": spec.assigned_fopt,
            "scale_factor": spec.scale_factor,
            "seed": spec.seed,
            "label": spec.label,
            "metadata": _list(spec.metadata),
        }
    if isinstance(spec, (CoordinateTransformChoice, ValueTransformChoice, CompositionChoice)):
        return {
            "kind": spec.kind.name,
            "probability": spec.probability,
            "parameters": _list(spec.parameters),
        }
    if isinstance(spec, SuiteSpec):
        return {
            "supported_dimensions": spec.supported_dimensions,
            "base_functions": [item.name for item in spec.base_functions],
            "composition_base_functions": [item.name for item in spec.composition_base_functions],
            "coordinate_transforms": [spec_to_dict(item) for item in spec.coordinate_transforms],
            "value_transforms": [spec_to_dict(item) for item in spec.value_transforms],
            "compositions": [spec_to_dict(item) for item in spec.compositions],
            "min_components": spec.min_components,
            "max_components": spec.max_components,
            "max_nested_composition_depth": spec.max_nested_composition_depth,
            "nested_probability": spec.nested_probability,
            "requested_number_of_functions": spec.requested_number_of_functions,
            "max_number_of_functions": spec.max_number_of_functions,
            "master_seed": spec.master_seed,
            "lower_bound": spec.lower_bound,
            "upper_bound": spec.upper_bound,
            "assigned_fopt": spec.assigned_fopt,
            "xopt_domain_shrink_factor": spec.xopt_domain_shrink_factor,
            "suite_label": spec.suite_label,
        }
    raise TypeError(f"unsupported spec object: {type(spec)!r}")


__all__ = [
    "BasicFunctionId",
    "ChoiceSpec",
    "ComponentSpec",
    "CompositionChoice",
    "CompositionKind",
    "CompositionSpec",
    "CoordinateTransformChoice",
    "CoordinateTransformKind",
    "CoordinateTransformSpec",
    "Domain",
    "DomainSpec",
    "FunctionSpec",
    "SuiteSpec",
    "TransformSpec",
    "ValueTransformChoice",
    "ValueTransformKind",
    "ValueTransformSpec",
    "basic_function_id",
    "component_spec",
    "composition_choice",
    "composition_kind",
    "composition_spec",
    "coordinate_transform_choice",
    "coordinate_transform_kind",
    "coordinate_transform_spec",
    "domain_spec",
    "function_spec",
    "make_choice",
    "make_component",
    "make_composition",
    "make_composition_choice",
    "make_coordinate_transform",
    "make_coordinate_transform_choice",
    "make_domain",
    "make_function_spec",
    "make_suite_spec",
    "make_value_transform",
    "make_value_transform_choice",
    "spec_to_dict",
    "suite_spec",
    "value_transform_choice",
    "value_transform_kind",
    "value_transform_spec",
]
