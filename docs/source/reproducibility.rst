Reproducibility and YAML export
===============================

FuncCraft distinguishes between generator specs and materialized function
specs.

``SuiteSpec``
   High-level recipe for generating many functions.

``FunctionSpec``
   Complete description of one materialized function. It contains the assigned
   optimum, scale factor, component definitions, transform matrices when known,
   selected subspaces, DPM biases, seeds, label, and metadata.

Function export
---------------

Python:

.. code-block:: python

   import funccraft as fc

   f = fc.suite_collection(2026, 1).benchmark_suite(2).function(0)
   before = f([[0.0, 0.0]])
   f.export_spec("function.yaml")

   same = fc.BenchmarkFunction("function.yaml")
   after = same([[0.0, 0.0]])
   assert before == after

C++:

.. code-block:: cpp

   const FuncCraft::BenchmarkFunction& f = suite.function(0);
   std::vector<double> before = f({std::vector<double>(10, 0.0)});
   f.export_spec("function.yaml");

   FuncCraft::BenchmarkFunction same =
       FuncCraft::make_benchmark_function("function.yaml");

Suite export
------------

``BenchmarkSuite::export_manifest`` and ``BenchmarkSuite.export_manifest``
write the suite spec and every generated function spec.

Python:

.. code-block:: python

   suite.export_manifest("suite_manifest.yaml")

C++:

.. code-block:: cpp

   suite.export_manifest("suite_manifest.yaml");

The manifest is the preferred artifact to archive for experiments because it
captures the exact generated function table used by the run.

Evaluation contract
-------------------

Evaluation is batched in both interfaces:

- C++: ``std::vector<std::vector<double>>`` in, ``std::vector<double>`` out.
- Python: ``list[list[float]]`` in, ``list[float]`` out.

This keeps the interface consistent with vectorized optimizers and avoids
ambiguous interpretation of a single flat vector.
