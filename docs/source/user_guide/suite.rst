FuncCraft 2026 v1 suite
=======================

The packaged ``2026_v1`` suite is the default published FuncCraft benchmark
suite. It is stored as YAML in ``suites/2026_v1.yaml`` and exposed through
``suite_collection(year=2026, version=1)``.

Load it in Python:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   suite = fc.suite_collection(year=2026, version=1).benchmark_suite(dimension)

Load it in C++:

.. code-block:: cpp

   const int dimension = 10;
   FuncCraft::BenchmarkSuite suite =
       FuncCraft::suite_collection(2026, 1).benchmark_suite(dimension);

Suite YAML
----------

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
     - kind: blockrotation
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
   max_number_of_functions: 0
   master_seed: 1
   lower_bound: -100
   upper_bound: 100
   assigned_fopt: 100.0
   xopt_domain_shrink_factor: 0.8
   suite_label: Funccraft-2026-benchmark-suite-v1

What the suite contains
-----------------------

``supported_dimensions: any``
   The suite can be materialized at any positive dimension. The YAML defines
   the generator recipe; the runtime ``BenchmarkSuite`` fixes the concrete
   dimension.

``base_functions``
   The suite includes all 34 primitive base functions as mandatory
   single-function benchmarks: ``[1, 2, 3, ..., 34]``. These occupy the
   leading function indices and include Sphere, Ellipsoidal,
   SumDifferentPowers, BuecheRastrigin, LinearSlope, AttractiveSector,
   StepEllipsoidal, StepRastrigin, Rosenbrock, Ackley, Rastrigin, Griewank,
   Schwefel, SharpRidge, Weierstrass, SchafferF7, SchafferF7Cond1000,
   GriewankRosenbrock, Gallagher21, Katsuura, LunacekBiRastrigin, Zakharov,
   Levy, Michalewicz, DixonPrice, BentCigar, HappyCat, HGBat, HCF, SchafferF6,
   Step, Quartic, Exponential, and StyblinskiTang. See
   :doc:`primitive_base_functions` for the full ID table.

``composition_base_functions``
   Composed and nested functions sample primitive components from this pool.
   The actual pool is ``[4, 8, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21,
   23, 28, 30, 34, 33, 2, 3, 5, 9, 26, 27]``. It emphasizes multimodal and
   structurally varied functions while also including selected unimodal
   functions.

``coordinate_transforms``
   The actual transform probabilities are:

   - ``none``: ``0.0``.
   - ``rotation``: ``0.34``.
   - ``affine``: ``0.33``.
   - ``blockrotation``: ``0.33``.

   The identity transform is listed with probability ``0.0`` so it remains
   visible in the suite definition but is disabled for generated composed
   functions.

``value_transforms``
   The actual value-transform probabilities are:

   - ``none``: ``0.5``.
   - ``power``: ``0.25``.
   - ``osc``: ``0.25``.
   - ``cosine-zero``: ``0.0``.

   Component values can be left unchanged, transformed with a power transform,
   or transformed with an oscillatory transform. ``cosine-zero`` is listed but
   disabled in this suite.

``compositions``
   Composed functions use a mixture of common-point and deceptive-point
   composition modes:

   - ``cpmsum`` with probability ``0.1``: common-point weighted sum.
   - ``cpmpmean`` with probability ``0.1`` and parameters ``[3.0]``: power
     mean with larger exponent.
   - ``cpmpmean`` with probability ``0.1`` and parameters ``[0.1]``: power
     mean with smaller exponent.
   - ``cpmlwell`` with probability ``0.2``: common-point level-well
     composition.
   - ``dpmsoftmax`` with probability ``0.25`` and parameters ``[0.005]``:
     deceptive softmax composition.
   - ``dpmbgsoftmax`` with probability ``0.25`` and parameters ``[0.005, 1.0,
     0.01]``: deceptive softmax with a background term.

``min_components`` and ``max_components``
   Each generated composed function has between 2 and 5 immediate components.

``max_nested_composition_depth`` and ``nested_probability``
   Components may themselves be composed functions up to one nested level.
   Each eligible component has probability ``0.1`` of becoming nested.

``requested_number_of_functions`` and ``max_number_of_functions``
   The suite requests 1,000,000 functions. ``max_number_of_functions: 0``
   means there is no separate hard cap beyond the requested count.

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
   The human-readable label stored in generated function specs and exported
   manifests.

Disabling options
-----------------

The YAML intentionally keeps all implemented mechanism families visible.
Setting a choice probability to ``0`` disables that option without removing it
from the file. This makes suite edits easier to review because unavailable
choices remain explicit.
