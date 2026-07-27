from __future__ import annotations

import argparse
import statistics
import sys
from pathlib import Path


DEFAULT_TOLERANCE = 1.0e-8
DEFAULT_POINT_AGREEMENT = 0.95
DEFAULT_FUNCTION_AGREEMENT = 0.95
DEFAULT_STRICT_FUNCTION_COUNT = 34
DEFAULT_STRICT_POINT_AGREEMENT = 1.0


def read_table(path: Path):
    platform = path.stem.replace("values_", "")
    rows = {}
    with path.open("r", encoding="utf-8") as handle:
        for line_number, raw_line in enumerate(handle, 1):
            line = raw_line.strip()
            if not line:
                continue
            if line.startswith("#"):
                if line.startswith("# platform:"):
                    platform = line.split(":", 1)[1].strip()
                continue

            parts = line.split()
            if len(parts) < 2:
                raise ValueError(f"{path}:{line_number}: expected function index and values")
            function_index = int(parts[0])
            rows[function_index] = [float(value) for value in parts[1:]]

    if not rows:
        raise ValueError(f"{path}: no value rows found")
    return platform, rows


def summarize_differences(
    tables,
    tolerance,
    point_agreement_threshold,
    strict_function_count=DEFAULT_STRICT_FUNCTION_COUNT,
    strict_point_agreement_threshold=DEFAULT_STRICT_POINT_AGREEMENT,
):
    platforms = [platform for platform, _ in tables]
    reference_indices = sorted(tables[0][1])
    failed_functions = []
    reports = []

    for platform, rows in tables[1:]:
        if sorted(rows) != reference_indices:
            raise ValueError(f"{platform}: function index set differs from {platforms[0]}")

    for function_index in reference_indices:
        value_rows = [rows[function_index] for _, rows in tables]
        point_count = len(value_rows[0])
        for platform, values in zip(platforms, value_rows):
            if len(values) != point_count:
                raise ValueError(
                    f"{platform}: function {function_index} has {len(values)} values, expected {point_count}"
                )

        relative_diffs = []
        for point_index in range(point_count):
            values = [row[point_index] for row in value_rows]
            min_value = min(values)
            max_value = max(values)
            scale = max(abs(value) for value in values)
            diff = max_value - min_value
            relative_diffs.append(0.0 if scale == 0.0 else diff / scale)

        min_relative_diff = min(relative_diffs)
        median_relative_diff = statistics.median(relative_diffs)
        max_relative_diff = max(relative_diffs)
        within_tolerance = sum(diff <= tolerance for diff in relative_diffs)
        agreement = within_tolerance / point_count
        required_agreement = (
            strict_point_agreement_threshold
            if function_index <= strict_function_count
            else point_agreement_threshold
        )
        failed = agreement < required_agreement
        if failed:
            failed_functions.append(function_index)
        reports.append(
            (
                function_index,
                min_relative_diff,
                median_relative_diff,
                max_relative_diff,
                agreement,
                required_agreement,
                failed,
            )
        )

    return platforms, reports, failed_functions


