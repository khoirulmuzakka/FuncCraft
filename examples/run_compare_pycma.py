"""Compare MinionPy CMA variants and pycma variants on FuncCraft F1-F70.

Outputs one file per algorithm, dimension, and evaluation budget:
    funccraft70_{algo}_{dimension}_{maxevals}.txt

Each output file is a 70 x 11 matrix. Rows are F1-F70 and columns are the
independent runs.
"""

from __future__ import annotations

import math
import os
import sys
import threading
import time
import ctypes
from concurrent.futures import ThreadPoolExecutor, as_completed
from contextlib import contextmanager
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

import funccraft as fc
from run_minimize import RunConfig, run_algorithm


ALGOS = [
    "cmaes",
    "acmaes",
    "bipopacmaes",
    "rcmaes",
    "pycma_cmaes",
    "pycma_acmaes",
    "pycma_ipop",
    "pycma_abipop",
]
DIMENSIONS = [5, 20, 40]
MAX_EVALS = [500, 1000, 5000, 10000, 20000, 50000, 100000, 500000, 1000000]
FUNCTION_INDICES = list(range(1, 71))
RUNS = list(range(1, 12))
PROGRESS_EVERY_JOBS = 100
THREAD_LOCAL = threading.local()


def thread_workers() -> int:
    override = os.environ.get("FUNCCRAFT_COMPARE_THREADS")
    if override:
        return max(1, int(override))
    return max(1, os.cpu_count() or 1)


def output_path(output_dir: Path, algo: str, dimension: int, max_evals: int) -> Path:
    safe_algo = "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in algo)
    return output_dir / f"funccraft70_{safe_algo}_{dimension}_{max_evals}.txt"


def total_jobs() -> int:
    return len(ALGOS) * len(DIMENSIONS) * len(MAX_EVALS) * len(FUNCTION_INDICES) * len(RUNS)


def format_elapsed(seconds: float) -> str:
    total_seconds = int(seconds)
    hours, remainder = divmod(total_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


def progress_line(started: float, finished: int, total: int) -> str:
    percentage = 100.0 * finished / total if total else 100.0
    return f"{format_elapsed(time.perf_counter() - started)}   {finished}/{total} total jobs   {percentage:5.1f}%"


def flush_c_stdio() -> None:
    for library in (None, "msvcrt"):
        try:
            ctypes.CDLL(library).fflush(None)
            return
        except Exception:
            pass


@contextmanager
def redirect_process_output(log_path: Path, *, restore: bool = True):
    log_path.parent.mkdir(parents=True, exist_ok=True)
    sys.stdout.flush()
    sys.stderr.flush()
    flush_c_stdio()
    stdout_fd = os.dup(1)
    stderr_fd = os.dup(2)
    with log_path.open("ab", buffering=0) as log_file:
        restore_windows_stdout = None
        restore_windows_stderr = None
        os.dup2(log_file.fileno(), 1)
        os.dup2(log_file.fileno(), 2)
        if os.name == "nt":
            kernel32 = ctypes.windll.kernel32
            restore_windows_stdout = kernel32.GetStdHandle(-11)
            restore_windows_stderr = kernel32.GetStdHandle(-12)
            import msvcrt

            kernel32.SetStdHandle(-11, msvcrt.get_osfhandle(1))
            kernel32.SetStdHandle(-12, msvcrt.get_osfhandle(2))
        try:
            yield
        finally:
            sys.stdout.flush()
            sys.stderr.flush()
            flush_c_stdio()
            if restore:
                os.dup2(stdout_fd, 1)
                os.dup2(stderr_fd, 2)
                if os.name == "nt":
                    kernel32.SetStdHandle(-11, restore_windows_stdout)
                    kernel32.SetStdHandle(-12, restore_windows_stderr)
            os.close(stdout_fd)
            os.close(stderr_fd)


def thread_local_suite(dimension: int):
    suites = getattr(THREAD_LOCAL, "suites", None)
    if suites is None:
        suites = {}
        THREAD_LOCAL.suites = suites
    if dimension not in suites:
        suites[dimension] = fc.SuiteCollection(2026, 1).benchmark_suite(dimension)
    return suites[dimension]


def run_one(
    algo: str,
    dimension: int,
    max_evals: int,
    function_index: int,
    run_number: int,
) -> tuple[int, int, float]:
    function = thread_local_suite(dimension).function(function_index)
    domain = function.domain
    bounds = list(zip(domain.lower_bound, domain.upper_bound))
    config = RunConfig(
        algo=algo,
        dimension=dimension,
        max_evals=max_evals,
        population_size=0,
        seed=run_number,
        low=function_index,
        high=function_index,
    )
    result = run_algorithm(function, bounds, config, function_index)
    return function_index, run_number, float(result.fun)


def write_matrix(path: Path, matrix: list[list[float]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        for row in matrix:
            handle.write(" ".join(f"{value:.17g}" for value in row))
            handle.write("\n")


def run_algo(algo: str, output_dir: Path, started: float, total: int, finished: int, console_write) -> int:
    next_report = ((finished // PROGRESS_EVERY_JOBS) + 1) * PROGRESS_EVERY_JOBS

    with ThreadPoolExecutor(max_workers=thread_workers()) as executor:
        for max_evals in MAX_EVALS:
            futures = {}
            results: dict[int, list[list[float]]] = {}
            for dimension in DIMENSIONS:
                results[dimension] = [
                    [math.nan for _ in RUNS]
                    for _ in FUNCTION_INDICES
                ]

            for dimension in DIMENSIONS:
                for function_index in FUNCTION_INDICES:
                    for run_number in RUNS:
                        future = executor.submit(
                            run_one,
                            algo,
                            dimension,
                            max_evals,
                            function_index,
                            run_number,
                        )
                        futures[future] = dimension

            for future in as_completed(futures):
                dimension = futures[future]
                function_index, run_number, value = future.result()
                results[dimension][function_index - 1][run_number - 1] = value
                finished += 1
                if finished >= next_report or finished == total:
                    console_write(progress_line(started, finished, total))
                    next_report = finished + PROGRESS_EVERY_JOBS

            for dimension in DIMENSIONS:
                path = output_path(output_dir, algo, dimension, max_evals)
                write_matrix(path, results[dimension])

    return finished


def main(argv: list[str]) -> int:
    output_dir = Path(argv[0]) if argv else Path.cwd()
    output_dir.mkdir(parents=True, exist_ok=True)

    workers = thread_workers()
    total = total_jobs()
    finished = 0
    started = time.perf_counter()
    console_fd = os.dup(1)

    def console_write(line: str) -> None:
        os.write(console_fd, f"{line}\n".encode("utf-8", errors="replace"))

    try:
        console_write(f"threads: {workers}   output: {output_dir}")
        console_write(progress_line(started, finished, total))

        log_path = output_dir / "logs" / "compare.log"
        with redirect_process_output(log_path, restore=False):
            for algo in ALGOS:
                try:
                    finished = run_algo(algo, output_dir, started, total, finished, console_write)
                except Exception as exc:
                    console_write(f"{algo} failed: {exc}")
                    console_write(f"details: {log_path}")
                    raise
    finally:
        os.close(console_fd)

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
