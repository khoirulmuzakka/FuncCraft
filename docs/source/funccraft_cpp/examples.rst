C++ examples
============

Create one function
-------------------

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
       sphere.coordinate_transform.assigned_xopt = x_star;
       sphere.value_transform.kind = ValueTransformKind::None;

       ComponentSpec rastrigin;
       rastrigin.base_function = BasicFunctionId::Rastrigin;
       rastrigin.coordinate_transform.kind = CoordinateTransformKind::Rotation;
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

       f.export_spec("function.yaml");
       BenchmarkFunction same = make_benchmark_function("function.yaml");
   }

Use a suite collection
----------------------

.. code-block:: cpp

   #include "funccraft.h"

   #include <vector>

   int main() {
       FuncCraft::SuiteCollection collection =
           FuncCraft::suite_collection(2026, 1);

       FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(10);
       std::vector<std::vector<double>> points = {
           std::vector<double>(10, 0.0),
           std::vector<double>(10, 1.0),
       };

       for (int i = 0; i < suite.size(); ++i) {
           const FuncCraft::BenchmarkFunction& f = suite.function(i);
           std::vector<double> values = f(points);
       }

       suite.export_manifest("suite_manifest.yaml");
   }

Minimize with Minion
--------------------

``examples/main_minimize.cpp`` contains the full example used by the project.
The key pattern is to wrap the batched FuncCraft objective in the callback
expected by Minion:

.. code-block:: cpp

   auto objective = [&f](const std::vector<std::vector<double>>& X, void*) {
       return f(X);
   };

Build examples with ``BUILD_EXAMPLES=ON``. This enables the Minion dependency.
