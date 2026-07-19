#include "suite.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

namespace py = pybind11;

PYBIND11_MODULE(_funccraft, m) {
    m.doc() = "FuncCraft Python bindings for benchmark suites";

    py::class_<FuncCraft::SuiteSpec>(m, "SuiteSpec")
        .def(py::init<>())
        .def_readwrite("supported_dimensions", &FuncCraft::SuiteSpec::supported_dimensions)
        .def_readwrite("base_functions", &FuncCraft::SuiteSpec::base_functions)
        .def_readwrite("base_function_coord_transforms", &FuncCraft::SuiteSpec::base_function_coord_transforms)
        .def_readwrite("coord_transforms", &FuncCraft::SuiteSpec::coord_transforms)
        .def_readwrite("value_transforms", &FuncCraft::SuiteSpec::value_transforms)
        .def_readwrite("composition_functions", &FuncCraft::SuiteSpec::composition_functions)
        .def_readwrite("base_functions_for_compositions", &FuncCraft::SuiteSpec::base_functions_for_compositions)
        .def_readwrite("requested_number_of_functions", &FuncCraft::SuiteSpec::requested_number_of_functions)
        .def_readwrite("max_number_of_functions", &FuncCraft::SuiteSpec::max_number_of_functions)
        .def_readwrite("max_dimension", &FuncCraft::SuiteSpec::max_dimension)
        .def_readwrite("master_seed", &FuncCraft::SuiteSpec::master_seed)
        .def_readwrite("lower_bound", &FuncCraft::SuiteSpec::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::SuiteSpec::upper_bound)
        .def_readwrite("f_opt", &FuncCraft::SuiteSpec::f_opt)
        .def_readwrite("suite_label", &FuncCraft::SuiteSpec::suite_label)
        .def("__repr__", [](const FuncCraft::SuiteSpec& self) {
            return "SuiteSpec(supported_dimensions='" + self.supported_dimensions
                + "', max_number_of_functions=" + std::to_string(self.max_number_of_functions)
                + ", max_dimension=" + std::to_string(self.max_dimension)
                + ", master_seed=" + std::to_string(self.master_seed)
                + ", lower_bound=" + std::to_string(self.lower_bound)
                + ", upper_bound=" + std::to_string(self.upper_bound)
                + ", f_opt=" + std::to_string(self.f_opt)
                + ", suite_label='" + self.suite_label + "')";
        });

    py::class_<FuncCraft::BenchmarkSuite>(m, "BenchmarkSuite")
        .def(py::init<FuncCraft::SuiteSpec, int>(), py::arg("spec"), py::arg("dimension"))
        .def_property_readonly("size", &FuncCraft::BenchmarkSuite::size)
        .def_property_readonly("dimension", &FuncCraft::BenchmarkSuite::dimension)
        .def("__len__", &FuncCraft::BenchmarkSuite::size)
        .def("function", &FuncCraft::BenchmarkSuite::function, py::arg("index"))
        .def("evaluate", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def("__call__", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def_property_readonly("spec", &FuncCraft::BenchmarkSuite::spec, py::return_value_policy::reference_internal)
        .def("__repr__", [](const FuncCraft::BenchmarkSuite& self) {
            return "BenchmarkSuite(size=" + std::to_string(self.size())
                + ", dimension=" + std::to_string(self.dimension()) + ")";
        });

    m.def("make_benchmark_suite", py::overload_cast<FuncCraft::SuiteSpec, int>(&FuncCraft::make_benchmark_suite),
        py::arg("spec"), py::arg("dimension"));
}
