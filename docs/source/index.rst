FuncCraft documentation
=======================

FuncCraft is a C++17 library with a Python interface for generating scalable
continuous-optimization benchmark suites. It is designed around editable YAML
specifications: write or edit a suite YAML file, load it in C++ or Python,
choose a dimension, and evaluate the generated benchmark functions in batches.

A suite specification can generate hundreds, thousands, or practically
unlimited numbers of distinct benchmark functions across dimensions while
controlling the constructed optimum location and optimum value. The generated
landscapes are assembled from primitive benchmark functions, coordinate
transforms, value transforms, and composition rules.

Start with :doc:`funccraft_py/getting_started` for Python and C++ quick
starts. The YAML reference is in :doc:`yaml_specs`.

.. toctree::
   :maxdepth: 2
   :caption: User guide

   installation
   funccraft_py/getting_started
   yaml_specs
   suite_collections
   suite_plots
   construction
   mechanisms
   exported_manifests

.. toctree::
   :maxdepth: 2
   :caption: Interfaces

   funccraft_py/index
   funccraft_cpp/index

.. toctree::
   :maxdepth: 1
   :caption: Project

   testing_ci
   license

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
