Implemented mechanisms
======================

Base functions
--------------

Primitive functions are exposed through ``BasicFunctionId``. The current
registry contains:

.. list-table::
   :header-rows: 1
   :widths: 10 35 55

   * - ID
     - Name
     - Notes
   * - 0
     - Sphere
     - Unimodal, smooth, separable.
   * - 1
     - Ellipsoidal
     - Unimodal, high-conditioned.
   * - 2
     - SumDifferentPowers
     - Unimodal, variable powers.
   * - 3
     - BuecheRastrigin
     - Multimodal Rastrigin-family landscape.
   * - 4
     - LinearSlope
     - Unimodal sloped landscape.
   * - 5
     - AttractiveSector
     - Unimodal, asymmetric sector structure.
   * - 6
     - StepEllipsoidal
     - Unimodal stepped high-conditioned landscape.
   * - 7
     - StepRastrigin
     - Multimodal stepped Rastrigin-family landscape.
   * - 8
     - Rosenbrock
     - Unimodal valley, nonseparable.
   * - 9
     - Ackley
     - Multimodal.
   * - 10
     - Rastrigin
     - Multimodal, separable in primitive coordinates.
   * - 11
     - Griewank
     - Multimodal.
   * - 12
     - Schwefel
     - Multimodal.
   * - 13
     - SharpRidge
     - Unimodal ridge.
   * - 14
     - Weierstrass
     - Multimodal, rugged.
   * - 15
     - SchafferF7
     - Multimodal.
   * - 16
     - SchafferF7Cond1000
     - Multimodal, conditioned Schaffer variant.
   * - 17
     - GriewankRosenbrock
     - Multimodal hybrid.
   * - 18
     - Gallagher21
     - Multimodal peaks.
   * - 19
     - Katsuura
     - Multimodal.
   * - 20
     - LunacekBiRastrigin
     - Multimodal double-funnel Rastrigin-family landscape.
   * - 21
     - Zakharov
     - Unimodal.
   * - 22
     - Levy
     - Multimodal.
   * - 23
     - Michalewicz
     - Multimodal.
   * - 24
     - DixonPrice
     - Unimodal.
   * - 25
     - BentCigar
     - Unimodal, high-conditioned.
   * - 26
     - Discus
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
     - GrieRosen
     - Multimodal Griewank-Rosenbrock variant.
   * - 31
     - SchafferF6
     - Multimodal.
   * - 32
     - Step
     - Unimodal stepped function.
   * - 33
     - Quartic
     - Unimodal quartic function.
   * - 34
     - Exponential
     - Unimodal exponential landscape.
   * - 35
     - StyblinskiTang
     - Multimodal.

Coordinate transforms
---------------------

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``none``
     - Identity transform.
   * - ``rotation``
     - Dense orthogonal rotation with reproducible matrix.
   * - ``affine``
     - Reproducible affine linear transform.
   * - ``block-rotation``
     - Rotation on a selected coordinate subspace.

Value transforms
----------------

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``none``
     - No scalar reshaping.
   * - ``power``
     - Power-law value reshaping.
   * - ``oscillatory``
     - Oscillatory nonlinearity for positive component values.
   * - ``cosine-zero``
     - Nonmonotone transform preserving zero.

Composition functions
---------------------

.. list-table::
   :header-rows: 1

   * - Name
     - Mode
     - Description
   * - ``none``
     - Identity
     - No composition for exactly one component.
   * - ``cpm-wsum``
     - CPM
     - Common-point weighted sum.
   * - ``cpm-power-mean``
     - CPM
     - Common-point power-mean aggregation.
   * - ``cpm-level-well``
     - CPM
     - Common-point level-well composition.
   * - ``dpm-softmax``
     - DPM
     - Deceptive-point softmax composition.
   * - ``dpm-bgsoftmax``
     - DPM
     - Deceptive-point softmax with a smooth background term.

Name parsing
------------

Spec parsers are permissive: case, spaces, hyphens, and underscores are
normalized before matching. For example, ``DPM BG Softmax``,
``dpm-bgsoftmax``, and ``dpmbgsoftmax`` are equivalent inputs. Exported specs
should use the canonical names.
