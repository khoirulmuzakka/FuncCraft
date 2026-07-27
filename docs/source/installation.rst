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

   python -c "import funccraft as fc; print(fc.SuiteCollection(year=2026, version=1).name)"

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

To build and install the Python package from a local checkout, run this from
the repository root:

.. code-block:: shell

   python -m pip install .

This builds a wheel locally, compiles the native extension, and installs the
result into the active Python environment.

To keep the wheel artifact instead of only installing it:

.. code-block:: shell

   python -m pip install --upgrade build
   python -m build --wheel

The wheel is written to ``dist/`` and can be installed with:

.. code-block:: shell

   python -m pip install dist/funccraft-*.whl

For editable Python development:

.. code-block:: shell

   python -m pip install -e .

Native C++ build
----------------

To build the native C++ library and tests directly with CMake, run this from
the repository root.

On Windows:

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

Use in another C++ project
--------------------------

For a CMake-based C++ project, the simplest way to consume FuncCraft directly
from Git is ``FetchContent``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.18)
   project(my_funccraft_app LANGUAGES CXX)

   include(FetchContent)

   set(BUILD_LIBRARY ON CACHE BOOL "" FORCE)
   set(BUILD_PYTHON OFF CACHE BOOL "" FORCE)
   set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
   set(BUILD_TEST OFF CACHE BOOL "" FORCE)

   FetchContent_Declare(
       funccraft
       GIT_REPOSITORY https://github.com/khoirulmuzakka/FuncCraft.git
       GIT_TAG master
   )
   FetchContent_MakeAvailable(funccraft)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE funccraft)

For reproducible builds, replace ``GIT_TAG master`` with a released tag or a
specific commit hash.

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
