Testing and CI
==============

The standard CI builds the C++ library and Python interface on supported
platforms, runs the C++ and Python tests, validates generated benchmark values,
and checks that the package can be built as a wheel.

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
per-function relative differences.

This is intended to catch platform-sensitive changes in floating-point
behavior while allowing small numerical drift from library/compiler
differences.
