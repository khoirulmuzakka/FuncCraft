Core object model
=================

FuncCraft separates editable YAML/dictionary configuration from runtime
benchmark objects. YAML files and YAML-shaped Python dictionaries describe
what should be built. Runtime objects evaluate concrete benchmark functions at
a chosen dimension.

The central flow is:

.. code-block:: text

   Suite YAML
       -> BenchmarkSuite(yaml_or_dict, dimension)
       -> BenchmarkFunction by index
       -> batched evaluations

For a single hand-authored function, the shorter flow is:

.. code-block:: text

   Function YAML
       -> BenchmarkFunction(yaml_or_dict)
       -> batched evaluations

``BenchmarkSuite`` is the entry point for a benchmark set. ``BenchmarkFunction``
is the object that evaluates one concrete function. A suite can be built from
editable YAML/dictionary configuration or from a packaged suite collection.

YAML files and dictionaries are useful for editing, generation, review, and
reproducibility. Runtime objects are useful for evaluation, optimization
experiments, inspection, and exporting materialized records.

See :doc:`benchmark_suite` for suite behavior, :doc:`benchmark_function` for
single-function behavior, and :doc:`../python/examples` for YAML-shaped
dictionary fields.
