#!/usr/bin/env python3
"""Analyze FuncCraft experiment result matrices.

Each result file is expected to contain one matrix:

    rows    independent runs
    columns function instances

The file name is expected to follow:

    ALGO_DIMD_MAXEVALS_EXPERIMENT_TREATMENT.txt

Manifest files produced by the C++ runner are ignored.
"""

from __future__ import annotations

import argparse
import csv
import math
import re
from pathlib import Path

import numpy as np


RESULT_RE = re.compile(
    r"^(?P<algo>.+)_(?P<dim>\d+)D_(?P<maxevals>\d+)_(?P<experiment>coord|value|composition)_(?P<treatment>.+)\.txt$"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", nargs="?", default="results", help="Directory containing result txt files.")
    parser.add_argument("--out", default=None, help="Output CSV path. Defaults to result_dir/summary.csv.")
    parser.add_argument("--tol", type=float, default=1.0e-8, help="Solved tolerance for f_best - f_star.")
    parser.add_argument("--fstar", type=float, default=0.0, help="Known optimum value used for all generated files.")
    return parser.parse_args()


def safe_log10_errors(errors: np.ndarray) -> np.ndarray:
    return np.log10(np.maximum(errors, 0.0) + 1.0e-16)


def summarize_file(path: Path, tol: float, fstar: float) -> dict[str, object] | None:
    match = RESULT_RE.match(path.name)
    if not match:
        return None

    data = np.loadtxt(path)
    if data.ndim == 0:
        data = data.reshape(1, 1)
    if data.ndim == 1:
        data = data.reshape(1, -1)

    errors = data - fstar
    log_errors = safe_log10_errors(errors)
    solved = errors <= tol

    return {
        "file": path.name,
        "algo": match.group("algo"),
        "dimension": int(match.group("dim")),
        "maxevals": int(match.group("maxevals")),
        "experiment": match.group("experiment"),
        "treatment": match.group("treatment"),
        "runs": int(data.shape[0]),
        "functions": int(data.shape[1]),
        "mean_error": float(np.mean(errors)),
        "median_error": float(np.median(errors)),
        "mean_log10_error": float(np.mean(log_errors)),
        "median_log10_error": float(np.median(log_errors)),
        "solved_fraction": float(np.mean(solved)),
        "best_error": float(np.min(errors)),
        "worst_error": float(np.max(errors)),
    }


def write_summary(rows: list[dict[str, object]], output_path: Path) -> None:
    fieldnames = [
        "file",
        "algo",
        "dimension",
        "maxevals",
        "experiment",
        "treatment",
        "runs",
        "functions",
        "mean_error",
        "median_error",
        "mean_log10_error",
        "median_log10_error",
        "solved_fraction",
        "best_error",
        "worst_error",
    ]
    with output_path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_per_function_summary(rows: list[dict[str, object]], result_dir: Path, tol: float, fstar: float) -> None:
    output_path = result_dir / "summary_by_function.csv"
    fieldnames = [
        "file",
        "algo",
        "dimension",
        "maxevals",
        "experiment",
        "treatment",
        "function",
        "mean_error",
        "median_error",
        "mean_log10_error",
        "median_log10_error",
        "solved_fraction",
    ]
    with output_path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            path = result_dir / str(row["file"])
            data = np.loadtxt(path)
            if data.ndim == 0:
                data = data.reshape(1, 1)
            if data.ndim == 1:
                data = data.reshape(1, -1)
            errors = data - fstar
            log_errors = safe_log10_errors(errors)
            solved = errors <= tol
            for function_index in range(data.shape[1]):
                writer.writerow(
                    {
                        "file": row["file"],
                        "algo": row["algo"],
                        "dimension": row["dimension"],
                        "maxevals": row["maxevals"],
                        "experiment": row["experiment"],
                        "treatment": row["treatment"],
                        "function": function_index,
                        "mean_error": float(np.mean(errors[:, function_index])),
                        "median_error": float(np.median(errors[:, function_index])),
                        "mean_log10_error": float(np.mean(log_errors[:, function_index])),
                        "median_log10_error": float(np.median(log_errors[:, function_index])),
                        "solved_fraction": float(np.mean(solved[:, function_index])),
                    }
                )


def main() -> int:
    args = parse_args()
    result_dir = Path(args.result_dir)
    output_path = Path(args.out) if args.out else result_dir / "summary.csv"

    rows: list[dict[str, object]] = []
    for path in sorted(result_dir.glob("*.txt")):
        if "_manifest_" in path.name:
            continue
        summary = summarize_file(path, args.tol, args.fstar)
        if summary is not None:
            rows.append(summary)

    if not rows:
        raise SystemExit(f"No result files found in {result_dir}")

    rows.sort(key=lambda r: (str(r["experiment"]), str(r["treatment"]), int(r["maxevals"])))
    write_summary(rows, output_path)
    write_per_function_summary(rows, result_dir, args.tol, args.fstar)

    print(f"Wrote {output_path}")
    print(f"Wrote {result_dir / 'summary_by_function.csv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
