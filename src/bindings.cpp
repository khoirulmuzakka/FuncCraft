#include "basicf.h"
#include "benchmark_function.h"
#include "suite.h"
#include "suite_collection.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <utility>

namespace py = pybind11;

namespace {

template <typename Choice>
void bind_choice(py::module_& m, const char* name) {
    py::class_<Choice>(m, name)
        .def(py::init<>())
        .def_readwrite("kind", &Choice::kind)
        .def_readwrite("probability", &Choice::probability)
        .def_readwrite("parameters", &Choice::parameters);
}

void set_environment_variable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 0);
#endif
}

void configure_packaged_suite_dir(py::module_& m) {
    if (std::getenv("FUNCCRAFT_SUITE_DIR") != nullptr) {
        return;
    }
    try {
        const std::filesystem::path module_path = m.attr("__file__").cast<std::string>();
        const std::filesystem::path suite_dir = module_path.parent_path() / "suites";
        if (std::filesystem::exists(suite_dir) && std::filesystem::is_directory(suite_dir)) {
            set_environment_variable("FUNCCRAFT_SUITE_DIR", suite_dir.string());
        }
    } catch (const std::exception&) {
    }
}

void configure_packaged_suite_dir() {
    if (std::getenv("FUNCCRAFT_SUITE_DIR") != nullptr) {
        return;
    }
    try {
        py::dict modules = py::module_::import("sys").attr("modules");
        py::object module = modules.contains("funccraft._funccraft")
            ? modules["funccraft._funccraft"]
            : modules["_funccraft"];
        const std::filesystem::path module_path = module.attr("__file__").cast<std::string>();
        const std::filesystem::path suite_dir = module_path.parent_path() / "suites";
        if (std::filesystem::exists(suite_dir) && std::filesystem::is_directory(suite_dir)) {
            set_environment_variable("FUNCCRAFT_SUITE_DIR", suite_dir.string());
        }
    } catch (const std::exception&) {
    }
}

} // namespace

PYBIND11_MODULE(_funccraft, m) {
    m.doc() = "FuncCraft Python bindings for benchmark suites";
    configure_packaged_suite_dir(m);

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

    py::enum_<FuncCraft::CoordinateTransformKind>(m, "CoordinateTransformKind")
        .value("None", FuncCraft::CoordinateTransformKind::None)
        .value("Rotation", FuncCraft::CoordinateTransformKind::Rotation)
        .value("Affine", FuncCraft::CoordinateTransformKind::Affine)
        .value("BlockRotation", FuncCraft::CoordinateTransformKind::BlockRotation)
        .export_values();

    py::enum_<FuncCraft::ValueTransformKind>(m, "ValueTransformKind")
        .value("None", FuncCraft::ValueTransformKind::None)
        .value("Power", FuncCraft::ValueTransformKind::Power)
        .value("Oscillatory", FuncCraft::ValueTransformKind::Oscillatory)
        .value("CosineZero", FuncCraft::ValueTransformKind::CosineZero)
        .export_values();

    py::enum_<FuncCraft::CompositionKind>(m, "CompositionKind")
        .value("None", FuncCraft::CompositionKind::None)
        .value("CpmWeightedSum", FuncCraft::CompositionKind::CpmWeightedSum)
        .value("CpmPowerMean", FuncCraft::CompositionKind::CpmPowerMean)
        .value("CpmLevelWell", FuncCraft::CompositionKind::CpmLevelWell)
        .value("DpmSoftmax", FuncCraft::CompositionKind::DpmSoftmax)
        .value("DpmBgSoftmax", FuncCraft::CompositionKind::DpmBgSoftmax)
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
        .def_readonly("properties", &FuncCraft::BasicF::properties);

    py::class_<FuncCraft::Domain>(m, "Domain")
        .def(py::init<int, double, double>(), py::arg("dimension") = 0, py::arg("lower_bound") = -100.0, py::arg("upper_bound") = 100.0)
        .def_readwrite("lower", &FuncCraft::Domain::lower)
        .def_readwrite("upper", &FuncCraft::Domain::upper)
        .def_property_readonly("dimension", &FuncCraft::Domain::dimension);

    py::class_<FuncCraft::DomainSpec>(m, "DomainSpec")
        .def(py::init<>())
        .def_readwrite("dimension", &FuncCraft::DomainSpec::dimension)
        .def_readwrite("lower_bound", &FuncCraft::DomainSpec::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::DomainSpec::upper_bound);

    py::class_<FuncCraft::CoordinateTransformSpec>(m, "CoordinateTransformSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::CoordinateTransformSpec::kind)
        .def_readwrite("dimension", &FuncCraft::CoordinateTransformSpec::dimension)
        .def_readwrite("assigned_xopt", &FuncCraft::CoordinateTransformSpec::assigned_xopt)
        .def_readwrite("selected_indices", &FuncCraft::CoordinateTransformSpec::selected_indices)
        .def_readwrite("parameters", &FuncCraft::CoordinateTransformSpec::parameters)
        .def_readwrite("matrix", &FuncCraft::CoordinateTransformSpec::matrix)
        .def_readwrite("seed", &FuncCraft::CoordinateTransformSpec::seed);

    py::class_<FuncCraft::ValueTransformSpec>(m, "ValueTransformSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::ValueTransformSpec::kind)
        .def_readwrite("parameters", &FuncCraft::ValueTransformSpec::parameters);

    auto function_spec_binding = py::class_<FuncCraft::FunctionSpec, std::shared_ptr<FuncCraft::FunctionSpec>>(m, "FunctionSpec");

    py::class_<FuncCraft::ComponentSpec>(m, "ComponentSpec")
        .def(py::init<>())
        .def_property(
            "base_function",
            [](const FuncCraft::ComponentSpec& self) -> std::optional<FuncCraft::BasicFunctionId> {
                return self.base_function;
            },
            [](FuncCraft::ComponentSpec& self, std::optional<FuncCraft::BasicFunctionId> value) {
                self.base_function = value;
            })
        .def_readwrite("composed_function", &FuncCraft::ComponentSpec::composed_function)
        .def_readwrite("coordinate_transform", &FuncCraft::ComponentSpec::coordinate_transform)
        .def_readwrite("value_transform", &FuncCraft::ComponentSpec::value_transform)
        .def_readwrite("seed", &FuncCraft::ComponentSpec::seed);

    py::class_<FuncCraft::CompositionSpec>(m, "CompositionSpec")
        .def(py::init<>())
        .def_readwrite("kind", &FuncCraft::CompositionSpec::kind)
        .def_readwrite("parameters", &FuncCraft::CompositionSpec::parameters)
        .def_readwrite("biases", &FuncCraft::CompositionSpec::biases);

    function_spec_binding
        .def(py::init<>())
        .def_readwrite("dimension", &FuncCraft::FunctionSpec::dimension)
        .def_readwrite("domain", &FuncCraft::FunctionSpec::domain)
        .def_readwrite("components", &FuncCraft::FunctionSpec::components)
        .def_readwrite("composition", &FuncCraft::FunctionSpec::composition)
        .def_readwrite("assigned_xopt", &FuncCraft::FunctionSpec::assigned_xopt)
        .def_readwrite("assigned_fopt", &FuncCraft::FunctionSpec::assigned_fopt)
        .def_readwrite("scale_factor", &FuncCraft::FunctionSpec::scale_factor)
        .def_readwrite("seed", &FuncCraft::FunctionSpec::seed)
        .def_readwrite("label", &FuncCraft::FunctionSpec::label)
        .def_readwrite("metadata", &FuncCraft::FunctionSpec::metadata);

    bind_choice<FuncCraft::CoordinateTransformChoice>(m, "CoordinateTransformChoice");
    bind_choice<FuncCraft::ValueTransformChoice>(m, "ValueTransformChoice");
    bind_choice<FuncCraft::CompositionChoice>(m, "CompositionChoice");

    py::class_<FuncCraft::SuiteSpec>(m, "SuiteSpec")
        .def(py::init<>())
        .def_readwrite("supported_dimensions", &FuncCraft::SuiteSpec::supported_dimensions)
        .def_readwrite("base_functions", &FuncCraft::SuiteSpec::base_functions)
        .def_readwrite("composition_base_functions", &FuncCraft::SuiteSpec::composition_base_functions)
        .def_readwrite("coordinate_transforms", &FuncCraft::SuiteSpec::coordinate_transforms)
        .def_readwrite("value_transforms", &FuncCraft::SuiteSpec::value_transforms)
        .def_readwrite("compositions", &FuncCraft::SuiteSpec::compositions)
        .def_readwrite("min_components", &FuncCraft::SuiteSpec::min_components)
        .def_readwrite("max_components", &FuncCraft::SuiteSpec::max_components)
        .def_readwrite("max_nested_composition_depth", &FuncCraft::SuiteSpec::max_nested_composition_depth)
        .def_readwrite("nested_probability", &FuncCraft::SuiteSpec::nested_probability)
        .def_readwrite("requested_number_of_functions", &FuncCraft::SuiteSpec::requested_number_of_functions)
        .def_readwrite("max_number_of_functions", &FuncCraft::SuiteSpec::max_number_of_functions)
        .def_readwrite("master_seed", &FuncCraft::SuiteSpec::master_seed)
        .def_readwrite("lower_bound", &FuncCraft::SuiteSpec::lower_bound)
        .def_readwrite("upper_bound", &FuncCraft::SuiteSpec::upper_bound)
        .def_readwrite("assigned_fopt", &FuncCraft::SuiteSpec::assigned_fopt)
        .def_readwrite("xopt_domain_shrink_factor", &FuncCraft::SuiteSpec::xopt_domain_shrink_factor)
        .def_readwrite("suite_label", &FuncCraft::SuiteSpec::suite_label);

    py::class_<FuncCraft::SuiteCollectionId>(m, "SuiteCollectionId")
        .def(py::init<>())
        .def_readwrite("year", &FuncCraft::SuiteCollectionId::year)
        .def_readwrite("version", &FuncCraft::SuiteCollectionId::version)
        .def_readwrite("name", &FuncCraft::SuiteCollectionId::name);

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
        .def_property_readonly("scale_factor", &FuncCraft::BenchmarkFunction::scale_factor)
        .def_property_readonly("assigned_fopt", &FuncCraft::BenchmarkFunction::assigned_fopt)
        .def_property_readonly("spec", &FuncCraft::BenchmarkFunction::spec, py::return_value_policy::reference_internal)
        .def("export_spec", [](const FuncCraft::BenchmarkFunction& self, const std::string& path) {
            self.export_spec(path);
        }, py::arg("path"));

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
        }, py::arg("path"));

    py::class_<FuncCraft::SuiteCollection>(m, "SuiteCollection")
        .def(py::init([](int year, int version) {
            configure_packaged_suite_dir();
            return FuncCraft::SuiteCollection(year, version);
        }), py::arg("year"), py::arg("version"))
        .def_property_readonly("year", &FuncCraft::SuiteCollection::year)
        .def_property_readonly("version", &FuncCraft::SuiteCollection::version)
        .def_property_readonly("name", &FuncCraft::SuiteCollection::name)
        .def_property_readonly("number_of_functions", &FuncCraft::SuiteCollection::number_of_functions)
        .def("spec", &FuncCraft::SuiteCollection::spec)
        .def("benchmark_suite", &FuncCraft::SuiteCollection::benchmark_suite,
            py::arg("dimension"));

    m.def("make_benchmark_function", [](FuncCraft::FunctionSpec spec) {
        return FuncCraft::BenchmarkFunction(std::move(spec));
    }, py::arg("spec"));
    m.def("make_benchmark_function", [](const std::string& path) {
        return FuncCraft::make_benchmark_function(path);
    }, py::arg("path"));
    m.def("make_benchmark_suite", py::overload_cast<FuncCraft::SuiteSpec, int>(&FuncCraft::make_benchmark_suite),
        py::arg("spec"), py::arg("dimension"));
    m.def("make_benchmark_suite", py::overload_cast<const std::string&, int>(&FuncCraft::make_benchmark_suite),
        py::arg("path"), py::arg("dimension"));
    m.def("load_suite_spec", &FuncCraft::load_suite_spec, py::arg("path"));
    m.def("load_function_spec", &FuncCraft::load_function_spec, py::arg("path"));
    m.def("list_suite_collections", []() {
        configure_packaged_suite_dir();
        return FuncCraft::list_suite_collections();
    });
    m.def("suite_collection", [](int year, int version) {
        configure_packaged_suite_dir();
        return FuncCraft::suite_collection(year, version);
    }, py::arg("year"), py::arg("version"));
    m.def("suite_collection_spec", [](int year, int version) {
        configure_packaged_suite_dir();
        return FuncCraft::suite_collection_spec(year, version);
    }, py::arg("year"), py::arg("version"));
    m.def("suite_collection_number_of_functions", [](int year, int version) {
        configure_packaged_suite_dir();
        return FuncCraft::suite_collection_number_of_functions(year, version);
    },
        py::arg("year"), py::arg("version"));
    m.def("suite_collection_benchmark_suite", [](int year, int version, int dimension) {
        configure_packaged_suite_dir();
        return FuncCraft::suite_collection_benchmark_suite(year, version, dimension);
    },
        py::arg("year"),
        py::arg("version"),
        py::arg("dimension"));
}
