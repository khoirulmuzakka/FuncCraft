Getting started
===============

FuncCraft evaluates benchmark functions in batches. A point is a vector of
numbers, and an evaluation input is a collection of points. Install FuncCraft
first from :doc:`installation`.

The usual workflow is:

.. code-block:: text

   SuiteSpec or packaged collection
       -> BenchmarkSuite(spec, dimension)
       -> suite.function(index)
       -> BenchmarkFunction
       -> f.evaluate(points)

Python quick start
------------------

Use the packaged suite when you want a published FuncCraft benchmark suite
exactly as shipped:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   function_index = 1

   suite = fc.suite_collection(year=2026, version=1).benchmark_suite(dimension)
   f = suite.function(function_index)

   points = [[0.0] * dimension, [1.0] * dimension]
   values = f.evaluate(points)

   print(f.spec.label)
   print(f.get_xopt())
   print(f.get_fopt())
   print(values)

C++ quick start
---------------

Use ``#include "funccraft.h"`` for normal C++ code:

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       const int dimension = 10;
       const int function_index = 1;

       FuncCraft::BenchmarkSuite suite =
           FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension);
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

Where to go next
----------------

Read :doc:`concepts/benchmark_function` and
:doc:`concepts/benchmark_suite` for the runtime object model. Use
:doc:`user_guide/yaml_specs` when you want to edit or author suite recipes,
and use :doc:`examples/python` or :doc:`examples/cpp` for longer examples.
