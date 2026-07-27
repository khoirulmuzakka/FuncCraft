Construction model
==================

FuncCraft builds benchmark functions from reusable pieces:

.. math::

   f(x) =
   f_{\mathrm{assigned}}^*
   + \lambda\,
   \psi\left(
       x,\,
       z_1(x),\ldots,z_m(x)
   \right),

where each component value is

.. math::

   z_i(x) = \phi_i\left(g_i(T_i(x)) - g_i(x_i^*)\right).

The pieces are:

``g_i``
   A primitive base function, or a nested composed FuncCraft function.

``T_i``
   A coordinate transform that maps the parent search point into the
   component input coordinates.

``phi_i``
   A value transform applied to the nonnegative shifted component value.

``psi``
   A composition function combining all component values.

``assigned_xopt`` and ``assigned_fopt``
   The constructed optimum location and value exposed by the final benchmark
   function.

``scale_factor``
   The final multiplier :math:`\lambda`. If omitted, FuncCraft estimates it
   from sampled raw values.

Components
----------

A component is either primitive or nested:

- A primitive component has ``base_function``.
- A nested component has ``composed_function``.

Each component owns one coordinate transform and one value transform. The
coordinate transform has an ``input_dimension`` equal to the parent/search
dimension and an ``output_dimension`` equal to the component input dimension.
Block rotation can therefore make a component live on a selected subspace.

Nested composed components must have ``assigned_fopt = 0``. Component values
are expected to be zero at their assigned optimum; the final nonzero optimum
value belongs to the outer ``BenchmarkFunction``.

Coordinate transforms and optimum placement
-------------------------------------------

The coordinate transform is responsible for placing the component optimum in
the generated search space. The public spec stores ``assigned_xopt``. The
target optimum of the primitive or nested child is determined internally by
FuncCraft, including domain scaling for primitive base functions.

For full-dimensional transforms, ``assigned_xopt`` has length equal to the
ambient dimension. For block rotation, ``assigned_xopt`` has length equal to
the selected subspace size.

CPM and DPM compositions
------------------------

FuncCraft uses two broad composition modes:

``CPM``
   Common-point composition. Components share the same assigned optimum.
   Implemented CPM modes include weighted sum, power mean, and level well.

``DPM``
   Deceptive-point composition. Component 0 is assigned to the constructed
   global optimum; other components can create local-minimum traps around
   separate centers. DPM centers and biases live on ``CompositionSpec``.

Suite generation
----------------

A ``SuiteSpec`` does not describe one function directly. It describes pools
and probabilities. The suite generator samples from those choices, creates
``FunctionSpec`` objects, and then materializes runtime ``BenchmarkFunction``
objects for a requested dimension.

The standard suite collection is also a YAML ``SuiteSpec``. Use
``suite_collection(year=2026, version=1)`` when you want the packaged suite,
or ``load_suite_spec("my_suite.yaml")`` when you want to edit the YAML
yourself.

.. _design-constraints:

Design constraints
------------------

FuncCraft is implemented around two practical constraints: scalable behavior
under dimension variation and robust numerical behavior across supported
platforms.

Scalable behavior under dimension variation means that a suite can be
materialized at different dimensions without resampling the identity of each
function. For a fixed suite, function index, and seed, the generator keeps the
structural choices stable: primitive-vs-nested component choices, base
functions, composition kind, coordinate-transform kind, value-transform kind,
component seeds, and transform seeds. Generated coordinate data such as
``assigned_xopt`` and DPM centers are prefix-stable, so a higher-dimensional
point extends the coordinates already used at lower dimension. Block-rotation
subspaces are also generated from coordinate-indexed seeds; when all
components use block rotation, the generated subspaces cover the active
dimension.

This does not mean that a low-dimensional function is literally the leading
principal block of the higher-dimensional function. Full rotations and affine
transforms use an adjacent-plane sweep. The same seed gives a related sequence
of plane rotations as the dimension grows, but adding a new adjacent plane can
change coordinates that already existed. The intended guarantee is stable
function identity and related geometry, not identical embedded function
values.

Cross-platform robustness means that generated specifications and sampled
values should not depend strongly on differences between standard-library
implementations. FuncCraft therefore uses deterministic integer-based random
streams, fixed lookup tables for generated rotation pairs and affine row
scales, and quantized generated matrix entries. Runtime numeric stabilization
is intentionally narrow: after each component value transform, the resulting
component value ``z_i(x)`` is passed through ``stable_numeric_value`` before it
enters ``psi``. Primitive base functions and composition formulas otherwise use
ordinary ``double`` arithmetic and the platform math library.

``stable_numeric_value`` uses relative binary mantissa rounding. It writes a
finite nonzero value as ``mantissa * 2^exponent`` with ``frexp``, rounds the
mantissa to a fixed ``2^-40`` grid, and reconstructs the value with ``ldexp``.
Because the rounding is relative to the value's magnitude, large and small
component values keep about the same effective precision.

Exported specs include materialized matrices and generated centers, so loading
an exported function avoids regenerating those parameters. Some primitives are
intentionally nonsmooth or highly oscillatory, so exact bitwise equality across
platforms is not a design target; the CI value comparison checks agreement
statistically rather than requiring every sampled point to match.

Final scaling
-------------

If ``scale_factor`` is omitted, FuncCraft estimates it from the raw unscaled
function. It samples 128 deterministic centered stratified points in the
active domain, computes raw values, applies the same relative numeric
quantization used for component stabilization, takes the 25th percentile
:math:`q`, and uses

.. math::

   \lambda =
   \begin{cases}
   1, & \text{if } q \text{ is non-finite or } q \le 10^{-12},\\
   \min(10^5/q,\;10^8), & \text{otherwise.}
   \end{cases}

This keeps typical values comparable while preserving the assigned optimum
value. The estimator is deterministic for a fixed materialized function spec:
the sample points are generated from the function seed without random jitter,
and the raw values are computed before the final scale and bias are applied.

For the exact formulas of each transform and composition mode, see
:doc:`mechanisms`.
