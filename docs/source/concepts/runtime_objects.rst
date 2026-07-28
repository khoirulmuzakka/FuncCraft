Runtime objects and packaged suite
==================================

FuncCraft separates benchmark configuration from benchmark execution. YAML
files and YAML-shaped Python dictionaries describe what should be built:
which primitive functions are available, which transformations may be used,
which composition rules are allowed, and how many functions should be
generated. Runtime objects are the concrete objects produced from that
configuration for one chosen dimension.

There are two common workflows. For a generated benchmark set, create a
``BenchmarkSuite`` first, then retrieve individual ``BenchmarkFunction``
objects by one-based index:

.. code-block:: text

   Suite YAML/dictionary
       -> BenchmarkSuite at dimension d
       -> BenchmarkFunction F1, F2, ...
       -> batched evaluations

For a single hand-authored function, create the ``BenchmarkFunction``
directly:

.. code-block:: text

   Function YAML/dictionary
       -> BenchmarkFunction
       -> batched evaluations

The two central runtime objects are:

``BenchmarkFunction``
   One callable benchmark function. It has a fixed dimension, domain,
   assigned optimum location, assigned optimum value, components, transforms,
   composition rule, and final scale factor.

``BenchmarkSuite``
   A materialized collection of benchmark functions for one chosen dimension.
   Retrieve one function with ``suite.function(index)``.

Function indices are one-based: valid indices are ``1`` through
``suite.size()``. Evaluations are batched: pass a collection of points and
receive one value per point. In Python, call ``f.evaluate(points)``. In C++,
call ``f(points)``.

Python
------

.. code-block:: python

   import funccraft as fc

   suite = fc.SuiteCollection(year=2026, version=1).benchmark_suite(10)
   f = suite.function(1)
   values = f.evaluate([[0.0] * 10, [1.0] * 10])

   print(f.label)
   print(f.get_xopt())
   print(f.get_fopt())
   print(values)

C++
---

.. code-block:: cpp

   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(10);
   const FuncCraft::BenchmarkFunction& f = suite.function(1);

   std::vector<double> values = f({
       std::vector<double>(10, 0.0),
       std::vector<double>(10, 1.0),
   });

   std::vector<double> xopt = f.get_xopt();
   double fopt = f.get_fopt();

Construction paths
------------------

A ``BenchmarkFunction`` can come from:

``suite.function(index)``
   Access one function materialized by a ``BenchmarkSuite``.

``BenchmarkFunction({...})``
   Build one function directly from a YAML-shaped Python dictionary.

``BenchmarkFunction("function_materialized.yaml")``
   Reload an exported materialized function YAML file in Python.

A ``BenchmarkSuite`` can come from:

``BenchmarkSuite({...}, dimension)``
   Build a generated suite from a YAML-shaped Python dictionary.

``BenchmarkSuite("my_suite.yaml", dimension)``
   Build a generated suite from a suite YAML file.

``SuiteCollection(year=2026, version=1).benchmark_suite(dimension)``
   Load a packaged suite collection in Python.

``FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension)``
   Load a packaged suite collection in C++.

Dimension and identity
----------------------

The suite YAML/dictionary defines the generator recipe and requested number of
functions. The runtime ``BenchmarkSuite`` fixes the evaluation dimension.

For a fixed suite, function index, and seed, FuncCraft keeps the generated
function identity stable across dimensions where possible: primitive choices,
composition choices, transform choices, and related seeds stay aligned. Some
coordinate data changes with dimension, especially for full rotations. The
detailed guarantees are described in :ref:`design-constraints`.

Exporting
---------

Exported YAML is a materialized record of what FuncCraft built. Function
exports include generated matrices, centers, optima, seeds, scale factors, and
metadata needed to reproduce the already-built function. Suite manifests
include the normalized suite record and every generated function record.

.. code-block:: python

   f.export_yaml("function_materialized.yaml")
   suite.export_manifest("suite_manifest.yaml")

.. warning::

   The packaged ``2026_v1`` suite contains 1,000,000 functions. Exporting the
   full packaged suite manifest will write every generated function record and
   is usually unnecessary. If you only need a smaller reproducible subset, copy
   the suite configuration, lower ``requested_number_of_functions``, build that
   smaller suite, and export the smaller suite instead.

   .. code-block:: python

      import funccraft as fc

      collection = fc.SuiteCollection(year=2026, version=1)
      config = collection.config
      config["requested_number_of_functions"] = 500

      suite = fc.BenchmarkSuite(config, dimension=2)
      suite.export_manifest("suite_2026_v1_first_500.yaml")

   .. code-block:: cpp

      FuncCraft::SuiteSpec config = FuncCraft::suite_collection_spec(2026, 1);
      config.requested_number_of_functions = 500;

      FuncCraft::BenchmarkSuite suite(config, 2);
      suite.export_manifest("suite_2026_v1_first_500.yaml");

See :doc:`../python/examples` and :doc:`../cpp/examples` for complete
roundtrip examples.

FuncCraft 2026 v1 suite
-----------------------

The packaged ``2026_v1`` suite is the default published FuncCraft benchmark
suite. It is stored as YAML in ``suites/2026_v1.yaml`` and exposed through
``SuiteCollection(year=2026, version=1)`` in Python and
``FuncCraft::suite_collection(2026, 1)`` in C++.

.. code-block:: python

   import funccraft as fc

   dimension = 10
   suite = fc.SuiteCollection(year=2026, version=1).benchmark_suite(dimension)

.. code-block:: cpp

   const int dimension = 10;
   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension);

Suite YAML
~~~~~~~~~~

