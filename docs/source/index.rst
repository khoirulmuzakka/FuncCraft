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

2D Plots
========

The figure below shows the 34 primitive base functions in two dimensions,
without rotation, using the default domain and no scaling.

.. figure:: figs/base_functions.pdf
   :alt: 2D plots of the 34 primitive base functions
   :width: 100%

The next figure shows the first 500 functions from the FuncCraft Benchmark
Suite 2026 v1 in two dimensions.

.. figure:: figs/2D_plot_log.pdf
   :alt: 2D plots of the first 500 functions from FuncCraft Benchmark Suite 2026 v1
   :width: 100%


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
