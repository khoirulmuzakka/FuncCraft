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
from typing import Callable


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


@dataclass(frozen=True)
class ResultSummary:
    fun: float
    nfev: int


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


def normalize_algo_name(algo: str) -> str:
    return "".join(ch for ch in algo.casefold() if ch.isalnum())


def initial_guess_sampler(
    bounds: list[tuple[float, float]],
    seed: int,
    index: int,
) -> Callable[[], list[float]]:
    mixed_seed = seed ^ (0x9E3779B97F4A7C15 + index)
    rng = random.Random(mixed_seed)

    def sample() -> list[float]:
        return [low + (high - low) * rng.random() for low, high in bounds]

    return sample


def origin_then_random_sampler(
    bounds: list[tuple[float, float]],
    seed: int,
    index: int,
) -> Callable[[], list[float]]:
    sampler = initial_guess_sampler(bounds, seed, index)
    first_call = True

    def sample() -> list[float]:
        nonlocal first_call
        if first_call:
            first_call = False
            return [0.0] * len(bounds)
        return sampler()

    return sample


def unit_bounds(dimension: int) -> list[tuple[float, float]]:
    return [(-1.0, 1.0)] * dimension


def function_domain_bounds(function) -> tuple[list[float], list[float]] | None:
    domain = getattr(function, "domain", None)
    if domain is None:
        return None
    lower = getattr(domain, "lower_bound", getattr(domain, "lower", None))
    upper = getattr(domain, "upper_bound", getattr(domain, "upper", None))
    if lower is None or upper is None:
        return None
    return list(lower), list(upper)


def map_unit_point_to_domain(
    point: list[float],
    lower: list[float] | None,
    upper: list[float] | None,
) -> list[float]:
    if lower is None or upper is None:
        return list(point)
    mapped: list[float] = []
    for value, lo, hi in zip(point, lower, upper):
        if hi == lo:
            mapped.append(lo)
        else:
            mapped.append(lo + 0.5 * (value + 1.0) * (hi - lo))
    return mapped


def map_unit_points_to_domain(
    points: list[list[float]],
    lower: list[float] | None,
    upper: list[float] | None,
) -> list[list[float]]:
    return [map_unit_point_to_domain(point, lower, upper) for point in points]


def sequence_to_list(value):
    if hasattr(value, "tolist"):
        return value.tolist()
    return list(value)


def is_vector(value) -> bool:
    return not (hasattr(value, "__iter__") and not isinstance(value, (str, bytes)))


def unit_domain_batch_objective(function) -> Callable[[list[list[float]]], list[float]]:
    lower_upper = function_domain_bounds(function)
    lower = lower_upper[0] if lower_upper is not None else None
    upper = lower_upper[1] if lower_upper is not None else None

    def objective(xs) -> list[float]:
        xs = sequence_to_list(xs)
        if len(xs) == 0:
            return []
        if is_vector(xs[0]):
            unit_points = [sequence_to_list(xs)]
        else:
            unit_points = [sequence_to_list(point) for point in xs]
        points = map_unit_points_to_domain(unit_points, lower, upper)
        return function.evaluate(points)

    return objective


def unit_domain_objective(function) -> Callable[[list[float] | list[list[float]]], float | list[float]]:
    batch_objective = unit_domain_batch_objective(function)

    def objective(x):
        x = sequence_to_list(x)
        if len(x) == 0:
            return []
        if is_vector(x[0]):
            return float(batch_objective([x])[0])
        return batch_objective(x)

    return objective


def single_point_objective(function) -> Callable[[list[float]], float]:
    lower_upper = function_domain_bounds(function)
    lower = lower_upper[0] if lower_upper is not None else None
    upper = lower_upper[1] if lower_upper is not None else None

    def objective(x: list[float]) -> float:
        point = map_unit_point_to_domain(sequence_to_list(x), lower, upper)
        return float(function.evaluate([point])[0])

    return objective


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
    return initial_guess_sampler(bounds, seed, index)()


def make_result_summary(fun: float, nfev: int) -> ResultSummary:
    return ResultSummary(fun=float(fun), nfev=int(nfev))


def run_minionpy(
    function,
    bounds: list[tuple[float, float]],
    config: RunConfig,
    index: int,
):
    try:
        import minionpy as mpy
    except ImportError as exc:
        raise RuntimeError(
            "MinionPy is not installed. Install it with: python -m pip install minionpy"
        ) from exc

    bounds = unit_bounds(config.dimension)
    x0 = [[0.0] * config.dimension]
    optimizer = mpy.Minimizer(
        func=unit_domain_batch_objective(function),
        bounds=bounds,
        x0=x0,
        algo=config.algo,
        maxevals=config.max_evals,
        callback=None,
        seed=config.seed,
        options={
            "convergence_tol": 1e-8,
            "population_size": config.population_size,
        },
    )
    return optimizer.optimize()


