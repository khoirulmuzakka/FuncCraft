Getting started
===============

Use the built-in suite collection:

.. code-block:: python

   import funccraft as fc

   collection = fc.suite_collection(2026, 1)
   suite = collection.benchmark_suite(dimension=10)
   values = suite.evaluate(0, [[0.0] * 10, [1.0] * 10])

All evaluations are batched. A single point must still be wrapped in a list:

.. code-block:: python

   value = suite.evaluate(0, [[0.0] * 10])[0]

Materialize a single function when you want to inspect its spec:

.. code-block:: python

   f = suite.function(0)
   print(f.spec.label)
   print(f.spec.assigned_xopt)
   print(f.spec.assigned_fopt)

Export a reproducible YAML spec:

.. code-block:: python

   f.export_spec("function.yaml")
   same = fc.BenchmarkFunction("function.yaml")
