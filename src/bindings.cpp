#include "suite.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

namespace py = pybind11;

PYBIND11_MODULE(_funccraft, m) {
    m.doc() = "FuncCraft Python bindings for benchmark suites";

    py::class_<FuncCraft::BenchmarkSuiteOptions>(m, "BenchmarkSuiteOptions")
        .def(py::init<>())
        .def_readwrite("function_count", &FuncCraft::BenchmarkSuiteOptions::function_count)
        .def_readwrite("dimension", &FuncCraft::BenchmarkSuiteOptions::dimension)
        .def_readwrite("seed", &FuncCraft::BenchmarkSuiteOptions::seed)
        .def_readwrite("lower_bound", &FuncCraft::BenchmarkSuiteOptions::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::BenchmarkSuiteOptions::upper_bound)
        .def_readwrite("assigned_fmin", &FuncCraft::BenchmarkSuiteOptions::assigned_fmin)
        .def("__repr__", [](const FuncCraft::BenchmarkSuiteOptions& self) {
            return "BenchmarkSuiteOptions(function_count=" + std::to_string(self.function_count)
                + ", dimension=" + std::to_string(self.dimension)
                + ", seed=" + std::to_string(self.seed)
                + ", lower_bound=" + std::to_string(self.lower_bound)
                + ", upper_bound=" + std::to_string(self.upper_bound)
                + ", assigned_fmin=" + std::to_string(self.assigned_fmin) + ")";
        });

    py::class_<FuncCraft::BenchmarkSuite>(m, "BenchmarkSuite")
        .def(py::init<FuncCraft::BenchmarkSuiteOptions>(), py::arg("options"))
        .def_static("mandatory_function_count", &FuncCraft::BenchmarkSuite::mandatory_function_count)
        .def_property_readonly("size", &FuncCraft::BenchmarkSuite::size)
        .def("__len__", &FuncCraft::BenchmarkSuite::size)
        .def("evaluate", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def("__call__", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def_property_readonly("options", &FuncCraft::BenchmarkSuite::options, py::return_value_policy::reference_internal)
        .def("__repr__", [](const FuncCraft::BenchmarkSuite& self) {
            return "BenchmarkSuite(size=" + std::to_string(self.size()) + ")";
        });

    m.def("make_benchmark_suite", &FuncCraft::make_benchmark_suite, py::arg("function_count"), py::arg("dimension"), py::arg("seed") = 1);
}
