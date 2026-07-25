Testing and CI
==============

The standard CI builds the C++ library, Python interface, and C++ test binary
on supported platforms, runs the C++ and Python tests, validates generated
benchmark values across platforms, and checks that the source package can be
built. The separate wheel workflow builds and tests installable wheels.

Local checks
------------

Build and run the C++ test:

.. code-block:: shell

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TEST=ON
   cmake --build build --config Release
   ctest --test-dir build --output-on-failure -C Release

Run the Python test:

.. code-block:: shell

   python tests/test.py

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
