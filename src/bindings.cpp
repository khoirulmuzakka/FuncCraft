#include "basicf.h"
#include "benchmark_function.h"
#include "suite.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

namespace py = pybind11;

PYBIND11_MODULE(_funccraft, m) {
    m.doc() = "FuncCraft Python bindings for benchmark suites";

    py::enum_<FuncCraft::BasicFunctionId>(m, "BasicFunctionId")
        .value("Sphere", FuncCraft::BasicFunctionId::Sphere)
        .value("Ellipsoidal", FuncCraft::BasicFunctionId::Ellipsoidal)
        .value("SumDifferentPowers", FuncCraft::BasicFunctionId::SumDifferentPowers)
        .value("BuecheRastrigin", FuncCraft::BasicFunctionId::BuecheRastrigin)
        .value("LinearSlope", FuncCraft::BasicFunctionId::LinearSlope)
        .value("AttractiveSector", FuncCraft::BasicFunctionId::AttractiveSector)
        .value("StepEllipsoidal", FuncCraft::BasicFunctionId::StepEllipsoidal)
        .value("StepRastrigin", FuncCraft::BasicFunctionId::StepRastrigin)
        .value("Rosenbrock", FuncCraft::BasicFunctionId::Rosenbrock)
        .value("Ackley", FuncCraft::BasicFunctionId::Ackley)
        .value("Rastrigin", FuncCraft::BasicFunctionId::Rastrigin)
        .value("Griewank", FuncCraft::BasicFunctionId::Griewank)
        .value("Schwefel", FuncCraft::BasicFunctionId::Schwefel)
        .value("SharpRidge", FuncCraft::BasicFunctionId::SharpRidge)
        .value("Weierstrass", FuncCraft::BasicFunctionId::Weierstrass)
        .value("SchafferF7", FuncCraft::BasicFunctionId::SchafferF7)
        .value("SchafferF7Cond1000", FuncCraft::BasicFunctionId::SchafferF7Cond1000)
        .value("GriewankRosenbrock", FuncCraft::BasicFunctionId::GriewankRosenbrock)
        .value("Gallagher21", FuncCraft::BasicFunctionId::Gallagher21)
        .value("Katsuura", FuncCraft::BasicFunctionId::Katsuura)
        .value("LunacekBiRastrigin", FuncCraft::BasicFunctionId::LunacekBiRastrigin)
        .value("Zakharov", FuncCraft::BasicFunctionId::Zakharov)
        .value("Levy", FuncCraft::BasicFunctionId::Levy)
        .value("Michalewicz", FuncCraft::BasicFunctionId::Michalewicz)
        .value("DixonPrice", FuncCraft::BasicFunctionId::DixonPrice)
        .value("BentCigar", FuncCraft::BasicFunctionId::BentCigar)
        .value("Discus", FuncCraft::BasicFunctionId::Discus)
        .value("HappyCat", FuncCraft::BasicFunctionId::HappyCat)
        .value("HGBat", FuncCraft::BasicFunctionId::HGBat)
        .value("HCF", FuncCraft::BasicFunctionId::HCF)
        .value("GrieRosen", FuncCraft::BasicFunctionId::GrieRosen)
        .value("SchafferF6", FuncCraft::BasicFunctionId::SchafferF6)
        .value("Step", FuncCraft::BasicFunctionId::Step)
        .value("Quartic", FuncCraft::BasicFunctionId::Quartic)
        .value("Exponential", FuncCraft::BasicFunctionId::Exponential)
        .value("StyblinskiTang", FuncCraft::BasicFunctionId::StyblinskiTang)
        .export_values();

    py::class_<FuncCraft::BasicF>(m, "BasicF")
        .def(py::init<FuncCraft::BasicFunctionId, int>(), py::arg("id"), py::arg("dimension"))
        .def("__call__", [](const FuncCraft::BasicF& self, const std::vector<std::vector<double>>& X) {
            return self(X);
        }, py::arg("points"))
        .def("evaluate", [](const FuncCraft::BasicF& self, const std::vector<double>& x) {
            return self.evaluate(x);
        }, py::arg("point"))
        .def_readonly("name", &FuncCraft::BasicF::name)
        .def_readonly("dimension", &FuncCraft::BasicF::dimension)
        .def_property_readonly("default_domain", &FuncCraft::BasicF::default_domain)
        .def_readonly("x_opt", &FuncCraft::BasicF::x_opt)
        .def_readonly("f_opt", &FuncCraft::BasicF::f_opt)
        .def_readonly("properties", &FuncCraft::BasicF::properties)
        .def("__repr__", [](const FuncCraft::BasicF& self) {
            return "BasicF(name='" + self.name
                + "', dimension=" + std::to_string(self.dimension) + ")";
        });

    py::class_<FuncCraft::Domain>(m, "Domain")
        .def(py::init<int, double, double>(), py::arg("dimension") = 0, py::arg("lower_bound") = -100.0, py::arg("upper_bound") = 100.0)
        .def_readwrite("lower", &FuncCraft::Domain::lower)
        .def_readwrite("upper", &FuncCraft::Domain::upper)
        .def_property_readonly("dimension", &FuncCraft::Domain::dimension)
        .def("__repr__", [](const FuncCraft::Domain& self) {
            return "Domain(dimension=" + std::to_string(self.dimension()) + ")";
        });

    py::class_<FuncCraft::ChoiceSpec>(m, "ChoiceSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::ChoiceSpec::kind)
        .def_readwrite("probability", &FuncCraft::ChoiceSpec::probability)
        .def_readwrite("parameters", &FuncCraft::ChoiceSpec::parameters)
        .def("__repr__", [](const FuncCraft::ChoiceSpec& self) {
            return "ChoiceSpec(kind='" + self.kind
                + "', probability=" + std::to_string(self.probability) + ")";
        });

    py::class_<FuncCraft::TransformSpec>(m, "TransformSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::TransformSpec::kind)
        .def_readwrite("dimension", &FuncCraft::TransformSpec::dimension)
        .def_readwrite("seed", &FuncCraft::TransformSpec::seed)
        .def_readwrite("selected_indices", &FuncCraft::TransformSpec::selected_indices)
        .def_readwrite("source_point", &FuncCraft::TransformSpec::source_point)
        .def_readwrite("target_point", &FuncCraft::TransformSpec::target_point)
        .def_readwrite("parameters", &FuncCraft::TransformSpec::parameters)
        .def_readwrite("matrix", &FuncCraft::TransformSpec::matrix);

    py::class_<FuncCraft::ValueTransformSpec>(m, "ValueTransformSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::ValueTransformSpec::kind)
        .def_readwrite("seed", &FuncCraft::ValueTransformSpec::seed)
        .def_readwrite("parameters", &FuncCraft::ValueTransformSpec::parameters);

    py::class_<FuncCraft::ComponentSpec>(m, "ComponentSpec")
        .def(py::init<>())
        .def_readwrite("base_function", &FuncCraft::ComponentSpec::base_function)
        .def_readwrite("component_dimension", &FuncCraft::ComponentSpec::component_dimension)
        .def_readwrite("coordinate_transform", &FuncCraft::ComponentSpec::coordinate_transform)
        .def_readwrite("value_transform", &FuncCraft::ComponentSpec::value_transform)
        .def_readwrite("seed", &FuncCraft::ComponentSpec::seed);

    py::class_<FuncCraft::CompositionSpec>(m, "CompositionSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::CompositionSpec::kind)
        .def_readwrite("seed", &FuncCraft::CompositionSpec::seed)
        .def_readwrite("parameters", &FuncCraft::CompositionSpec::parameters)
        .def_readwrite("centers", &FuncCraft::CompositionSpec::centers)
        .def_readwrite("offsets", &FuncCraft::CompositionSpec::offsets)
        .def_readwrite("weights", &FuncCraft::CompositionSpec::weights);

    py::class_<FuncCraft::FunctionSpec>(m, "FunctionSpec")
        .def(py::init<>())
        .def_readwrite("dimension", &FuncCraft::FunctionSpec::dimension)
        .def_readwrite("lower_bound", &FuncCraft::FunctionSpec::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::FunctionSpec::upper_bound)
        .def_readwrite("component_specs", &FuncCraft::FunctionSpec::component_specs)
        .def_readwrite("composition_spec", &FuncCraft::FunctionSpec::composition_spec)
        .def_readwrite("seed", &FuncCraft::FunctionSpec::seed)
        .def_readwrite("function_class_label", &FuncCraft::FunctionSpec::function_class_label)
        .def_readwrite("known_global_minimizer", &FuncCraft::FunctionSpec::known_global_minimizer)
        .def_readwrite("known_global_value", &FuncCraft::FunctionSpec::known_global_value)
        .def_readwrite("parameters", &FuncCraft::FunctionSpec::parameters)
        .def("__repr__", [](const FuncCraft::FunctionSpec& self) {
            return "FunctionSpec(dimension=" + std::to_string(self.dimension)
                + ", seed=" + std::to_string(self.seed)
                + ", label='" + self.function_class_label + "')";
        });

    py::class_<FuncCraft::SuiteSpec>(m, "SuiteSpec")
        .def(py::init<>())
        .def_readwrite("supported_dimensions", &FuncCraft::SuiteSpec::supported_dimensions)
        .def_readwrite("base_functions", &FuncCraft::SuiteSpec::base_functions)
        .def_readwrite("base_function_coord_transforms", &FuncCraft::SuiteSpec::base_function_coord_transforms)
        .def_readwrite("coord_transforms", &FuncCraft::SuiteSpec::coord_transforms)
        .def_readwrite("value_transforms", &FuncCraft::SuiteSpec::value_transforms)
        .def_readwrite("composition_functions", &FuncCraft::SuiteSpec::composition_functions)
        .def_readwrite("base_functions_for_compositions", &FuncCraft::SuiteSpec::base_functions_for_compositions)
        .def_readwrite("max_components", &FuncCraft::SuiteSpec::max_components)
        .def_readwrite("requested_number_of_functions", &FuncCraft::SuiteSpec::requested_number_of_functions)
        .def_readwrite("max_number_of_functions", &FuncCraft::SuiteSpec::max_number_of_functions)
        .def_readwrite("master_seed", &FuncCraft::SuiteSpec::master_seed)
        .def_readwrite("lower_bound", &FuncCraft::SuiteSpec::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::SuiteSpec::upper_bound)
        .def_readwrite("f_opt", &FuncCraft::SuiteSpec::f_opt)
        .def_readwrite("suite_label", &FuncCraft::SuiteSpec::suite_label)
        .def("__repr__", [](const FuncCraft::SuiteSpec& self) {
            return "SuiteSpec(supported_dimensions='" + self.supported_dimensions
                + "', max_components=" + std::to_string(self.max_components)
                + ", max_number_of_functions=" + std::to_string(self.max_number_of_functions)
                + ", requested_number_of_functions=" + std::to_string(self.requested_number_of_functions)
                + ", master_seed=" + std::to_string(self.master_seed)
                + ", lower_bound=" + std::to_string(self.lower_bound)
                + ", upper_bound=" + std::to_string(self.upper_bound)
                + ", f_opt=" + std::to_string(self.f_opt)
                + ", suite_label='" + self.suite_label + "')";
        });

    py::class_<FuncCraft::BenchmarkFunction>(m, "BenchmarkFunction")
        .def(py::init<FuncCraft::FunctionSpec>(), py::arg("spec"))
        .def("__call__", [](const FuncCraft::BenchmarkFunction& self, const std::vector<std::vector<double>>& X) {
            return self(X);
        }, py::arg("points"))
        .def("evaluate", [](const FuncCraft::BenchmarkFunction& self, const std::vector<std::vector<double>>& X) {
            return self(X);
        }, py::arg("points"))
        .def_property_readonly("domain", &FuncCraft::BenchmarkFunction::domain, py::return_value_policy::reference_internal)
        .def_property_readonly("dimension", &FuncCraft::BenchmarkFunction::dimension)
        .def_property_readonly("lambda", &FuncCraft::BenchmarkFunction::lambda)
        .def_property_readonly("scale", &FuncCraft::BenchmarkFunction::lambda)
        .def_property_readonly("bias", &FuncCraft::BenchmarkFunction::bias)
        .def_property_readonly("spec", &FuncCraft::BenchmarkFunction::spec, py::return_value_policy::reference_internal)
        .def("export_spec", [](const FuncCraft::BenchmarkFunction& self, const std::string& path) {
            self.export_spec(path);
        }, py::arg("path"))
        .def("__repr__", [](const FuncCraft::BenchmarkFunction& self) {
            return "BenchmarkFunction(dimension=" + std::to_string(self.dimension())
                + ", label='" + self.spec().function_class_label
                + "', lambda=" + std::to_string(self.lambda()) + ")";
        });

    py::class_<FuncCraft::BenchmarkSuite>(m, "BenchmarkSuite")
        .def(py::init<FuncCraft::SuiteSpec, int>(), py::arg("spec"), py::arg("dimension"))
        .def(py::init<const std::string&, int>(), py::arg("yaml_path"), py::arg("dimension"))
        .def_property_readonly("size", &FuncCraft::BenchmarkSuite::size)
        .def_property_readonly("max_number_of_functions", &FuncCraft::BenchmarkSuite::max_number_of_functions)
        .def_property_readonly("theoretical_max_number_of_functions", &FuncCraft::BenchmarkSuite::theoretical_max_number_of_functions)
        .def_property_readonly("dimension", &FuncCraft::BenchmarkSuite::dimension)
        .def("__len__", &FuncCraft::BenchmarkSuite::size)
        .def("function", &FuncCraft::BenchmarkSuite::function, py::arg("index"), py::return_value_policy::reference_internal)
        .def("evaluate", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def("__call__", [](const FuncCraft::BenchmarkSuite& self, int index, const std::vector<std::vector<double>>& X) {
            return self(index, X);
        }, py::arg("index"), py::arg("points"))
        .def_property_readonly("spec", &FuncCraft::BenchmarkSuite::spec, py::return_value_policy::reference_internal)
        .def("export_manifest", [](const FuncCraft::BenchmarkSuite& self, const std::string& path) {
            self.export_manifest(path);
        }, py::arg("path"))
        .def("export_spec", [](const FuncCraft::BenchmarkSuite& self, const std::string& path) {
            self.export_spec(path);
        }, py::arg("path"))
        .def("__repr__", [](const FuncCraft::BenchmarkSuite& self) {
            return "BenchmarkSuite(size=" + std::to_string(self.size())
                + ", dimension=" + std::to_string(self.dimension()) + ")";
        });

    m.def("make_benchmark_function", [](FuncCraft::FunctionSpec spec) {
        return FuncCraft::BenchmarkFunction(std::move(spec));
    }, py::arg("spec"));
    m.def("make_benchmark_suite", py::overload_cast<FuncCraft::SuiteSpec, int>(&FuncCraft::make_benchmark_suite),
        py::arg("spec"), py::arg("dimension"));
    m.def("load_suite_spec_yaml", &FuncCraft::load_suite_spec_yaml, py::arg("path"));
    m.def("load_function_spec_yaml", &FuncCraft::load_function_spec_yaml, py::arg("path"));
    m.def("make_benchmark_suite_from_yaml", &FuncCraft::make_benchmark_suite_from_yaml,
        py::arg("path"), py::arg("dimension"));
    m.def("make_benchmark_function_from_yaml", &FuncCraft::make_benchmark_function_from_yaml,
        py::arg("path"));
}
