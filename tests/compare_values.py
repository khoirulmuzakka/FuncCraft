from __future__ import annotations

import argparse
import statistics
import sys
from pathlib import Path


DEFAULT_TOLERANCE = 1.0e-12
DEFAULT_ABSOLUTE_TOLERANCE = 1.0e-9


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


def summarize_differences(tables, relative_tolerance, absolute_tolerance):
    platforms = [platform for platform, _ in tables]
    reference_indices = sorted(tables[0][1])
    failures = []
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

        diffs = []
        relative_diffs = []
        allowed_tolerances = []
        for point_index in range(point_count):
            values = [row[point_index] for row in value_rows]
            min_value = min(values)
            max_value = max(values)
            scale = max(abs(value) for value in values)
            diff = max_value - min_value
            diffs.append(diff)
            relative_diffs.append(0.0 if scale == 0.0 else diff / scale)
            allowed_tolerances.append(max(absolute_tolerance, relative_tolerance * scale))

        min_diff = min(diffs)
        median_diff = statistics.median(diffs)
        max_diff = max(diffs)
        max_relative_diff = max(relative_diffs)
        max_allowed = max(allowed_tolerances)
        failed = any(diff > allowed for diff, allowed in zip(diffs, allowed_tolerances))
        if failed:
            failures.append(function_index)
        reports.append((function_index, min_diff, median_diff, max_diff, max_relative_diff, max_allowed, failed))

    return platforms, reports, failures


def print_report(platforms, reports, failures, relative_tolerance, absolute_tolerance):
    print("FuncCraft Cross-Platform Value Comparison")
    print("=" * 48)
    print(f"Platforms: {', '.join(platforms)}")
    print(f"Relative tolerance: {relative_tolerance:.3e}")
    print(f"Absolute tolerance: {absolute_tolerance:.3e}")
    print(f"Functions: {len(reports)}")
    print()
    print(
        f"{'Function':>8}  "
        f"{'min_abs':>14}  "
        f"{'median_abs':>14}  "
        f"{'max_abs':>14}  "
        f"{'max_rel':>14}  "
        f"{'max_allowed':>14}  "
        "Status"
    )
    print("-" * 104)
    for function_index, min_diff, median_diff, max_diff, max_relative_diff, max_allowed, failed in reports:
        status = "FAIL" if failed else "OK"
        print(
            f"F{function_index + 1:<7d}  "
            f"{min_diff:14.6e}  "
            f"{median_diff:14.6e}  "
            f"{max_diff:14.6e}  "
            f"{max_relative_diff:14.6e}  "
            f"{max_allowed:14.6e}  "
            f"{status}"
        )
    print("-" * 104)
    print(f"Failed functions: {len(failures)} / {len(reports)}")
    if failures:
        shown = ", ".join(f"F{index + 1}" for index in failures[:25])
        suffix = " ..." if len(failures) > 25 else ""
        print(f"Failures: {shown}{suffix}")


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
        "--absolute-tolerance",
        type=float,
        default=DEFAULT_ABSOLUTE_TOLERANCE,
        help="absolute tolerance floor used near zero",
    )
    args = parser.parse_args(argv)

    if len(args.files) < 2:
        parser.error("at least two value files are required")

    tables = [read_table(path) for path in args.files]
    platforms, reports, failures = summarize_differences(tables, args.tolerance, args.absolute_tolerance)
    print_report(platforms, reports, failures, args.tolerance, args.absolute_tolerance)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
