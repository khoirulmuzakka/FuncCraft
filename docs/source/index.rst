FuncCraft documentation
=======================

FuncCraft is a C++17 library with a Python interface for generating scalable
black-box optimization benchmark suites. It is designed around editable YAML
specifications: write or edit a suite YAML file, load it in C++ or Python,
choose a dimension, and evaluate the generated benchmark functions in batches.

A suite specification can generate hundreds, thousands, or practically
unlimited numbers of distinct benchmark functions across dimensions while
controlling the constructed optimum location and optimum value. The generated
landscapes are assembled from primitive benchmark functions, coordinate
transforms, value transforms, and composition rules.

Start with :doc:`installation` and :doc:`getting_started`. The two central
runtime objects are :doc:`concepts/benchmark_function` and
:doc:`concepts/benchmark_suite`.

.. toctree::
   :maxdepth: 2
   :caption: Start here

   installation
   getting_started

.. toctree::
   :maxdepth: 2
   :caption: Core concepts

   concepts/overview
   concepts/benchmark_function
   concepts/benchmark_suite
   concepts/specs_vs_runtime

.. toctree::
   :maxdepth: 2
   :caption: User guide

   user_guide/packaged_suites
   user_guide/yaml_specs
   user_guide/construction_model
   user_guide/mechanisms
   user_guide/exporting
   user_guide/plotting

.. toctree::
   :maxdepth: 2
   :caption: Examples

   examples/python
   examples/cpp

.. toctree::
   :maxdepth: 2
   :caption: API reference

   api/python
   api/cpp

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
