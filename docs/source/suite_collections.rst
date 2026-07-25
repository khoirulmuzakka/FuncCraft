Suite collections
=================

Suite collections are versioned benchmark-suite definitions shipped with
FuncCraft. They are backed by YAML files in ``suites/`` and exposed through a
stable API.

The current packaged collection is ``2026_v1``:

.. code-block:: python

   import funccraft as fc

   collection = fc.suite_collection(2026, 1)
   print(collection.name)
   print(collection.number_of_functions)

   suite = collection.benchmark_suite(10)

C++:

.. code-block:: cpp

   #include "funccraft.h"

   int main() {
       FuncCraft::SuiteCollection collection =
           FuncCraft::suite_collection(2026, 1);

       int n = collection.number_of_functions();
       FuncCraft::BenchmarkSuite suite =
           collection.benchmark_suite(10);
   }

When to use a collection
------------------------

Use ``suite_collection(year, version)`` when you want a published FuncCraft
suite exactly as shipped.

Use ``load_suite_spec("my_suite.yaml")`` when you want to edit the suite
recipe yourself. The YAML fields are described in :doc:`yaml_specs`.

Dimension and seeds
-------------------

The suite YAML defines the number of generated functions. You choose the
dimension when materializing a runtime ``BenchmarkSuite``.

For a given collection and function index, FuncCraft keeps structural choices
stable across dimensions: base/composed decision, base-function choices,
composition kind, coordinate-transform kind, value-transform kind, and the
relevant seeds. Generated coordinate vectors use prefix-stable generation
where applicable, so higher-dimensional assigned optima and DPM centers extend
lower-dimensional ones. The detailed design constraints and limitations are
described in :ref:`design-constraints`.