def run_pycma(
    function,
    bounds: list[tuple[float, float]],
    config: RunConfig,
    index: int,
    variant: str,
) -> ResultSummary:
    try:
        import cma
    except ImportError as exc:
        raise RuntimeError(
            "pycma is not installed. Install it with: python -m pip install cma"
        ) from exc

    bounds = unit_bounds(config.dimension)
    lower = [low for low, _ in bounds]
    upper = [high for _, high in bounds]
    width = [high - low for low, high in bounds if high > low]
    sigma0 = 0.3
    normalized = normalize_algo_name(variant)
    active = normalized in {"acmaes", "abipop", "ipop"}
    use_bipop = normalized == "abipop"
    restarts = math.inf if normalized in {"abipop", "ipop"} else 0
    x0 = origin_then_random_sampler(bounds, config.seed, index)
    batch_objective = unit_domain_batch_objective(function)
    options = {
        "bounds": [lower, upper],
        "seed": config.seed,
        "verb_time": 0,
        "verbose": -9,
        "maxfevals": config.max_evals,
        "CMA_active": active,
    }
    if config.population_size > 0:
        options["popsize"] = config.population_size

    _, es = cma.fmin2(
        None,
        x0,
        sigma0,
        options,
        restarts=restarts,
        bipop=use_bipop,
        parallel_objective=batch_objective,
    )
    return make_result_summary(es.result.fbest, es.result.evaluations)


def run_scipy(
    function,
    bounds: list[tuple[float, float]],
    config: RunConfig,
    index: int,
    variant: str,
) -> ResultSummary:
    try:
        import scipy.optimize as spo
    except ImportError as exc:
        raise RuntimeError(
            "SciPy is not installed. Install it with: python -m pip install scipy"
        ) from exc

    bounds = unit_bounds(config.dimension)
    normalized = normalize_algo_name(variant)
    x0 = [0.0] * config.dimension
    objective = single_point_objective(function)

    if normalized == "de":
        if config.population_size > 0:
            popsize = max(1, math.ceil(config.population_size / max(1, config.dimension)))
        else:
            popsize = 5
        maxiter = max(0, config.max_evals // (popsize * max(1, config.dimension)) - 1)
        result = spo.differential_evolution(
            objective,
            bounds,
            x0=x0,
            seed=config.seed,
            popsize=popsize,
            maxiter=maxiter,
            polish=False,
            disp=False,
        )
        return make_result_summary(result.fun, result.nfev)

    if normalized == "neldermead":
        result = spo.minimize(
            objective,
            x0,
            method="Nelder-Mead",
            bounds=bounds,
            options={
                "maxiter": config.max_evals,
                "maxfev": config.max_evals,
                "xatol": 0.0,
                "fatol": 0.0,
                "disp": False,
            },
        )
        return make_result_summary(result.fun, result.nfev)

    if normalized == "lbfgsb":
        result = spo.minimize(
            objective,
            x0,
            method="L-BFGS-B",
            bounds=bounds,
            options={
                "maxiter": config.max_evals,
                "maxfun": config.max_evals,
                "disp": False,
            },
        )
        return make_result_summary(result.fun, result.nfev)

    raise ValueError(
        "unsupported scipy algorithm variant: "
        f"{variant!r}; use scipy_DE, scipy_NelderMead, or scipy_L_bfgs_b"
    )


def run_algorithm(
    function,
    bounds: list[tuple[float, float]],
    config: RunConfig,
    index: int,
):
    normalized = normalize_algo_name(config.algo)
    if normalized.startswith("pycma"):
        variant = normalized[len("pycma") :]
        if not variant:
            variant = "cmaes"
        return run_pycma(function, bounds, config, index, variant)
    if normalized.startswith("scipy"):
        variant = normalized[len("scipy") :]
        if not variant:
            raise ValueError(
                "scipy algorithms require a suffix: scipy_DE, scipy_NelderMead, or scipy_L_bfgs_b"
            )
        return run_scipy(function, bounds, config, index, variant)
    return run_minionpy(function, bounds, config, index)


def main(argv: list[str]) -> int:
    try:
        config = parse_cli(argv)

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
                result = run_algorithm(function, bounds, config, index)
                error = abs(float(result.fun) - function.get_fopt())
                ok = math.isfinite(float(result.fun))
                failed += 0 if ok else 1

                print(
                    f"{index:<6}"
                    f"{fields[0]:<14}"
                    f"{fields[1]:<14}"
                    f"{fields[2]:<12}"
                    f"{truncate_with_ellipsis(fields[3], 29):<30}"
                    f"{float(result.fun):>16.6e}"
                    f"{int(result.nfev):>16}"
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
