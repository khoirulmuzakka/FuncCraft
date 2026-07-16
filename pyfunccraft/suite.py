"""Suite-focused Python wrapper for FuncCraft."""

from . import _funccraft


class BenchmarkSuite:
    """Pythonic wrapper around the compiled FuncCraft suite."""

    def __init__(self, options):
        self._suite = _funccraft.BenchmarkSuite(options)

    @staticmethod
    def mandatory_function_count():
        return _funccraft.BenchmarkSuite.mandatory_function_count()

    @property
    def size(self):
        return self._suite.size

    @property
    def options(self):
        return self._suite.options

    def evaluate(self, index, points):
        return self._suite.evaluate(index, points)

    def __call__(self, index, points):
        return self._suite(index, points)

    def __len__(self):
        return len(self._suite)

    def __repr__(self):
        return repr(self._suite)


BenchmarkSuiteOptions = _funccraft.BenchmarkSuiteOptions
make_benchmark_suite = _funccraft.make_benchmark_suite

__all__ = [
    "BenchmarkSuite",
    "BenchmarkSuiteOptions",
    "make_benchmark_suite",
]
