Testing and CI
==============

The standard CI builds the C++ library, Python interface, and C++ test binary
on supported platforms, runs the C++ and Python tests, validates generated
benchmark values across platforms, and checks that the source package can be
built. The separate wheel workflow builds and tests installable wheels.

Local checks
------------

Build and run the C++ test binary:

.. code-block:: shell

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TEST=ON
   cmake --build build --config Release
   ./bin/funccraft_test.exe

On Linux and macOS, the executable is named ``funccraft_test``. The test
prints a named report such as ``[ 1/15] RUN ...`` and ``[ 1/15] PASS ...``,
then ends with the number of passed and failed checks. A successful run ends
with ``Overall status: PASS``.

The same test can also be launched through CTest:

.. code-block:: shell

   ctest --test-dir build --output-on-failure -C Release

Run the Python test:

.. code-block:: shell

   python tests/test.py


C++ test coverage
-----------------

The C++ test binary prints a named report. The checks cover these groups:

- ``Packaged suite optima``: builds the packaged ``2026_v1`` suite at
  dimension 2 and verifies that the
  first primitive benchmark functions evaluate close to their assigned optimum
  value at ``assigned_xopt``.

- ``Suite collection API``: verifies that the collection registry can find
  ``2026_v1``, that the
  reported year/version/function count match the loaded suite spec, and that
  a materialized suite has the requested dimension.

- ``Identity transform assigned optimum``,
  ``Native-domain scaled optimum``, and
  ``Native-domain optimum in high dimension``: check that assigned optima
  remain correct after coordinate transforms and after mapping between the
  benchmark domain and each primitive base function's native domain.

- ``Block rotation subspace output``: checks the low-level block-rotation
  transform directly. The transform
  selects a subspace from the parent vector and returns only the rotated
  child-space coordinates.

- ``Composed function component`` and
  ``Reject nonzero nested assigned_fopt``: check nested benchmark-function
  components. Nested components must have ``assigned_fopt = 0`` because only
  the outermost function owns the final optimum value.

- ``Suite YAML accepts base-function names``,
  ``Composition kind aliases``,
  ``Function YAML roundtrip``, and
  ``Suite YAML roundtrip``: check YAML parsing, canonical name aliases,
  exported function specs, and exported suite manifests. Roundtrip checks
  evaluate the function before and after export/import and require matching
  values.

- ``Suite structure stable across dimensions``: builds the same suite spec at
  dimensions 2 and 5. For every generated
  function index, it checks that the generated structure is the same:
  function seed, component count, composition kind and parameters, DPM biases,
  component seeds, base-vs-nested source, base-function choices, coordinate
  transform kind and seed, value-transform kind and parameters, and the same
  recursive structure inside nested composed components. This test checks
  stable function identity, not coordinate values or matrices.

- ``Suite geometry prefix-stable across dimensions``: builds a non-nested DPM
  suite that uses only block rotation, then compares
  dimensions 4 and 8. It checks that ``assigned_xopt`` and each DPM center in
  4D are exact prefixes of the corresponding 8D vectors. It also checks that
  each component's coordinate-transform ``assigned_xopt`` and block
  ``selected_indices`` are prefix-stable. The test is intentionally
  non-nested, because nested block rotation can reduce the local child
  dimension; if the local dimension is smaller than the component count,
  subspace reuse may be necessary.

- ``Direct function geometry prefix-stable``: constructs two hand-written DPM
  ``FunctionSpec`` objects directly, one at
  dimension 1 and one at dimension 4, with omitted ``assigned_xopt`` and DPM
  centers. The ``BenchmarkFunction`` constructor generates those values and
  the test checks that the lower-dimensional generated geometry is an exact
  prefix of the higher-dimensional geometry. This validates prefix-stable
  behavior outside the suite generator.
  

Python test coverage
--------------------

The Python test is a public-API smoke and regression test. It verifies that the
installed or local ``funccraft`` package can import the main classes and helper
constructors, load the packaged ``2026_v1`` suite collection, build benchmark
suites, evaluate functions, export a function spec to YAML, reload it, export a
suite manifest to YAML, reload it, and reproduce the same sampled function
values after each roundtrip. It also checks the exposed DPM composition kinds
through the Python interface. The detailed numerical and construction checks
live in the C++ test binary.


Cross-platform value comparison
-------------------------------

The C++ test can generate platform value tables. The Python script
``tests/compare_values.py`` compares the generated tables and reports
per-function relative differences and the fraction of sampled points within
the configured relative tolerance.

This is intended to catch platform-sensitive changes in floating-point
behavior while allowing small numerical drift from library/compiler
differences.

In CI, the comparison succeeds when at least 95% of functions have at least
95% of sampled values agree within ``1e-8`` relative error.
