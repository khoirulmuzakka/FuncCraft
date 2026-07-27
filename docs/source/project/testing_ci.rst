Testing and CI
==============

FuncCraft uses two GitHub Actions workflows:

``ci.yml``
   Builds the C++ library, Python extension, and C++ test binary on Linux,
   macOS arm, and Windows. It runs the C++ tests, runs the Python public-API
   test, builds the Python source/wheel artifacts with ``python -m build``,
   generates cross-platform value tables, and compares those tables.

``wheel.yaml``
   Builds installable wheels with ``cibuildwheel`` for CPython 3.9 through
   3.14 on Linux, macOS arm, macOS x86_64, and Windows. Each wheel is installed
   and tested with the Python test in installed-package mode.

Local checks
------------

Build and run the C++ test binary:

.. code-block:: shell

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TEST=ON
   cmake --build build --config Release --target funccraft_test
   ./bin/funccraft_test

On Windows, run:

.. code-block:: bat

   .\bin\funccraft_test.exe

The C++ test binary can also be launched through CTest:

.. code-block:: shell

   ctest --test-dir build --output-on-failure -C Release

Run the Python test against the local source-tree package and locally compiled
extension:

.. code-block:: shell

   python tests/test.py

Run the same Python test against an installed package or wheel:

.. code-block:: shell

   python tests/test.py --installed

In installed-package mode, the test removes the repository root from
``sys.path`` so it cannot accidentally import the checkout's ``funccraft/``
directory instead of the installed wheel.

C++ test coverage
-----------------

The C++ binary prints a named report with 17 checks and finishes with
``Overall status: PASS`` on success. The checks are:

``Basic function registry ids``
   Verifies that the primitive base-function registry is nonempty, contiguous
   from ID 1, and starts with the expected leading functions.

``Packaged suite optima``
   Builds the packaged ``2026_v1`` suite at dimension 10 and checks the first
   500 functions, or fewer if the suite is smaller. Each checked function is
   evaluated at ``assigned_xopt`` and compared with its assigned optimum value.

``Suite collection API``
   Verifies the collection registry, ``2026_v1`` year/version metadata,
   reported function count, suite label, materialized suite size, and function
   dimension.

``Identity transform assigned optimum``, ``Native-domain scaled optimum``, and ``Native-domain optimum in high dimension``
   Check assigned optimum behavior for primitive functions, coordinate
   transforms, and mapping between benchmark domains and primitive native
   domains. The high-dimensional check covers dimensions 1 and 100.

``Block rotation subspace output``
   Checks the low-level block-rotation transform directly, including selected
   subspace output and input/output dimensions.

``Composed function component`` and ``Reject nonzero nested assigned_fopt``
   Check nested composed-function components. Nested components must have
   ``assigned_fopt = 0`` because only the outermost function owns the final
   optimum value.

``Suite YAML accepts base-function names`` and ``Composition kind aliases``
   Check YAML parsing for base-function names and canonical handling of DPM
   composition aliases.

``Suite structure stable across dimensions``
   Builds the same generated suite at dimensions 2 and 5 and checks that every
   function keeps the same generated structure: seeds, component counts,
   base-vs-nested choices, base-function choices, composition kind and
   parameters, transform choices, value-transform choices, and recursive nested
   structure.

``Suite geometry prefix-stable across dimensions``
   Builds a non-nested block-rotation DPM suite at dimensions 4 and 8. It
   checks prefix-stability of ``assigned_xopt``, DPM centers, component
   transform ``assigned_xopt`` values, and block ``selected_indices``.

``Direct function geometry prefix-stable``
   Constructs direct DPM ``FunctionSpec`` objects at dimensions 1 and 4 with
   omitted generated geometry, then checks that generated lower-dimensional
   geometry is an exact prefix of the higher-dimensional geometry.

``Function YAML roundtrip`` and ``Suite YAML roundtrip``
   Export materialized function YAML records and suite manifests, reload them, and
   require matching evaluations on deterministic candidate points.

``Packaged suite manifest exact function-spec roundtrip``
   Builds 500 functions from the packaged suite at dimension 10. For each
   function, it evaluates 1000 deterministic points, exports the suite
   manifest, rebuilds each exported function YAML record as a standalone
   ``BenchmarkFunction``, and requires exact ``double`` equality on the same
   points.

Python test coverage
--------------------

``tests/test.py`` is a public-API smoke and regression test. It verifies that
the package can import the main classes, enums, helper constructors, packaged
suite collections, and roundtrip helpers.

The Python test checks:

- primitive base-function IDs are contiguous from 1, and ID 0 is rejected;
- the packaged ``2026_v1`` collection reports consistent metadata;
- packaged suites can be materialized and evaluated;
- the first 500 packaged-suite functions evaluate near their assigned optima;
- DPM composition kinds roundtrip through direct functions and generated
  suites;
- function YAML export/import reproduces sampled values;
- suite manifest export/import reproduces sampled values.

Cross-platform value comparison
-------------------------------

The C++ test binary supports value-table generation:

.. code-block:: shell

   ./bin/funccraft_test --generate-values linux values_linux.txt

The generated table uses the packaged ``2026_v1`` suite at dimension 10,
records one-based function indices ``1`` through ``500``, and evaluates 1000
deterministic points per function.

In ``ci.yml``, Linux, macOS arm, and Windows each upload one value table. The
``compare-values`` job downloads all tables and runs:

.. code-block:: shell

   python tests/compare_values.py value_tables/values_linux.txt value_tables/values_macos_arm.txt value_tables/values_windows.txt --tolerance 1e-8 --strict-function-count 34 --strict-point-agreement 1.0 --point-agreement 0.75 --function-agreement 0.75

The first 34 functions are the primitive-function prefix and must have 100%
point agreement within ``1e-8`` relative tolerance. The remaining functions
are checked statistically: at least 75% of sampled points must agree for a
function to pass, and at least 75% of those generated functions must pass.

Wheel testing
-------------

The wheel workflow builds wheels through ``cibuildwheel`` and uses:

.. code-block:: shell

   python {project}/tests/test.py --installed

This command runs the repository's test file but imports ``funccraft`` from the
installed wheel. This catches missing wheel files, extension import failures,
and packaging mistakes that local source-tree tests may hide.
