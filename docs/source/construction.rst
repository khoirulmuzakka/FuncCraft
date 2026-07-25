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
``suite_collection(2026, 1)`` when you want the packaged suite, or
``load_suite_spec("my_suite.yaml")`` when you want to edit the YAML yourself.

Final scaling
-------------

If ``scale_factor`` is omitted, FuncCraft estimates it from the raw unscaled
function. It samples 100 deterministic points in the active domain, computes
raw values, takes the 25th percentile :math:`q`, and uses

.. math::

   \lambda =
   \begin{cases}
   1, & \text{if } q \text{ is non-finite or } q \le 10^{-12},\\
   \min(10^5/q,\;10^8), & \text{otherwise.}
   \end{cases}

This keeps typical values comparable while preserving the assigned optimum
value.

For the exact formulas of each transform and composition mode, see
:doc:`mechanisms`.
