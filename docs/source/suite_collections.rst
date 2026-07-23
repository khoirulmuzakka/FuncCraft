Suite collections
=================

Suite collections are versioned benchmark-suite definitions shipped as YAML.
The current package contains ``suites/2026_v1.yaml`` and exposes it through
``SuiteCollection``.

Why collections exist
---------------------

A ``SuiteSpec`` is a generator recipe. It defines the base-function pool,
choice probabilities, component-count range, nesting policy, bounds, optimum
placement policy, and number of requested functions. A ``SuiteCollection`` is
a stable named access point for a published recipe, identified by year and
version.

Python
------

.. code-block:: python

   import funccraft as fc

   collection = fc.suite_collection(2026, 1)
   print(collection.name)
   print(collection.number_of_functions)

   suite = collection.benchmark_suite(10)
   f0 = suite.function(0)
   values = suite.evaluate(0, [[0.0] * 10])

C++
---

.. code-block:: cpp

   #include "funccraft.h"

   int main() {
       FuncCraft::SuiteCollection collection =
           FuncCraft::suite_collection(2026, 1);

       int n = collection.number_of_functions();
       FuncCraft::SuiteSpec spec = collection.spec();
       FuncCraft::BenchmarkSuite suite = collection.benchmark_suite(10);
   }

Dimension and seed behavior
---------------------------

For a given suite collection and function index, FuncCraft keeps the structural
choices stable across dimensions: base/composed decision, base-function choices,
composition kind, coordinate-transform kind, value-transform kind, and the
relevant seeds. Geometry details such as generated coordinate vectors are
dimension-dependent but use prefix-stable generation where applicable, so
higher-dimensional runs extend lower-dimensional coordinates for assigned
optimum locations and DPM centers.

The number of functions is defined by the suite YAML. Users choose only the
dimension when materializing a collection into a runtime ``BenchmarkSuite``.
