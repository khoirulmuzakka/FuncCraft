Getting Started
===============

FuncCraft evaluates benchmark functions in batches. A point is a vector of
numbers, and an evaluation input is a collection of points. Install FuncCraft
first from :doc:`installation`, then choose the interface you want to use.

Python
------

Python users usually start with the packaged suite or a YAML file:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   suite = fc.SuiteCollection(2026, 1).benchmark_suite(dimension)
   f = suite.function(1)
   values = f.evaluate([[0.0] * dimension, [1.0] * dimension])

   print(f.label)
   print(values)

Continue with :doc:`python/examples`.

C++
---

C++ users can use packaged suites, YAML loading, or direct typed construction:

.. code-block:: cpp

   #include "funccraft.h"

   #include <iostream>
   #include <vector>

   int main() {
       const int dimension = 10;
       FuncCraft::BenchmarkSuite suite =
           FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension);
       const FuncCraft::BenchmarkFunction& f = suite.function(1);

       std::vector<std::vector<double>> points = {
           std::vector<double>(dimension, 0.0),
           std::vector<double>(dimension, 1.0),
       };
       std::vector<double> values = f(points);

       std::cout << f.label() << '\n';
       std::cout << values.front() << '\n';
   }

Continue with :doc:`cpp/examples`.
