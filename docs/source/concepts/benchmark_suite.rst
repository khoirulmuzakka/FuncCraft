BenchmarkSuite
==============

``BenchmarkSuite`` represents a materialized collection of benchmark functions
for one chosen dimension. It is built from a ``SuiteSpec`` or from a packaged
suite collection.

The suite owns the generated function list. Retrieve a function by index when
you want to inspect it or pass it to an optimizer. Function indices are
one-based: valid indices are ``1`` through ``suite.size()``.

.. code-block:: python

   import funccraft as fc

   spec = fc.load_suite_spec("my_suite.yaml")
   suite = fc.BenchmarkSuite(spec, dimension=10)

   f = suite.function(1)
   values = f.evaluate([[0.0] * 10])

For a packaged suite:

.. code-block:: python

   suite = fc.suite_collection(year=2026, version=1).benchmark_suite(10)

Evaluation
----------

The recommended workflow is to materialize the benchmark function first, then
evaluate it:

.. code-block:: python

   function_index = 1
   f = suite.function(function_index)
   values = f.evaluate([[0.0] * 10, [1.0] * 10])

In C++, use ``suite.function(index)`` and call the returned function object:

.. code-block:: cpp

   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(10);
   const FuncCraft::BenchmarkFunction& f = suite.function(1);
   std::vector<double> values = f({std::vector<double>(10, 0.0)});

Dimension and identity
----------------------

The suite spec defines the generator recipe and requested number of functions.
The runtime ``BenchmarkSuite`` fixes the evaluation dimension.

For a fixed suite, function index, and seed, FuncCraft keeps the generated
function identity stable across dimensions where possible: primitive choices,
composition choices, transform choices, and related seeds stay aligned. Some
coordinate data changes with dimension, especially for full rotations. The
detailed guarantees are described in :ref:`design-constraints`.

Exporting
---------

Use ``export_manifest`` to write the normalized suite spec plus every generated
function spec:

.. code-block:: python

   suite.export_manifest("suite_manifest.yaml")

See :doc:`../user_guide/exporting` for archive and roundtrip details.
