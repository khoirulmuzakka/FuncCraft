"""Suite-focused Python wrapper for FuncCraft."""

from . import _funccraft


class BenchmarkSuite:
    """Pythonic wrapper around the compiled FuncCraft suite."""

    def __init__(self, spec):
        self._suite = _funccraft.BenchmarkSuite(spec)

    @property
    def size(self):
        return self._suite.size

    @property
    def spec(self):
        return self._suite.spec

    def evaluate(self, index, dimension, points):
        return self._suite.evaluate(index, dimension, points)

    def __call__(self, index, dimension, points):
        return self._suite(index, dimension, points)

    def __len__(self):
        return len(self._suite)

    def __repr__(self):
        return repr(self._suite)


SuiteSpec = _funccraft.SuiteSpec
make_benchmark_suite = _funccraft.make_benchmark_suite

__all__ = [
    "BenchmarkSuite",
    "SuiteSpec",
    "make_benchmark_suite",
]
