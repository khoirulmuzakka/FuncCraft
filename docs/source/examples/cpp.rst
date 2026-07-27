C++ examples
============

Use ``#include "funccraft.h"`` for the public C++ API. Evaluations are
batched: pass ``std::vector<std::vector<double>>`` and receive one value per
point.

Construction framework
----------------------

FuncCraft uses the same runtime model in C++ and Python:

``BenchmarkFunction``
   One concrete callable benchmark function with fixed dimension, domain,
   assigned optimum, components, transforms, composition rule, and scale
   factor.

``BenchmarkSuite``
   A materialized collection of benchmark functions for one chosen dimension.
   Use ``suite.function(index)`` to retrieve one ``BenchmarkFunction``.
   Function indices are one-based.

``FunctionSpec`` and ``SuiteSpec``
   Plain C++ structs corresponding to the YAML spec structure. In C++ you
   usually fill these structs directly or load them from YAML.

BenchmarkFunction
-----------------

This example creates one composed ``BenchmarkFunction`` from explicit C++
``FunctionSpec`` structs. One outer component is itself a nested composed
function.

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <memory>
   #include <vector>

   int main() {
       using namespace FuncCraft;

       const int dimension = 2;
       const std::vector<double> assigned_xopt = {0.1, 3.6};
       const std::vector<double> nested_xopt = {1.0, -1.0};

       ComponentSpec nested_rosenbrock;
       nested_rosenbrock.base_function = BasicFunctionId::Rosenbrock;
       nested_rosenbrock.coordinate_transform.kind = CoordinateTransformKind::None;
       nested_rosenbrock.coordinate_transform.input_dimension = dimension;
       nested_rosenbrock.coordinate_transform.output_dimension = dimension;
       nested_rosenbrock.coordinate_transform.assigned_xopt = nested_xopt;
       nested_rosenbrock.value_transform.kind = ValueTransformKind::None;

       ComponentSpec nested_schwefel;
       nested_schwefel.base_function = BasicFunctionId::Schwefel;
       nested_schwefel.coordinate_transform.kind = CoordinateTransformKind::Rotation;
       nested_schwefel.coordinate_transform.input_dimension = dimension;
       nested_schwefel.coordinate_transform.output_dimension = dimension;
       nested_schwefel.coordinate_transform.assigned_xopt = nested_xopt;
       nested_schwefel.coordinate_transform.seed = 17;
       nested_schwefel.value_transform.kind = ValueTransformKind::Power;
       nested_schwefel.value_transform.parameters = {1.1, 1.0};

       FunctionSpec nested_spec;
       nested_spec.dimension = dimension;
       nested_spec.domain.dimension = dimension;
       nested_spec.domain.lower_bound = {-5.0, -5.0};
       nested_spec.domain.upper_bound = {5.0, 5.0};
       nested_spec.components = {nested_rosenbrock, nested_schwefel};
       nested_spec.composition.kind = CompositionKind::CpmWeightedSum;
       nested_spec.assigned_xopt = nested_xopt;
       nested_spec.assigned_fopt = 0.0;
       nested_spec.seed = 11;
       nested_spec.label = "nested-rosenbrock-schwefel";

       ComponentSpec outer_griewank;
       outer_griewank.base_function = BasicFunctionId::Griewank;
       outer_griewank.coordinate_transform.kind = CoordinateTransformKind::None;
       outer_griewank.coordinate_transform.input_dimension = dimension;
       outer_griewank.coordinate_transform.output_dimension = dimension;
       outer_griewank.coordinate_transform.assigned_xopt = assigned_xopt;
       outer_griewank.value_transform.kind = ValueTransformKind::None;

       ComponentSpec outer_nested;
       outer_nested.composed_function = std::make_shared<FunctionSpec>(nested_spec);
       outer_nested.coordinate_transform.kind = CoordinateTransformKind::Rotation;
       outer_nested.coordinate_transform.input_dimension = dimension;
       outer_nested.coordinate_transform.output_dimension = dimension;
       outer_nested.coordinate_transform.assigned_xopt = assigned_xopt;
       outer_nested.coordinate_transform.seed = 31;
       outer_nested.value_transform.kind = ValueTransformKind::Power;
       outer_nested.value_transform.parameters = {1.25, 1.0};

       FunctionSpec spec;
       spec.dimension = dimension;
       spec.domain.dimension = dimension;
       spec.domain.lower_bound = {-10.0, -10.0};
       spec.domain.upper_bound = {10.0, 10.0};
       spec.components = {outer_griewank, outer_nested};
       spec.composition.kind = CompositionKind::CpmWeightedSum;
       spec.assigned_xopt = assigned_xopt;
       spec.assigned_fopt = 0.0;
       spec.seed = 1;
       spec.label = "cpp-nested-composed-function";

       BenchmarkFunction f(spec);
       std::vector<std::vector<double>> points = {
           std::vector<double>(dimension, 0.0),
           assigned_xopt,
           std::vector<double>(dimension, 1.0),
       };
       std::vector<double> values = f(points);

       std::cout << "dimension: " << f.dimension() << '\n';
       std::cout << "label: " << f.spec().label << '\n';
       std::cout << "component_types: " << f.component_types() << '\n';
       std::cout << "fopt: " << f.get_fopt() << '\n';
       std::cout << "first value: " << values.front() << '\n';
   }

