Core object model
=================

FuncCraft separates benchmark descriptions from runtime benchmark objects.
YAML files and spec classes describe what should be built. Runtime objects
evaluate concrete benchmark functions at a chosen dimension.

The central flow is:

.. code-block:: text

   Suite YAML
       -> SuiteSpec
       -> BenchmarkSuite(spec, dimension)
       -> BenchmarkFunction by index
       -> batched evaluations

For a single hand-authored function, the shorter flow is:

.. code-block:: text

   Function YAML
       -> FunctionSpec
       -> BenchmarkFunction
       -> batched evaluations

``BenchmarkSuite`` is the entry point for a benchmark set. ``BenchmarkFunction``
is the object that evaluates one concrete function. A suite can be built from
an editable ``SuiteSpec`` or from a packaged suite collection.

Specs are useful for editing, generation, review, and reproducibility. Runtime
objects are useful for evaluation, optimization experiments, inspection, and
exporting materialized records.

See :doc:`benchmark_suite` for suite behavior, :doc:`benchmark_function` for
single-function behavior, and :doc:`specs_vs_runtime` for how specs become
runtime objects.
