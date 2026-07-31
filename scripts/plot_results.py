#!/usr/bin/env python3
"""Plot FuncCraft experiment results.

This script creates two figures for each experiment block:

1. average transformed relative error as a function of maxevals;
2. fraction of solved problems as a function of maxevals.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


TREATMENT_ORDER = {
    "coord": ["NONE", "ROT", "AFF"],
    "value": ["NONE", "POWER_P01", "POWER_P3", "HUBER", "OSC", "SPTHRESH"],
    "composition": ["CPM_SUM", "CPM_PMEAN", "CPM_LWELL", "CPM_LEX", "DPM_SOFTMAX", "DPM_BGSOFTMAX"],
}

DISPLAY_LABELS = {
    "NONE": "none",
    "ROT": "rotation",
    "AFF": "affine",
    "POWER_P01": "power, p=0.1",
    "POWER_P3": "power, p=3",
    "HUBER": "huber",
    "OSC": "relative-modulation",
    "SPTHRESH": "softplus-threshold",
    "CPM_SUM": "cpm-wsum",
    "CPM_PMEAN": "cpm-power-mean",
    "CPM_LWELL": "cpm-level-well",
    "CPM_LEX": "cpm-lexicographic",
    "DPM_SOFTMAX": "dpm-softmax",
    "DPM_BGSOFTMAX": "dpm-bgsoftmax",
}

EXPECTED_FUNCTION_COUNTS = {
    "coord": 34,
    "value": 34,
    "composition": 50,
}

RESULT_RE = re.compile(
    r"^(?P<algo>.+)_(?P<dim>\d+)D_(?P<maxevals>\d+)_(?P<experiment>coord|value|composition)_(?P<treatment>.+)\.txt$"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", nargs="?", default="results", help="Directory containing raw result .txt files.")
    parser.add_argument("--format", default="pdf", choices=["pdf", "png", "svg"], help="Output figure format.")
    parser.add_argument("--fstar", type=float, default=100.0, help="Known optimum value and denominator for relative error.")
    parser.add_argument("--tol", type=float, default=1.0e-6, help="Tolerance for abs(f_best - fstar) / fstar used to mark one run as solved.")
    return parser.parse_args()


def ordered_treatments(values: list[str], experiment: str) -> list[str]:
    preferred = TREATMENT_ORDER.get(experiment, [])
    return [v for v in preferred if v in values] + sorted(v for v in values if v not in preferred)


def load_result_matrix(path: Path) -> np.ndarray:
    data = np.loadtxt(path)
    if data.ndim == 0:
        return data.reshape(1, 1)
    if data.ndim == 1:
        return data.reshape(1, -1)
    return data


def select_expected_functions(data: np.ndarray, experiment: str, path: Path) -> np.ndarray:
    expected = EXPECTED_FUNCTION_COUNTS[experiment]
    if data.shape[1] < expected:
        raise SystemExit(
            f"{path} has {data.shape[1]} function columns, but {experiment} plots require {expected}."
        )
    return data[:, :expected]


def load_raw_summary(result_dir: Path, fstar: float, tol: float) -> pd.DataFrame:
    rows: list[dict[str, object]] = []
    for path in sorted(result_dir.glob("*.txt")):
        if "_manifest_" in path.name:
            continue
        match = RESULT_RE.match(path.name)
        if not match:
            continue

        data = load_result_matrix(path)
        experiment = match.group("experiment")
        data = select_expected_functions(data, experiment, path)
        relative_error = np.abs(data - fstar) / abs(fstar)
        tre = relative_error / (1.0 + relative_error)
        run_solved = relative_error < tol
        function_solved = np.mean(run_solved, axis=0) >= 0.5

        rows.append(
            {
                "file": path.name,
                "algo": match.group("algo"),
                "dimension": int(match.group("dim")),
                "maxevals": int(match.group("maxevals")),
                "experiment": experiment,
                "treatment": match.group("treatment").replace("-", "_"),
                "avg_tre": float(np.mean(tre)),
                "solved_fraction": float(np.mean(function_solved)),
            }
        )

    if not rows:
        raise SystemExit(f"No raw result files found in {result_dir}")
    return pd.DataFrame(rows)


def plot_metric(
    df: pd.DataFrame,
    experiment: str,
    metric: str,
    ylabel: str,
    output_path: Path,
) -> None:
    sub = df[df["experiment"] == experiment].copy()
    treatments = ordered_treatments(sorted(sub["treatment"].unique()), experiment)

    fig, ax = plt.subplots(figsize=(5.8, 3.8), constrained_layout=True)
    for treatment in treatments:
        line = sub[sub["treatment"] == treatment].sort_values("maxevals")
        ax.plot(
            line["maxevals"],
            line[metric],
            marker="o",
            linewidth=1.8,
            markersize=4.5,
            label=DISPLAY_LABELS.get(treatment, treatment),
        )

    ax.set_xscale("log")
    ax.set_xlabel("Max evaluations")
    ax.set_ylabel(ylabel)
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(frameon=False, fontsize=8)

    if metric == "solved_fraction":
        ax.set_ylim(-0.03, 1.03)
    else:
        ax.set_yscale("log")

    fig.savefig(output_path)
    plt.close(fig)


def main() -> int:
    args = parse_args()
    result_dir = Path(args.result_dir)
    fig_dir = result_dir / "figures"
    fig_dir.mkdir(parents=True, exist_ok=True)

    summary = load_raw_summary(result_dir, args.fstar, args.tol)

    for experiment in ["coord", "value", "composition"]:
        if experiment not in set(summary["experiment"]):
            continue

        plot_metric(
            summary,
            experiment,
            "avg_tre",
            "Average transformed relative error",
            fig_dir / f"experiment_{experiment}_tre.{args.format}",
        )
        plot_metric(
            summary,
            experiment,
            "solved_fraction",
            "Fraction of solved problems",
            fig_dir / f"experiment_{experiment}_solved_fraction.{args.format}",
        )

    print(f"Wrote figures to {fig_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
