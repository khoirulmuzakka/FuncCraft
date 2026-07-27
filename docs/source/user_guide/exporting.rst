Exported manifests
==================

Input YAML is for configuration and editing. Exported YAML is a materialized
record of what FuncCraft built at runtime.

Function export
---------------

``BenchmarkFunction::export_spec`` and ``BenchmarkFunction.export_spec`` write
one materialized function spec. Exported function specs include generated
details such as transform matrices, selected subspaces, DPM centers and
biases, assigned optima, scale factors, labels, and metadata.

Python:

.. code-block:: python

   import funccraft as fc

   function_index = 1
   year = 2026
   version = 1
   f = fc.suite_collection(year, version).benchmark_suite(2).function(function_index)
   f.export_spec("function_materialized.yaml")

   same = fc.BenchmarkFunction("function_materialized.yaml")

C++:

.. code-block:: cpp

   const int function_index = 1;
   const FuncCraft::BenchmarkFunction& f = suite.function(function_index);
   f.export_spec("function_materialized.yaml");

   FuncCraft::BenchmarkFunction same =
       FuncCraft::make_benchmark_function("function_materialized.yaml");

Suite manifest export
---------------------

``BenchmarkSuite::export_manifest`` and ``BenchmarkSuite.export_manifest``
write the normalized suite spec plus every generated function spec.

Python:

.. code-block:: python

   suite.export_manifest("suite_manifest.yaml")

C++:

.. code-block:: cpp

   suite.export_manifest("suite_manifest.yaml");

Use exported manifests when you want to archive the exact generated function
table used by an experiment.

Evaluation contract
-------------------

Evaluation is batched in both interfaces:

- C++: ``std::vector<std::vector<double>>`` in, ``std::vector<double>`` out.
- Python: ``list[list[float]]`` in, ``list[float]`` out.

A single point must still be wrapped as a one-element batch.
