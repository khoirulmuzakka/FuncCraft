Python API
==========

The supported Python API is intentionally small. Build concrete benchmark
objects from dictionaries, YAML files, or packaged suite collections; helper
functions used internally for YAML conversion are not part of the public API.

.. currentmodule:: funccraft

Runtime Objects
---------------

.. autoclass:: BenchmarkFunction
   :members:
   :show-inheritance:

.. autoclass:: BenchmarkSuite
   :members:
   :show-inheritance:

Public Functions
----------------

.. autofunction:: listSuiteCollections

Suite Collections
-----------------

.. autoclass:: SuiteCollection
   :members:
   :show-inheritance:

Primitive Functions
-------------------

.. autoclass:: BasicF
   :members:
   :show-inheritance:

.. autoclass:: BasicFunctionId
   :members:
   :show-inheritance:
