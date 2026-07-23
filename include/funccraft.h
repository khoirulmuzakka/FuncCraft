#ifndef FUNCCRAFT_H
#define FUNCCRAFT_H

/**
 * @file funccraft.h
 * @brief Public umbrella header for the high-level FuncCraft C++ API.
 *
 * Include this header when you want to construct benchmark functions, generate
 * benchmark suites, load specs from files, and export reproducibility specs.
 * Low-level runtime builders and transform implementations remain available
 * through their own headers, but are intentionally not included here.
 *
 * Public API map:
 *
 * Primitive functions:
 * - `FuncCraft::BasicFunctionId`
 * - `FuncCraft::BasicF`
 * - `FuncCraft::list_basic_functions()`
 * - `FuncCraft::to_string(BasicFunctionId)`
 *
 * Function specs:
 * - `FuncCraft::DomainSpec`
 * - `FuncCraft::CoordinateTransformSpec`
 * - `FuncCraft::ValueTransformSpec`
 * - `FuncCraft::ComponentSpec`
 * - `FuncCraft::CompositionSpec`
 * - `FuncCraft::FunctionSpec`
 *
 * Suite specs:
 * - `FuncCraft::CoordinateTransformChoice`
 * - `FuncCraft::ValueTransformChoice`
 * - `FuncCraft::CompositionChoice`
 * - `FuncCraft::SuiteSpec`
 * - `FuncCraft::make_choice(...)`
 * - `FuncCraft::all_suite_base_functions()`
 * - `FuncCraft::all_coordinate_transform_choices()`
 * - `FuncCraft::all_value_transform_choices()`
 * - `FuncCraft::all_composition_choices()`
 *
 * Runtime objects:
 * - `FuncCraft::BenchmarkFunction`
 * - `FuncCraft::BenchmarkSuite`
 * - `FuncCraft::SuiteCollection`
 *
 * Loading and construction:
 * - `FuncCraft::load_function_spec(path)`
 * - `FuncCraft::make_benchmark_function(path)`
 * - `FuncCraft::load_suite_spec(path)`
 * - `FuncCraft::make_benchmark_suite(spec, dimension)`
 * - `FuncCraft::make_benchmark_suite(path, dimension)`
 * - `FuncCraft::list_suite_collections()`
 * - `FuncCraft::suite_collection(year, version)`
 * - `FuncCraft::suite_collection_spec(year, version)`
 * - `FuncCraft::suite_collection_number_of_functions(year, version)`
 *
 * Export:
 * - `BenchmarkFunction::export_spec(path)`
 * - `BenchmarkSuite::export_manifest(path)`
 * - `BenchmarkSuite::export_spec(path)`
 *
 * Evaluation is batched:
 * - `BenchmarkFunction::operator()(std::vector<std::vector<double>>)`
 * - `BenchmarkSuite::operator()(index, std::vector<std::vector<double>>)`
 *
 * Minimal example:
 *
 * @code{.cpp}
 * #include "funccraft.h"
 *
 * int main() {
 *     FuncCraft::SuiteCollection collection =
 *         FuncCraft::suite_collection(2026, 1);
 *
 *     FuncCraft::BenchmarkSuite suite =
 *         collection.benchmark_suite(10);
 *     const FuncCraft::BenchmarkFunction& f = suite.function(0);
 *     std::vector<double> values = f({std::vector<double>(10, 0.0)});
 *
 *     f.export_spec("function.yaml");
 *     suite.export_manifest("suite_manifest.yaml");
 * }
 * @endcode
 */

#include "basicf.h"
#include "benchmark_function.h"
#include "function_spec.h"
#include "suite.h"
#include "suite_collection.h"
#include "suite_spec.h"

#endif // FUNCCRAFT_H
