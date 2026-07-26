Getting started
===============

FuncCraft evaluates benchmark functions in batches. A point is a vector of
numbers, and an evaluation input is a collection of points.

Install the package:

.. code-block:: shell

   python -m pip install --upgrade funccraft

Python quick start
------------------

Load a suite YAML
~~~~~~~~~~~~~~~~~

For an editable suite, write or copy a YAML file and load it:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   spec = fc.load_suite_spec("my_suite.yaml")
   suite = fc.BenchmarkSuite(spec, dimension)

   function_index = 0
   points = [[0.0] * dimension, [1.0] * dimension]
   values = suite.evaluate(function_index, points)

   print(values)

Use the packaged suite
~~~~~~~~~~~~~~~~~~~~~~

The packaged 2026 suite is also defined by YAML, but it is exposed through a
short collection API:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   year = 2026
   version = 1
   collection = fc.suite_collection(year, version)
   suite = collection.benchmark_suite(dimension)

   function_index = 0
   f = suite.function(function_index)
   values = f.evaluate([[0.0] * dimension])

Inspect a function
~~~~~~~~~~~~~~~~~~

.. code-block:: python

   print(f.spec.label)
   print(f.get_xopt())
   print(f.get_fopt())
   print(f.component_types)

Export a materialized spec
~~~~~~~~~~~~~~~~~~~~~~~~~~

Exported YAML is a materialized record of a function or suite. It is useful
for archiving the exact generated table used by an experiment.

.. code-block:: python

   f.export_spec("function_materialized.yaml")
   suite.export_manifest("suite_manifest.yaml")

   same = fc.BenchmarkFunction("function_materialized.yaml")

C++ quick start
---------------

Use ``#include "funccraft.h"`` for normal C++ code. Evaluations are batched in
C++ too: pass ``std::vector<std::vector<double>>`` and receive one value per
point.

Load a suite YAML
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 0;

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

Use the packaged suite
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 0;
       const int year = 2026;
       const int version = 1;

       FuncCraft::BenchmarkSuite suite =
           FuncCraft::suite_collection(year, version).benchmark_suite(dimension);
       const FuncCraft::BenchmarkFunction& f =
           suite.function(function_index);

       std::vector<double> x = f.get_xopt();
       std::vector<double> values = f({x});

       std::cout << values.front() << '\n';
   }

Export a materialized spec
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   const FuncCraft::BenchmarkFunction& f = suite.function(function_index);
   f.export_spec("function_materialized.yaml");

   FuncCraft::BenchmarkFunction same =
       FuncCraft::make_benchmark_function("function_materialized.yaml");
