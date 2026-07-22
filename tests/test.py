from __future__ import annotations

import math
import sys
import tempfile
from pathlib import Path

try:
    from funccraft import BenchmarkFunction, BenchmarkSuite, SuiteSpec, load_function_spec_yaml, load_suite_spec_yaml
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from funccraft import BenchmarkFunction, BenchmarkSuite, SuiteSpec, load_function_spec_yaml, load_suite_spec_yaml


def assert_close_sequence(actual, expected, *, tolerance=1.0e-9):
    if len(actual) != len(expected):
        raise AssertionError(f"length mismatch: {len(actual)} != {len(expected)}")
    for i, (a, b) in enumerate(zip(actual, expected)):
        if not math.isclose(a, b, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(f"value mismatch at {i}: {a!r} != {b!r}")


def candidate_points(function):
    dimension = function.dimension
    x_star = list(function.spec.known_global_minimizer)
    zero = [0.0] * dimension
    pattern = [0.25 if i % 2 == 0 else -0.75 for i in range(dimension)]
    return [x_star, zero, pattern]


def check_optima(suite, *, tolerance=20.0):
    for index in range(len(suite)):
        function = suite.function(index)
        x_star = list(function.spec.known_global_minimizer)
        value = function.evaluate([x_star])[0]
        expected = function.spec.known_global_value
        if not math.isclose(value, expected, rel_tol=tolerance, abs_tol=tolerance):
            raise AssertionError(
                f"optimum mismatch for function {index}: {value!r} != {expected!r}"
            )


def check_function_yaml_roundtrip(function, path):
    points = candidate_points(function)
    before = function.evaluate(points)
    function.export_spec(path)

    imported_spec = load_function_spec_yaml(path)
    imported_function = BenchmarkFunction(imported_spec)
    after = imported_function.evaluate(points)
    assert_close_sequence(after, before)


def check_suite_yaml_roundtrip(suite, path):
    indices = [0, 36, len(suite) - 1]
    before = {
        index: suite.function(index).evaluate(candidate_points(suite.function(index)))
        for index in indices
    }
    suite.export_manifest(path)

    imported_suite = BenchmarkSuite(str(path), suite.dimension)
    for index in indices:
        function = imported_suite.function(index)
        after = function.evaluate(candidate_points(function))
        assert_close_sequence(after, before[index])


def main():
    dimension = 3
    default_suite_path = Path(__file__).resolve().parents[1] / "BenchmarkSuites" / "default_suite.yaml"
    optimum_suite_spec = load_suite_spec_yaml(default_suite_path)
    optimum_suite_spec.requested_number_of_functions = 36
    optimum_suite = BenchmarkSuite(optimum_suite_spec, 2)
    check_optima(optimum_suite)

    roundtrip_suite = BenchmarkSuite(
        SuiteSpec(
            requested_number_of_functions=100,
            max_components=4,
            master_seed=123,
        ),
        dimension,
    )

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        check_function_yaml_roundtrip(roundtrip_suite.function(36), tmpdir / "function.yaml")
        check_suite_yaml_roundtrip(roundtrip_suite, tmpdir / "suite.yaml")


if __name__ == "__main__":
    main()
