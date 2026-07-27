FuncCraft Documentation
=======================

FuncCraft is a C++17 library with a Python interface for generating scalable
black-box optimization benchmark suites. It builds benchmark functions from
editable YAML files, packaged suite collections, primitive benchmark
functions, coordinate transforms, value transforms, and composition rules.

Choose the language track first:

``Python``
   A compact YAML/dictionary-first interface with a small public API.

``C++``
   A native API with runtime objects plus typed construction structs for
   advanced control.

The shared concept pages explain the construction model and implemented
mechanisms without assuming either interface.

.. toctree::
   :maxdepth: 2
   :caption: Start Here

   installation


.. toctree::
   :maxdepth: 2
   :caption: Shared Concepts

   concepts/construction_model
   concepts/mechanisms
   concepts/runtime_objects
   concepts/primitive_base_functions


.. toctree::
   :maxdepth: 2
   :caption: Examples and API

   python/index
   cpp/index


.. toctree::
   :maxdepth: 1
   :caption: Project

   project/testing_ci
   project/license

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
