BenchmarkFunction
=================

``BenchmarkFunction`` represents one materialized benchmark function. It has a
fixed dimension, domain, assigned optimum location, assigned optimum value,
components, transforms, composition rule, and final scale factor.

This is the object optimizers usually call.

Python
------

.. code-block:: python

   import funccraft as fc

   suite = fc.SuiteCollection(year=2026, version=1).benchmark_suite(10)
   f = suite.function(1)

   values = f.evaluate([[0.0] * 10, [1.0] * 10])

   print(f.get_xopt())
   print(f.get_fopt())
   print(f.domain)
   print(f.component_types)

C++
---

.. code-block:: cpp

   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(10);
   const FuncCraft::BenchmarkFunction& f = suite.function(1);

   std::vector<double> values = f({
       std::vector<double>(10, 0.0),
       std::vector<double>(10, 1.0),
   });

   std::vector<double> xopt = f.get_xopt();
   double fopt = f.get_fopt();

Evaluation is batched: pass many points and receive one value per point. In
Python, call ``f.evaluate(points)``. In C++, call ``f(points)``.

Construction paths
------------------

A ``BenchmarkFunction`` can come from:

``suite.function(index)``
   Access one function materialized by a ``BenchmarkSuite``.

``BenchmarkFunction({...})``
   Build one function directly from a YAML-shaped Python dictionary.

``BenchmarkFunction("function_materialized.yaml")``
   Reload an exported materialized function YAML file in Python.

Exporting
---------

Use ``export_yaml`` to write a materialized YAML record:

.. code-block:: python

   f.export_yaml("function_materialized.yaml")

The exported file contains generated matrices, centers, optima, seeds, and
other values needed to reproduce the already-built function without rerunning
the suite generator. See :doc:`../python/examples` and
:doc:`../cpp/examples` for details.