Export and import one function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Exported function YAML contains generated matrices, resolved scale factor, and
other materialized values needed to reproduce the exact function.

.. code-block:: cpp

   f.export_spec("manual_function.yaml");

   FuncCraft::BenchmarkFunction reloaded =
       FuncCraft::make_benchmark_function("manual_function.yaml");

   std::vector<double> original_values = f(points);
   std::vector<double> reloaded_values = reloaded(points);

BenchmarkSuite
--------------

This example creates a custom ``BenchmarkSuite`` from an explicit ``SuiteSpec``.
It lists every current mechanism family. Set a choice probability to ``0.0``
to keep an option visible but disabled.

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       using namespace FuncCraft;

       SuiteSpec spec;
       spec.supported_dimensions = "2";
       spec.base_functions = all_suite_base_functions();
       spec.composition_base_functions = all_suite_base_functions();

       spec.coordinate_transforms = {
           make_choice(CoordinateTransformKind::None, 0.0),
           make_choice(CoordinateTransformKind::Rotation, 0.5),
           make_choice(CoordinateTransformKind::Affine, 0.0),
           make_choice(CoordinateTransformKind::BlockRotation, 0.5),
       };
       spec.value_transforms = {
           make_choice(ValueTransformKind::None, 0.5),
           make_choice(ValueTransformKind::Power, 0.25, 1.0, 1.0),
           make_choice(ValueTransformKind::Oscillatory, 0.25, 0.1, 1.0),
           make_choice(ValueTransformKind::CosineZero, 0.0, 1.0),
       };
       spec.compositions = {
           make_choice(CompositionKind::CpmWeightedSum, 0.1),
           make_choice(CompositionKind::CpmPowerMean, 0.1, 3.0),
           make_choice(CompositionKind::CpmPowerMean, 0.1, 0.1),
           make_choice(CompositionKind::CpmLevelWell, 0.2, 0.1, 1.0),
           make_choice(CompositionKind::DpmSoftmax, 0.25, 0.01),
           make_choice(CompositionKind::DpmBgSoftmax, 0.25, 0.01, 1.0, 0.01),
       };
       spec.min_components = 2;
       spec.max_components = 4;
       spec.max_nested_composition_depth = 1;
       spec.nested_probability = 0.1;
       spec.requested_number_of_functions = 200;
       spec.master_seed = 1;
       spec.lower_bound = -100.0;
       spec.upper_bound = 100.0;
       spec.assigned_fopt = 100.0;
       spec.xopt_domain_shrink_factor = 0.8;
       spec.suite_label = "cpp-suite";

       BenchmarkSuite suite(spec, 2);
       std::cout << "size: " << suite.size() << '\n';
       std::cout << "dimension: " << suite.dimension() << '\n';
       std::cout << "capacity: "
                 << suite.theoretical_max_number_of_functions() << '\n';

       for (int index = 1; index <= 5; ++index) {
           const BenchmarkFunction& f = suite.function(index);
           std::cout << "F" << index << ": "
                     << f.component_types() << " | "
                     << f.spec().label << '\n';
       }
   }

Export and import one suite
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The suite manifest contains the normalized suite spec and every generated
function spec, so the exact benchmark set can be reused later.