def print_report(
    platforms,
    reports,
    failed_functions,
    tolerance,
    point_agreement_threshold,
    function_agreement_threshold,
    strict_function_count,
    strict_point_agreement_threshold,
):
    strict_reports = [row for row in reports if row[0] <= strict_function_count]
    statistical_reports = [row for row in reports if row[0] > strict_function_count]
    strict_failed = [row[0] for row in strict_reports if row[-1]]
    statistical_failed = [row[0] for row in statistical_reports if row[-1]]
    statistical_passed = len(statistical_reports) - len(statistical_failed)
    statistical_function_agreement = (
        1.0 if not statistical_reports else statistical_passed / len(statistical_reports)
    )
    suite_failed = bool(strict_failed) or statistical_function_agreement < function_agreement_threshold

    print("FuncCraft Cross-Platform Value Comparison")
    print("=" * 48)
    print(f"Platforms: {', '.join(platforms)}")
    print(f"Relative tolerance: {tolerance:.3e}")
    if strict_function_count > 0:
        print(
            f"Strict prefix: first {strict_function_count} functions require "
            f"{strict_point_agreement_threshold:.1%} point agreement"
        )
    print(f"Statistical point agreement per function: {point_agreement_threshold:.1%}")
    print(f"Required passing statistical functions: {function_agreement_threshold:.1%}")
    print(f"Functions: {len(reports)}")
    print()
    print(
        f"{'Function':>8}  "
        f"{'min_rel':>14}  "
        f"{'median_rel':>14}  "
        f"{'max_rel':>14}  "
        f"{'agreement':>10}  "
        f"{'required':>10}  "
        "Status"
    )
    print("-" * 98)
    for function_index, min_diff, median_diff, max_diff, agreement, required_agreement, failed in reports:
        status = "FAIL" if failed else "OK"
        print(
            f"F{function_index:<7d}  "
            f"{min_diff:14.6e}  "
            f"{median_diff:14.6e}  "
            f"{max_diff:14.6e}  "
            f"{agreement:9.2%}  "
            f"{required_agreement:9.2%}  "
            f"{status}"
        )
    print("-" * 98)
    if strict_function_count > 0:
        print(f"Strict prefix failures: {len(strict_failed)} / {len(strict_reports)}")
    print(
        f"Passing statistical functions: {statistical_passed} / {len(statistical_reports)} "
        f"({statistical_function_agreement:.2%})"
    )
    print(f"Failed statistical functions: {len(statistical_failed)} / {len(statistical_reports)}")
    print(f"Overall status: {'FAIL' if suite_failed else 'OK'}")
    if failed_functions:
        shown = ", ".join(f"F{index}" for index in failed_functions[:25])
        suffix = " ..." if len(failed_functions) > 25 else ""
        print(f"Failures: {shown}{suffix}")
    return suite_failed


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compare FuncCraft value tables across platforms.")
    parser.add_argument("files", nargs="+", type=Path, help="values_<platform>.txt files")
    parser.add_argument(
        "--tolerance",
        type=float,
        default=DEFAULT_TOLERANCE,
        help="relative tolerance used for cross-platform comparisons",
    )
    parser.add_argument(
        "--point-agreement",
        type=float,
        default=DEFAULT_POINT_AGREEMENT,
        help="minimum fraction of points in each function that must be within tolerance",
    )
    parser.add_argument(
        "--function-agreement",
        type=float,
        default=DEFAULT_FUNCTION_AGREEMENT,
        help="minimum fraction of functions that must satisfy point agreement",
    )
    parser.add_argument(
        "--strict-function-count",
        type=int,
        default=DEFAULT_STRICT_FUNCTION_COUNT,
        help="number of leading functions that must satisfy the stricter point agreement",
    )
    parser.add_argument(
        "--strict-point-agreement",
        type=float,
        default=DEFAULT_STRICT_POINT_AGREEMENT,
        help="minimum point agreement for the leading strict functions",
    )
    args = parser.parse_args(argv)

    if len(args.files) < 2:
        parser.error("at least two value files are required")
    if not 0.0 <= args.point_agreement <= 1.0:
        parser.error("--point-agreement must be between 0 and 1")
    if not 0.0 <= args.function_agreement <= 1.0:
        parser.error("--function-agreement must be between 0 and 1")
    if args.strict_function_count < 0:
        parser.error("--strict-function-count must be nonnegative")
    if not 0.0 <= args.strict_point_agreement <= 1.0:
        parser.error("--strict-point-agreement must be between 0 and 1")

    tables = [read_table(path) for path in args.files]
    platforms, reports, failed_functions = summarize_differences(
        tables,
        args.tolerance,
        args.point_agreement,
        args.strict_function_count,
        args.strict_point_agreement,
    )
    suite_failed = print_report(
        platforms,
        reports,
        failed_functions,
        args.tolerance,
        args.point_agreement,
        args.function_agreement,
        args.strict_function_count,
        args.strict_point_agreement,
    )
    return 1 if suite_failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