The packaged suite is defined by this YAML file:

.. code-block:: yaml

   # Base-function registry:
   # Some unimodal landscapes are very similar; this registry intentionally keeps
   # the full set here, but note the overlap among:
   # DixonPrice, BentCigar, Ellipsoidal.
   # 1: Sphere
   # 2: Ellipsoidal
   # 3: SumDifferentPowers
   # 4: BuecheRastrigin
   # 5: LinearSlope
   # 6: AttractiveSector
   # 7: StepEllipsoidal
   # 8: StepRastrigin
   # 9: Rosenbrock
   # 10: Ackley
   # 11: Rastrigin
   # 12: Griewank
   # 13: Schwefel
   # 14: SharpRidge
   # 15: Weierstrass
   # 16: SchafferF7
   # 17: SchafferF7Cond1000
   # 18: GriewankRosenbrock
   # 19: Gallagher21
   # 20: Katsuura
   # 21: LunacekBiRastrigin
   # 22: Zakharov
   # 23: Levy
   # 24: Michalewicz
   # 25: DixonPrice
   # 26: BentCigar
   # 27: HappyCat
   # 28: HGBat
   # 29: HCF
   # 30: SchafferF6
   # 31: Step
   # 32: Quartic
   # 33: Exponential
   # 34: StyblinskiTang
   # Multimodal ids:
   # 4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 23, 24, 27, 30, 34
   # Unimodal ids:
   # 1, 2, 3, 5, 6, 7, 9, 14, 22, 25, 26, 28, 29, 31, 32, 33
   supported_dimensions: any
   base_functions: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34]
   coordinate_transforms:
     - kind: none
       probability: 0.0
       parameters: []
     - kind: rotation
       probability: 0.34
       parameters: []
     - kind: affine
       probability: 0.33
       parameters: []
     - kind: subspace-rotation
       probability: 0.33
       parameters: []
   value_transforms:
     - kind: none
       probability: 0.5
       parameters: []
     - kind: power
       probability: 0.25
       parameters: []
     - kind: osc
       probability: 0.25
       parameters: []
     - kind: cosine-zero
       probability: 0.0
       parameters: []
   compositions:
     - kind: cpmsum
       probability: 0.1
       parameters: []
     - kind: cpmpmean
       probability: 0.1
       parameters: [3.0]
     - kind: cpmpmean
       probability: 0.1
       parameters: [0.1]
     - kind: cpmlwell
       probability: 0.2
       parameters: []
     - kind: dpmsoftmax
       probability: 0.25
       # parameters = [sharpness]
       parameters: [0.005]
     - kind: dpmbgsoftmax
       probability: 0.25
       #parameters = [sharpness, background_strength, background_sharpness]
       parameters : [0.005, 1.0, 0.01]

   composition_base_functions: [4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 23, 28, 30, 34, 33, 2, 3, 5, 9, 26, 27]
   min_components: 2
   max_components: 5
   max_nested_composition_depth: 1
   nested_probability: 0.1
   requested_number_of_functions: 1000000
   master_seed: 1
   lower_bound: -100
   upper_bound: 100
   assigned_fopt: 100.0
   xopt_domain_shrink_factor: 0.8
   suite_label: Funccraft-2026-benchmark-suite-v1

What the suite contains
~~~~~~~~~~~~~~~~~~~~~~~

``base_functions``
   The suite includes all 34 primitive base functions as mandatory
   single-function benchmarks. These occupy the leading function indices. See
   :doc:`primitive_base_functions` for the full ID table.

``composition_base_functions``
   Composed and nested functions sample primitive components from
   ``[4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 23, 28, 30, 34, 33,
   2, 3, 5, 9, 26, 27]``.

``coordinate_transforms``
   ``none`` is listed but disabled. Generated composed functions use
   ``rotation`` with probability ``0.34``, ``affine`` with probability
   ``0.33``, and ``subspace-rotation`` with probability ``0.33``.

``value_transforms``
   Component values use ``none`` with probability ``0.5``, ``power`` with
   probability ``0.25``, and ``osc`` with probability ``0.25``.
   ``cosine-zero`` is listed but disabled.

``compositions``
   Composed functions use ``cpmsum``, two ``cpmpmean`` variants,
   ``cpmlwell``, ``dpmsoftmax``, and ``dpmbgsoftmax`` with the probabilities
   and parameters shown in the YAML.

``min_components`` and ``max_components``
   Each generated composed function has between 2 and 5 immediate components.

``max_nested_composition_depth`` and ``nested_probability``
   Components may themselves be composed functions up to one nested level.
   Each eligible component has probability ``0.1`` of becoming nested.

``requested_number_of_functions``
   The suite requests 1,000,000 functions. The runtime ``BenchmarkSuite``
   reports the generated count through ``size()``.

``master_seed``
   The deterministic seed for suite generation. For a fixed suite YAML,
   dimension, and function index, this controls the generated structure and
   materialized parameters.

``lower_bound`` and ``upper_bound``
   The generated benchmark domain is ``[-100, 100]^d`` for the chosen
   dimension ``d``.

``assigned_fopt``
   Generated functions receive assigned optimum value ``100.0``.

``xopt_domain_shrink_factor``
   Generated optima and DPM centers are sampled from the central 80 percent of
   the benchmark domain.

``suite_label``
   The human-readable label stored in generated function YAML records and
   exported manifests.

Disabling options
~~~~~~~~~~~~~~~~~

The YAML intentionally keeps all implemented mechanism families visible.
Setting a choice probability to ``0`` disables that option without removing it
from the file. This makes suite edits easier to review because unavailable
choices remain explicit.