.. code-block:: cpp

   suite.export_manifest("generated_suite_manifest.yaml");

   FuncCraft::BenchmarkSuite reloaded_suite(
       "generated_suite_manifest.yaml",
       suite.dimension());

   const FuncCraft::BenchmarkFunction& f1 = reloaded_suite.function(1);

Shipped suite: FuncCraft Benchmark Suite 2026 v1
------------------------------------------------

FuncCraft ships the versioned ``2026_v1`` benchmark suite. Load it through
``suite_collection(2026, 1)``, then materialize a ``BenchmarkSuite`` at the
dimension you want.

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>

   int main() {
       FuncCraft::SuiteCollection collection =
           FuncCraft::suite_collection(2026, 1);
       FuncCraft::BenchmarkSuite suite =
           collection.benchmark_suite(2);

       std::cout << "collection name: " << collection.name() << '\n';
       std::cout << "default number of functions: "
                 << collection.number_of_functions() << '\n';
       std::cout << "suite size: " << suite.size() << '\n';
       std::cout << "suite dimension: " << suite.dimension() << '\n';

       const FuncCraft::BenchmarkFunction& f1 = suite.function(1);
       std::cout << "F1 label: " << f1.spec().label << '\n';
       std::cout << "F1 component_types: "
                 << f1.component_types() << '\n';
   }

Plot functions 1 through 500
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

C++ does not include a plotting backend. A common workflow is to export a
value table or manifest from C++, then plot it with Python. For plotting inside
FuncCraft documentation, see the Python example page.

.. code-block:: cpp

   #include <string>

   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(2);

   for (int index = 1; index <= 500; ++index) {
       const FuncCraft::BenchmarkFunction& f = suite.function(index);
       f.export_spec("function_" + std::to_string(index) + ".yaml");
   }

Minimize F45 at 10D with Minion
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Build examples with ``BUILD_EXAMPLES=ON`` to fetch and build the Minion
dependency. Function indices are one-based, so ``F45`` is loaded with
``suite.function(45)``.

.. code-block:: cpp

   #include "funccraft.h"
   #include <minion.h>

   #include <cmath>
   #include <cstddef>
   #include <iostream>
   #include <utility>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 45;

       FuncCraft::BenchmarkSuite suite =
           FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension);
       const FuncCraft::BenchmarkFunction& f45 =
           suite.function(function_index);
       const FuncCraft::Domain& domain = f45.domain();

       std::vector<std::pair<double, double>> bounds;
       bounds.reserve(static_cast<std::size_t>(domain.dimension()));
       for (int i = 0; i < domain.dimension(); ++i) {
           bounds.emplace_back(domain.lower[i], domain.upper[i]);
       }

       std::vector<double> x0(dimension, 1.0);
       auto objective = [&f45](
           const std::vector<std::vector<double>>& X,
           void*) {
           return f45(X);
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
           50000,
           1,
           settings);

       minion::MinionResult result = optimizer.optimize();
       const double error = std::abs(result.fun - f45.get_fopt());

       std::cout << "function: F" << function_index << '\n';
       std::cout << "label: " << f45.spec().label << '\n';
       std::cout << "dimension: " << f45.dimension() << '\n';
       std::cout << "assigned fopt: " << f45.get_fopt() << '\n';
       std::cout << "best value: " << result.fun << '\n';
       std::cout << "|best - fopt|: " << error << '\n';
   }

Primitive base functions
------------------------

Primitive functions can be evaluated directly with ``BasicF``. Use
``BasicFunctionId`` values or names from the primitive function ID table.

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       for (FuncCraft::BasicFunctionId id :
            FuncCraft::list_basic_functions()) {
           FuncCraft::BasicF f(id, 2);
           FuncCraft::Domain domain = f.default_domain();
           std::vector<double> x = f.x_opt;
           double value = f.evaluate(x);

           std::cout << static_cast<int>(id) << " "
                     << FuncCraft::to_string(id) << " "
                     << "domain=[" << domain.lower[0] << ", "
                     << domain.upper[0] << "] "
                     << "f(x_opt)=" << value << '\n';
       }
   }

See :doc:`../user_guide/primitive_base_functions` for the full primitive
function ID table.
