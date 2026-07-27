"""Minimize FuncCraft 2026 v1 suite functions with MinionPy.

Usage:
    python examples/run_minimize.py [algo] [dimension] [maxevals] [population_size] [seed] [low] [high]

The positional arguments mirror the C++ ``run_minimize`` executable.
Function indices are one-based.
"""

from __future__ import annotations

import math
from pathlib import Path
import random
import sys
import time
from dataclasses import dataclass


try:
    import funccraft as fc
except ImportError:
    REPO_ROOT = Path(__file__).resolve().parents[1]
    if str(REPO_ROOT) not in sys.path:
        sys.path.insert(0, str(REPO_ROOT))
    import funccraft as fc


USAGE = (
    "Usage: run_minimize [algo] [dimension] [maxevals] "
    "[population_size] [seed] [low] [high]"
)


@dataclass(frozen=True)
class RunConfig:
    algo: str = "ARRDE"
    dimension: int = 10
    max_evals: int = 10000
    population_size: int = 0
    seed: int = 1
    low: int = 1
    high: int = 32


def is_integer_arg(text: str) -> bool:
    if not text:
        return False
    try:
        int(text, 10)
    except ValueError:
        return False
    return True


def parse_int_arg(text: str, name: str) -> int:
    try:
        return int(text, 10)
    except ValueError as exc:
        raise ValueError(f"{name} must be an integer") from exc


def parse_nonnegative_int_arg(text: str, name: str) -> int:
    value = parse_int_arg(text, name)
    if value < 0:
        raise ValueError(f"{name} must be a non-negative integer")
    return value


def parse_cli(argv: list[str]) -> RunConfig:
    algo = "ARRDE"
    arg_index = 0
    if argv and not is_integer_arg(argv[0]):
        algo = argv[0]
        arg_index = 1

    values = argv[arg_index:]
    if len(values) > 6:
        raise ValueError("too many arguments")

    dimension = parse_int_arg(values[0], "dimension") if len(values) > 0 else 10
    max_evals = parse_nonnegative_int_arg(values[1], "maxevals") if len(values) > 1 else 10000
    population_size = parse_nonnegative_int_arg(values[2], "population_size") if len(values) > 2 else 0
    seed = parse_nonnegative_int_arg(values[3], "seed") if len(values) > 3 else 1
    low = parse_int_arg(values[4], "low") if len(values) > 4 else 1
    high = parse_int_arg(values[5], "high") if len(values) > 5 else 32

    if dimension < 1:
        raise ValueError("dimension must be at least 1")
    if max_evals < 1:
        raise ValueError("maxevals must be at least 1")

    return RunConfig(
        algo=algo,
        dimension=dimension,
        max_evals=max_evals,
        population_size=population_size,
        seed=seed,
        low=low,
        high=high,
    )


def split_class_label(label: str) -> tuple[str, str, str, str]:
    fields = {"C": "-", "P": "-", "T": "-", "G": "-"}
    for token in label.split(";"):
        key, sep, value = token.partition("=")
        if sep and key in fields:
            fields[key] = value
    return fields["C"], fields["P"], fields["T"], fields["G"]


def truncate_with_ellipsis(text: str, max_len: int) -> str:
    if len(text) <= max_len:
        return text
    if max_len <= 3:
        return text[:max_len]
    return text[: max_len - 3] + "..."


def initial_guess(bounds: list[tuple[float, float]], seed: int, index: int) -> list[float]:
    mixed_seed = seed ^ (0x9E3779B97F4A7C15 + index)
    rng = random.Random(mixed_seed)
    return [low + (high - low) * rng.random() for low, high in bounds]


def main(argv: list[str]) -> int:
    try:
        config = parse_cli(argv)

        try:
            import minionpy as mpy
        except ImportError as exc:
            raise RuntimeError(
                "MinionPy is not installed. Install it with: python -m pip install minionpy"
            ) from exc

        year = 2026
        version = 1
        collection = fc.SuiteCollection(year, version)
        suite = collection.benchmark_suite(config.dimension)

        if config.low < 1 or config.high < 1:
            raise ValueError("function indices are one-based; low and high must be at least 1")
        if config.high < config.low:
            raise ValueError("high must be greater than or equal to low")
        if config.low > suite.size or config.high > suite.size:
            raise IndexError("requested function index range is outside the suite size")

        processed_functions = config.high - config.low + 1

        print(USAGE)
        print(f"Suite collection: {collection.name} ({collection.year}_v{collection.version})")
        print(
            f"Suite size: {suite.size}, dimension: {suite.dimension}, "
            f"index_range: [{config.low}, {config.high}], "
            f"processed_functions: {processed_functions}, algo: {config.algo}, "
            f"maxevals: {config.max_evals}, population_size: {config.population_size}, "
            f"seed: {config.seed}\n"
        )

        print(
            f"{'idx':<6}{'C':<14}{'P':<14}{'T':<12}{'G':<30}"
            f"{'best_f':>16}{'nfev':>16}{'|best-fopt|':>16}"
        )
        print("-" * 110)

        start_time = time.perf_counter()
        failed = 0
        for index in range(config.low, config.high + 1):
            function = suite.function(index)
            try:
                fields = split_class_label(function.label)
                domain = function.domain
                bounds = list(zip(domain.lower_bound, domain.upper_bound))
                x0 = initial_guess(bounds, config.seed, index)

                optimizer = mpy.Minimizer(
                    func=function.evaluate,
                    bounds=bounds,
                    x0=[x0],
                    algo=config.algo,
                    maxevals=config.max_evals,
                    callback=None,
                    seed=config.seed,
                    options={
                        "convergence_tol": 0.0,
                        "population_size": config.population_size,
                    },
                )
                result = optimizer.optimize()
                error = abs(result.fun - function.get_fopt())
                ok = math.isfinite(result.fun)
                failed += 0 if ok else 1

                print(
                    f"{index:<6}"
                    f"{fields[0]:<14}"
                    f"{fields[1]:<14}"
                    f"{fields[2]:<12}"
                    f"{truncate_with_ellipsis(fields[3], 29):<30}"
                    f"{result.fun:>16.6e}"
                    f"{result.nfev:>16}"
                    f"{error:>16.3e}"
                )
            except Exception as exc:
                print(
                    f"run_minimize failed at index {index} "
                    f"label={function.label}: {exc}",
                    file=sys.stderr,
                )
                raise

        elapsed = time.perf_counter() - start_time
        print(f"\nTotal elapsed time: {elapsed:.3f} seconds")

        return 0 if failed == 0 else 1
    except Exception as exc:
        print(f"run_minimize failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
