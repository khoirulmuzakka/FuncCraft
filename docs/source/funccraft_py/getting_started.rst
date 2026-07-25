Getting started
===============

FuncCraft evaluates benchmark functions in batches. A point is a list of
numbers, and an evaluation input is a list of points.

Install the package:

.. code-block:: shell

   python -m pip install --upgrade funccraft

Load a suite YAML
-----------------

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
----------------------

The packaged 2026 suite is also defined by YAML, but it is exposed through a
short collection API:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   collection = fc.suite_collection(2026, 1)
   suite = collection.benchmark_suite(dimension)

   function_index = 0
   f = suite.function(function_index)
   values = f.evaluate([[0.0] * dimension])

Inspect a function
------------------

.. code-block:: python

   print(f.spec.label)
   print(f.spec.assigned_xopt)
   print(f.spec.assigned_fopt)
   print(f.component_types)

Export a materialized spec
--------------------------

Exported YAML is a materialized record of a function or suite. It is useful
for archiving the exact generated table used by an experiment.

.. code-block:: python

   f.export_spec("function_materialized.yaml")
   suite.export_manifest("suite_manifest.yaml")

   same = fc.BenchmarkFunction("function_materialized.yaml")
