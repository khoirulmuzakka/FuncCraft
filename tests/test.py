from __future__ import annotations

import math
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
TEST_INSTALLED_PACKAGE = "--installed" in sys.argv
if TEST_INSTALLED_PACKAGE:
    sys.argv.remove("--installed")
    sys.path = [
        path for path in sys.path
        if Path(path or ".").resolve() != PROJECT_ROOT
    ]
else:
    sys.path.insert(0, str(PROJECT_ROOT))

OPTIMUM_FUNCTION_COUNT = 500

try:
    from funccraft import (
        BasicFunctionId,
        BenchmarkFunction,
        BenchmarkSuite,
        SuiteCollection,
    )
except ModuleNotFoundError:
    from funccraft import (
        BasicFunctionId,
        BenchmarkFunction,
        BenchmarkSuite,
        SuiteCollection,
    )


def assert_close_sequence(actual, expected, *, tolerance=1.0e-9):
    if len(actual) != len(expected):
        raise AssertionError(f"length mismatch: {len(actual)} != {len(expected)}")
    for i, (a, b) in enumerate(zip(actual, expected)):
        if not math.isclose(a, b, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(f"value mismatch at {i}: {a!r} != {b!r}")


def candidate_points(function):
    dimension = function.dimension
    x_star = function.get_xopt()
    zero = [0.0] * dimension
    pattern = [0.25 if i % 2 == 0 else -0.75 for i in range(dimension)]
    return [x_star, zero, pattern]


def check_optima(suite, *, count=None, tolerance=20.0):
    end = len(suite) if count is None else min(count, len(suite))
    for index in range(1, end + 1):
        function = suite.function(index)
        x_star = function.get_xopt()
        value = function.evaluate([x_star])[0]
        expected = function.get_fopt()
        if not math.isclose(value, expected, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(
                f"optimum mismatch for function {index}: {value!r} != {expected!r}"
            )


def check_function_yaml_roundtrip(function, path):
    points = candidate_points(function)
    before = function.evaluate(points)
    function.export_yaml(str(path))

    imported_function = BenchmarkFunction(str(path))
    after = imported_function.evaluate(points)
    assert_close_sequence(after, before)


def check_suite_yaml_roundtrip(suite, path):
    first_composed = len(BasicFunctionId.__members__) + 1
    indices = [1, first_composed + 2, len(suite)]
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


def alias_function_config(kind):
    return {
        "dimension": 2,
        "domain": {
            "dimension": 2,
            "lower_bound": [-10.0, -10.0],
            "upper_bound": [10.0, 10.0],
        },
        "components": [
            {
                "base_function": "Sphere",
                "coordinate_transform": {
                    "kind": "none",
                    "input_dimension": 2,
                    "output_dimension": 2,
                    "assigned_xopt": [float(index), 0.0],
                },
                "value_transform": {"kind": "none"},
                "seed": 11 + index,
            }
            for index in range(2)
        ],
        "composition": {"kind": kind, "parameters": [0.01, 1.0, 0.01]},
        "assigned_xopt": [0.0, 0.0],
        "assigned_fopt": 0.0,
        "seed": 7,
    }


def check_composition_kinds():
    expected_names = {
        "dpm-softmax": "DpmSoftmax",
        "dpm-bgsoftmax": "DpmBgSoftmax",
    }
    for kind, expected_name in expected_names.items():
        function = BenchmarkFunction(alias_function_config(kind))
        if function._function.spec.composition.kind.name != expected_name:
            raise AssertionError(f"composition kind did not roundtrip: {kind!r}")
        values = function.evaluate([[0.0, 0.0], [1.0, 0.0]])
        if len(values) != 2:
            raise AssertionError(f"composition evaluation failed: {kind!r}")

    suite_config = {
        "compositions": [
            {"kind": "dpm-bgsoftmax", "probability": 1.0, "parameters": [0.01, 1.0, 0.01]},
        ],
        "requested_number_of_functions": 40,
        "max_components": 3,
        "master_seed": 19,
    }

    suite = BenchmarkSuite(suite_config, 2)
    if not any(
        suite.function(index)._function.spec.composition.kind.name == "DpmBgSoftmax"
        for index in range(1, len(suite) + 1)
    ):
        raise AssertionError("suite composition choice did not generate DPM bg softmax functions")


def check_basic_function_ids_start_at_one():
    values = [item.value for item in BasicFunctionId.__members__.values()]
    expected = list(range(1, len(values) + 1))
    if values != expected:
        raise AssertionError(f"basic function ids are not contiguous from 1: {values!r}")
    try:
        BenchmarkSuite({"base_functions": [0]}, 2)
    except ValueError:
        return
    raise AssertionError("base function id 0 was accepted")


def main():
    dimension = 3

    check_basic_function_ids_start_at_one()

    suite_year = 2026
    suite_version = 1
    collection = SuiteCollection(suite_year, suite_version)
    if collection.number_of_functions <= 0:
        raise AssertionError("suite collection function count mismatch")
    collection_suite = collection.benchmark_suite(2)
    first_function = collection_suite.function(1)
    value = first_function.evaluate([first_function.get_xopt()])[0]
    if not math.isfinite(value):
        raise AssertionError("suite collection generated a nonfinite value")

    check_optima(collection_suite, count=OPTIMUM_FUNCTION_COUNT)
    check_composition_kinds()

    roundtrip_suite_config = {
        "requested_number_of_functions": 100,
        "max_components": 4,
        "master_seed": 123,
    }
    roundtrip_suite = BenchmarkSuite(roundtrip_suite_config, dimension)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        check_function_yaml_roundtrip(
            roundtrip_suite.function(len(BasicFunctionId.__members__) + 3),
            tmpdir / "function.yaml",
        )
        check_suite_yaml_roundtrip(roundtrip_suite, tmpdir / "suite.yaml")


if __name__ == "__main__":
    main()
