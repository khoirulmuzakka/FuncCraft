Primitive base function IDs
===========================

Primitive functions are exposed through ``BasicFunctionId``. Numeric IDs and
names can both be used in YAML fields such as ``base_functions``,
``composition_base_functions``, and component ``base_function`` entries.

.. list-table::
   :header-rows: 1
   :widths: 10 35 55

   * - ID
     - Name
     - Notes
   * - 1
     - Sphere
     - Unimodal, smooth, separable.
   * - 2
     - Ellipsoidal
     - Unimodal, high-conditioned.
   * - 3
     - SumDifferentPowers
     - Unimodal, variable powers.
   * - 4
     - BuecheRastrigin
     - Multimodal Rastrigin-family landscape.
   * - 5
     - LinearSlope
     - Unimodal sloped landscape.
   * - 6
     - AttractiveSector
     - Unimodal, asymmetric sector structure.
   * - 7
     - StepEllipsoidal
     - Unimodal stepped high-conditioned landscape.
   * - 8
     - StepRastrigin
     - Multimodal stepped Rastrigin-family landscape.
   * - 9
     - Rosenbrock
     - Unimodal valley, nonseparable.
   * - 10
     - Ackley
     - Multimodal.
   * - 11
     - Rastrigin
     - Multimodal, separable in primitive coordinates.
   * - 12
     - Griewank
     - Multimodal.
   * - 13
     - Schwefel
     - Multimodal.
   * - 14
     - SharpRidge
     - Unimodal ridge.
   * - 15
     - Weierstrass
     - Multimodal, rugged.
   * - 16
     - SchafferF7
     - Multimodal.
   * - 17
     - SchafferF7Cond1000
     - Multimodal, conditioned Schaffer variant.
   * - 18
     - GriewankRosenbrock
     - Multimodal hybrid.
   * - 19
     - Gallagher21
     - Multimodal peaks.
   * - 20
     - Katsuura
     - Multimodal.
   * - 21
     - LunacekBiRastrigin
     - Multimodal double-funnel Rastrigin-family landscape.
   * - 22
     - Zakharov
     - Unimodal.
   * - 23
     - Levy
     - Multimodal.
   * - 24
     - Michalewicz
     - Multimodal.
   * - 25
     - DixonPrice
     - Unimodal.
   * - 26
     - BentCigar
     - Unimodal, high-conditioned.
   * - 27
     - HappyCat
     - Multimodal/nonconvex BBOB-style function.
   * - 28
     - HGBat
     - Unimodal/nonconvex BBOB-style function.
   * - 29
     - HCF
     - Unimodal composition-style primitive.
   * - 30
     - SchafferF6
     - Multimodal.
   * - 31
     - Step
     - Unimodal stepped function.
   * - 32
     - Quartic
     - Unimodal quartic function.
   * - 33
     - Exponential
     - Unimodal exponential landscape.
   * - 34
     - StyblinskiTang
     - Multimodal.
