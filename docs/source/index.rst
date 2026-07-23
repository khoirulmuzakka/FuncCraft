Welcome to FuncCraft's documentation
====================================

FuncCraft is a C++17 library, with a Python interface, for scalable generation
of reproducible continuous-optimization benchmark functions. A single suite
specification can generate hundreds, thousands, or practically unbounded
numbers of distinct benchmark instances across dimensions while preserving the
metadata needed to reproduce each function exactly.

FuncCraft is designed around controlled benchmark construction: the generated
function has an assigned global minimizer and assigned optimum value, while the
surrounding landscape is assembled from primitive benchmark functions,
coordinate transforms, value transforms, and composition functions.

This documentation covers the construction model, installation, C++ API,
Python API, built-in suite collections, and reproducibility workflow.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   installation
   construction
   mechanisms
   suite_collections
   reproducibility
   funccraft_cpp/index
   funccraft_py/index
   testing_ci
   license

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
