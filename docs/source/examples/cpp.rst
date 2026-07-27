C++ examples
============

Use ``#include "funccraft.h"`` for the public C++ API. All evaluations are
batched: pass ``std::vector<std::vector<double>>`` and receive one value per
point.

Load a suite YAML
-----------------

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 1;

       FuncCraft::SuiteSpec spec =
           FuncCraft::load_suite_spec("my_suite.yaml");
       FuncCraft::BenchmarkSuite suite(spec, dimension);

       const FuncCraft::BenchmarkFunction& f =
           suite.function(function_index);
       std::vector<std::vector<double>> points = {
           std::vector<double>(dimension, 0.0),
           std::vector<double>(dimension, 1.0),
       };
       std::vector<double> values = f(points);

       std::cout << f.spec().label << '\n';
       std::cout << f.get_fopt() << '\n';
       std::cout << values.front() << '\n';
   }

Use a packaged suite collection
-------------------------------

.. code-block:: cpp

   #include "funccraft.h"

   int main() {
       const int dimension = 10;
       const int year = 2026;
       const int version = 1;
       FuncCraft::SuiteCollection collection =
           FuncCraft::suite_collection(year, version);
       FuncCraft::BenchmarkSuite suite =
           collection.benchmark_suite(dimension);
   }

Create one function in C++
--------------------------

For frequent editing, a YAML ``FunctionSpec`` is usually easier. Direct struct
construction is useful in C++ programs that generate specs:

.. code-block:: cpp

   #include "funccraft.h"

   #include <random>
   #include <vector>

   int main() {
       using namespace FuncCraft;

       std::mt19937_64 rng(1);
       std::uniform_real_distribution<double> uniform(-4.0, 4.0);
       std::vector<double> x_star = {uniform(rng), uniform(rng)};

       ComponentSpec sphere;
       sphere.base_function = BasicFunctionId::Sphere;
       sphere.coordinate_transform.kind = CoordinateTransformKind::None;
       sphere.coordinate_transform.input_dimension = 2;
       sphere.coordinate_transform.output_dimension = 2;
       sphere.coordinate_transform.assigned_xopt = x_star;
       sphere.value_transform.kind = ValueTransformKind::None;

       ComponentSpec rastrigin;
       rastrigin.base_function = BasicFunctionId::Rastrigin;
       rastrigin.coordinate_transform.kind = CoordinateTransformKind::Rotation;
       rastrigin.coordinate_transform.input_dimension = 2;
       rastrigin.coordinate_transform.output_dimension = 2;
       rastrigin.coordinate_transform.seed = 17;
       rastrigin.coordinate_transform.assigned_xopt = x_star;
       rastrigin.value_transform.kind = ValueTransformKind::Power;
       rastrigin.value_transform.parameters = {1.25, 1.0};

       FunctionSpec spec;
       spec.dimension = 2;
       spec.domain.dimension = 2;
       spec.domain.lower_bound = {-5.0, -5.0};
       spec.domain.upper_bound = {5.0, 5.0};
       spec.components = {sphere, rastrigin};
       spec.composition.kind = CompositionKind::CpmWeightedSum;
       spec.assigned_xopt = x_star;
       spec.assigned_fopt = 0.0;

       BenchmarkFunction f(spec);
       std::vector<double> values = f({x_star, {1.0, 1.0}});
   }

Load one function YAML
----------------------

.. code-block:: cpp

   #include "funccraft.h"

   int main() {
       FuncCraft::FunctionSpec spec =
           FuncCraft::load_function_spec("my_function.yaml");
       FuncCraft::BenchmarkFunction f(spec);
   }

Minimize with Minion
--------------------

Build examples with ``BUILD_EXAMPLES=ON`` to fetch and build the Minion
dependency. ``examples/main_minimize.cpp`` contains a fuller program.

.. code-block:: cpp

   #include "funccraft.h"
   #include <minion.h>

   #include <utility>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 1;
       const int year = 2026;
       const int version = 1;

       FuncCraft::BenchmarkSuite suite =
           FuncCraft::suite_collection(year, version).benchmark_suite(dimension);
       const FuncCraft::BenchmarkFunction& f =
           suite.function(function_index);
       const FuncCraft::Domain& domain = f.domain();

       std::vector<std::pair<double, double>> bounds;
       for (int i = 0; i < domain.dimension(); ++i) {
           bounds.emplace_back(domain.lower[i], domain.upper[i]);
       }

       std::vector<double> x0(dimension, 0.0);
       auto objective = [&f](
           const std::vector<std::vector<double>>& X,
           void*) {
           return f(X);
       };

       auto settings =
           minion::DefaultSettings().getDefaultSettings("ARRDE");
       settings["convergence_tol"] = 1e-8;

       minion::Minimizer optimizer(
           objective,
           bounds,
           x0,
           nullptr,
           nullptr,
           "ARRDE",
           10000,
           1,
           settings);

       minion::MinionResult result = optimizer.optimize();
   }

Export materialized YAML
------------------------

.. code-block:: cpp

   f.export_spec("function_materialized.yaml");
   suite.export_manifest("suite_manifest.yaml");
