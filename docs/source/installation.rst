Installation
============

FuncCraft can be used as a Python package or as a native C++ library.

Python package
--------------

Install the Python interface from PyPI:

.. code-block:: shell

   python -m pip install --upgrade funccraft

Check the installation:

.. code-block:: shell

   python -c "import funccraft as fc; print(fc.suite_collection(2026, 1).name)"

Optional optimization examples use SciPy or MinionPy:

.. code-block:: shell

   python -m pip install scipy minionpy

Build from source
-----------------

Requirements:

- CMake 3.18 or newer
- a C++17 compiler
- macOS 10.15 or newer when building on macOS, because FuncCraft uses
  C++17 ``std::filesystem``
- Python 3.9 or newer for the Python interface
- ``pybind11`` for Python builds
- ``yaml-cpp`` or network access so CMake can fetch ``yaml-cpp``

From the repository root:

.. code-block:: shell

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ^
     -DBUILD_LIBRARY=ON ^
     -DBUILD_PYTHON=ON ^
     -DBUILD_EXAMPLES=OFF ^
     -DBUILD_TEST=ON
   cmake --build build --config Release

On Linux or macOS:

.. code-block:: shell

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_LIBRARY=ON \
     -DBUILD_PYTHON=ON \
     -DBUILD_EXAMPLES=OFF \
     -DBUILD_TEST=ON
   cmake --build build --config Release

Helper scripts are also provided:

.. code-block:: shell

   ./compile.sh
   ./compile.sh --debug

On Windows:

.. code-block:: bat

   compile.bat
   compile.bat --debug

Build options
-------------

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - Option
     - Default
     - Purpose
   * - ``BUILD_LIBRARY``
     - ``ON``
     - Build the native ``funccraft`` static library.
   * - ``BUILD_PYTHON``
     - ``ON``
     - Build the Python extension module.
   * - ``BUILD_EXAMPLES``
     - ``OFF``
     - Build C++ examples. This enables the Minion dependency for optimization examples.
   * - ``BUILD_TEST``
     - ``OFF``
     - Build C++ tests. Minion is not required.
   * - ``FUNCCRAFT_INSTALL``
     - ``OFF``
     - Install headers, library, CMake package metadata, and suite YAML files.

Build a wheel
-------------

.. code-block:: shell

   python -m pip install --upgrade build
   python -m build --wheel
   python -m pip install dist/funccraft-*.whl

Build the documentation
-----------------------

The documentation uses Doxygen for C++ API XML and Sphinx with the Read the
Docs theme for HTML pages.

.. code-block:: shell

   python -m pip install sphinx sphinx-rtd-theme breathe
   doxygen Doxyfile
   cd docs
   make html

On Windows:

.. code-block:: bat

   doxygen Doxyfile
   cd docs
   make.bat html

The generated HTML is written to ``docs/build/html``.
