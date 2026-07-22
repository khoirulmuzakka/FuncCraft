from __future__ import annotations

import argparse
import statistics
import sys
from pathlib import Path


DEFAULT_TOLERANCE = 1.0e-12


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


def summarize_differences(tables, tolerance):
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
        for point_index in range(point_count):
            values = [row[point_index] for row in value_rows]
            diffs.append(max(values) - min(values))

        min_diff = min(diffs)
        median_diff = statistics.median(diffs)
        max_diff = max(diffs)
        failed = max_diff > tolerance
        if failed:
            failures.append(function_index)
        reports.append((function_index, min_diff, median_diff, max_diff, failed))

    return platforms, reports, failures


def print_report(platforms, reports, failures, tolerance):
    print("FuncCraft Cross-Platform Value Comparison")
    print("=" * 48)
    print(f"Platforms: {', '.join(platforms)}")
    print(f"Tolerance: {tolerance:.3e}")
    print(f"Functions: {len(reports)}")
    print()
    print(f"{'Function':>8}  {'min_diff':>14}  {'median_diff':>14}  {'max_diff':>14}  Status")
    print("-" * 72)
    for function_index, min_diff, median_diff, max_diff, failed in reports:
        status = "FAIL" if failed else "OK"
        print(
            f"F{function_index + 1:<7d}  "
            f"{min_diff:14.6e}  "
            f"{median_diff:14.6e}  "
            f"{max_diff:14.6e}  "
            f"{status}"
        )
    print("-" * 72)
    print(f"Failed functions: {len(failures)} / {len(reports)}")
    if failures:
        shown = ", ".join(f"F{index + 1}" for index in failures[:25])
        suffix = " ..." if len(failures) > 25 else ""
        print(f"Failures: {shown}{suffix}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compare FuncCraft value tables across platforms.")
    parser.add_argument("files", nargs="+", type=Path, help="values_<platform>.txt files")
    parser.add_argument("--tolerance", type=float, default=DEFAULT_TOLERANCE)
    args = parser.parse_args(argv)

    if len(args.files) < 2:
        parser.error("at least two value files are required")

    tables = [read_table(path) for path in args.files]
    platforms, reports, failures = summarize_differences(tables, args.tolerance)
    print_report(platforms, reports, failures, args.tolerance)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
