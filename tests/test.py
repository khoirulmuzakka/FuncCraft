from __future__ import annotations

import math
import sys
import tempfile
from pathlib import Path

try:
    from funccraft import (
        BasicFunctionId,
        BenchmarkFunction,
        BenchmarkSuite,
        CompositionKind,
        load_suite_spec_yaml,
        make_component,
        make_composition,
        make_composition_choice,
        make_coordinate_transform,
        make_domain,
        make_function_spec,
        make_suite_spec,
        make_value_transform,
        make_benchmark_function_from_yaml,
    )
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from funccraft import (
        BasicFunctionId,
        BenchmarkFunction,
        BenchmarkSuite,
        CompositionKind,
        load_suite_spec_yaml,
        make_component,
        make_composition,
        make_composition_choice,
        make_coordinate_transform,
        make_domain,
        make_function_spec,
        make_suite_spec,
        make_value_transform,
        make_benchmark_function_from_yaml,
    )


def assert_close_sequence(actual, expected, *, tolerance=1.0e-9):
    if len(actual) != len(expected):
        raise AssertionError(f"length mismatch: {len(actual)} != {len(expected)}")
    for i, (a, b) in enumerate(zip(actual, expected)):
        if not math.isclose(a, b, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(f"value mismatch at {i}: {a!r} != {b!r}")


def candidate_points(function):
    dimension = function.dimension
    x_star = list(function.spec.assigned_xopt)
    zero = [0.0] * dimension
    pattern = [0.25 if i % 2 == 0 else -0.75 for i in range(dimension)]
    return [x_star, zero, pattern]


def check_optima(suite, *, tolerance=20.0):
    for index in range(len(suite)):
        function = suite.function(index)
        x_star = list(function.spec.assigned_xopt)
        value = function.evaluate([x_star])[0]
        expected = function.spec.assigned_fopt
        if not math.isclose(value, expected, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(
                f"optimum mismatch for function {index}: {value!r} != {expected!r}"
            )


def check_function_yaml_roundtrip(function, path):
    points = candidate_points(function)
    before = function.evaluate(points)
    function.export_spec(str(path))

    imported_function = make_benchmark_function_from_yaml(str(path))
    after = imported_function.evaluate(points)
    assert_close_sequence(after, before)


def check_suite_yaml_roundtrip(suite, path):
    indices = [0, 36, len(suite) - 1]
    before = {
        index: suite.function(index).evaluate(candidate_points(suite.function(index)))
        for index in indices
    }
    suite.export_manifest(str(path))

    imported_suite = BenchmarkSuite(str(path), suite.dimension)
    for index in indices:
        function = imported_suite.function(index)
        after = function.evaluate(candidate_points(function))
        assert_close_sequence(after, before[index])


def alias_function_spec(kind):
    return make_function_spec(
        dimension=2,
        domain=make_domain(2, -10.0, 10.0),
        components=[
            make_component(
                base_function=BasicFunctionId.Sphere,
                component_dimension=2,
                coordinate_transform=make_coordinate_transform(
                    kind="none",
                    dimension=2,
                    assigned_xopt=[float(index), 0.0],
                    base_xopt=[0.0, 0.0],
                ),
                value_transform=make_value_transform("none"),
                f_bias=10.0 * index,
                seed=11 + index,
            )
            for index in range(2)
        ],
        composition=make_composition(kind, parameters=[0.01, 1.0, 0.01]),
        assigned_xopt=[0.0, 0.0],
        assigned_fopt=0.0,
        seed=7,
    )


def check_composition_kinds():
    for kind in (CompositionKind.DpmSoftmax, CompositionKind.DpmBgSoftmax):
        function = BenchmarkFunction(alias_function_spec(kind))
        if function.spec.composition.kind != kind:
            raise AssertionError(f"composition kind did not roundtrip: {kind!r}")
        values = function.evaluate([[0.0, 0.0], [1.0, 0.0]])
        if len(values) != 2:
            raise AssertionError(f"composition evaluation failed: {kind!r}")

    suite_spec = make_suite_spec(
        compositions=[
            make_composition_choice(CompositionKind.DpmBgSoftmax, 1.0, [0.01, 1.0, 0.01])
        ],
        requested_number_of_functions=40,
        max_components=3,
        master_seed=19,
    )

    suite = BenchmarkSuite(suite_spec, 2)
    if not any(
        suite.function(index).spec.composition.kind == CompositionKind.DpmBgSoftmax
        for index in range(len(suite))
    ):
        raise AssertionError("suite composition choice did not generate DPM bg softmax functions")


def main():
    dimension = 3
    default_suite_path = Path(__file__).resolve().parents[1] / "BenchmarkSuites" / "default_suite.yaml"

    optimum_suite_spec = load_suite_spec_yaml(str(default_suite_path))
    optimum_suite_spec.requested_number_of_functions = 36
    optimum_suite = BenchmarkSuite(optimum_suite_spec, 2)
    check_optima(optimum_suite)
    check_composition_kinds()

    roundtrip_suite_spec = make_suite_spec(
        requested_number_of_functions=100,
        max_components=4,
        master_seed=123,
    )
    roundtrip_suite = BenchmarkSuite(roundtrip_suite_spec, dimension)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        check_function_yaml_roundtrip(roundtrip_suite.function(36), tmpdir / "function.yaml")
        check_suite_yaml_roundtrip(roundtrip_suite, tmpdir / "suite.yaml")


if __name__ == "__main__":
    main()
