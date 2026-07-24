Construction model
==================

FuncCraft constructs benchmark functions from reusable pieces. The conceptual
form is

.. math::

   f(x) = \psi\left(
       \phi_1(g_1(T_1(x))),
       \ldots,
       \phi_m(g_m(T_m(x)))
   \right).

Here:

- :math:`g_i` is a primitive benchmark function.
- :math:`T_i` is a coordinate transform for component ``i``.
- :math:`\phi_i` is a scalar value transform.
- :math:`\psi` is the composition function.

The core public data structure for one function is ``FunctionSpec``. It stores
the ambient dimension, domain, component list, composition rule, assigned
global minimizer, assigned optimum value, optional scale factor, seed, label,
and metadata.

Coordinate-transform convention
-------------------------------

Every coordinate transform maps from a parent/search point to the input of one
component. Let the parent dimension be :math:`D`, the component dimension be
:math:`d_i`, the assigned component optimum be :math:`a_i`, and the internally
resolved primitive/nested target optimum be :math:`t_i`. ``assigned_xopt`` in
``CoordinateTransformSpec`` stores :math:`a_i`.

``none``
   Full-dimensional shift with no rotation. Here :math:`d_i = D` and

   .. math::

      T_i(x) = t_i + (x - a_i).

``rotation``
   Full-dimensional shifted rotation. Here :math:`d_i = D`; :math:`R_i` is an
   orthogonal matrix generated from the transform seed or read from an
   exported spec:

   .. math::

      T_i(x) = t_i + R_i (x - a_i).

``affine``
   Full-dimensional shifted affine transform. Here :math:`d_i = D`; :math:`A_i`
   is a generated or exported matrix:

   .. math::

      T_i(x) = t_i + A_i (x - a_i).

``block-rotation``
   Subspace transform. Let ``selected_indices`` define a projection
   :math:`P_i : \mathbb{R}^D \to \mathbb{R}^{d_i}`. The transform first slices
   the parent point and then rotates only that selected subspace:

   .. math::

      x_{\mathrm{sub}} = P_i x,\qquad
      T_i(x) = t_i + R_i (x_{\mathrm{sub}} - a_i).

   For block rotation, ``assigned_xopt``, ``target_xopt`` and the rotation
   matrix are all subspace-sized. The component function therefore sees only
   :math:`d_i = |\mathrm{selected\_indices}|` variables.

After the coordinate transform, FuncCraft rescales the transformed point from
the transform-output domain into the child function's native domain. That is
how primitive optima remain correct even when the generated benchmark domain
differs from a base function's default domain.

Value transforms
----------------

Let

.. math::

   u_i(x) = g_i(T_i(x)) - f_i^*

be the nonnegative shifted component value before the value transform.
FuncCraft implements:

``none``
   .. math::

      \phi_i(u) = u.

``power``
   Parameters are ``[alpha, p]`` with :math:`\alpha > 0` and :math:`p > 0`:

   .. math::

      \phi_i(u) = \alpha u^p.

``oscillatory``
   Parameters are ``[epsilon, alpha]`` with
   :math:`0 \le \epsilon < 1` and :math:`\alpha \ge 0`:

   .. math::

      \phi_i(u) = u\left(1 + \epsilon\sin(\alpha u)\right).

``cosine-zero``
   Parameter is ``[alpha]`` with :math:`\alpha > 0`:

   .. math::

      \phi_i(u) = 1 - \cos(\alpha u).

The trigonometric transforms reduce the phase modulo :math:`2\pi` internally
for better cross-platform numerical consistency.

Composition modes
-----------------

Let :math:`z_i = \phi_i(u_i(x))` be the transformed nonnegative component
values.

``none``
   Valid only for exactly one component:

   .. math::

      \psi(z_1) = z_1.

``cpm-wsum``
   Common-point weighted sum. Current generated weights are all one:

   .. math::

      \psi(z) = \sum_{i=1}^m w_i z_i.

``cpm-power-mean``
   Parameter is ``[p]`` with :math:`p > 0`; current generated weights are all
   one:

   .. math::

      \psi(z) = \left(\sum_{i=1}^m w_i z_i^p\right)^{1/p}.

``cpm-level-well``
   Parameters are ``[epsilon, alpha]``. Define
   :math:`s = \sum_i w_i z_i`; current generated weights are all one:

   .. math::

      \psi(z) = s\left(1 + \epsilon\sin(\alpha s)\right).

``dpm-softmax``
   Deceptive-point softmax. Let :math:`c_i` be full-dimensional DPM centers,
   :math:`b_i` be DPM biases with :math:`b_0 = 0`, and :math:`\gamma` be the
   sharpness parameter. Define

   .. math::

      \ell_i(x) = -\gamma\|x-c_i\|^2,\qquad
      q_i(x) = \exp(\ell_i(x)-\max_j \ell_j(x)).

   Non-global centers are masked near the global center:

   .. math::

      m_0(x)=1,\qquad
      m_i(x)=1-\exp(-\|x-c_0\|^2),\quad i>0.

   The composition is

   .. math::

      \psi(x,z) =
      \frac{\sum_i q_i(x)m_i(x)(z_i+b_i)}
           {\sum_i q_i(x)m_i(x)}.

``dpm-bgsoftmax``
   This uses the same softmax weights and mask, plus a smooth background term.
   With background strength :math:`\rho` and background sharpness :math:`\eta`,

   .. math::

      \beta(x) = \rho\left(1-\exp(-\eta \min_i\|x-c_i\|)\right),

   and

   .. math::

      \psi(x,z) =
      \frac{\sum_i (q_i(x)m_i(x)+\beta(x))(z_i+b_i)}
           {\sum_i (q_i(x)m_i(x)+\beta(x))}.

For DPM compositions, center 0 is the assigned global optimum. Other centers
are deceptive points. DPM biases live in ``CompositionSpec.biases`` because
they are part of the composition, not the component value transform.

Final scaling and optimum assignment
------------------------------------

The raw composed value is scaled and shifted at the final ``BenchmarkFunction``
level:

.. math::

   f(x) = f_{\mathrm{assigned}}^* + \lambda\,\psi(x,z(x)).

``assigned_xopt`` controls the constructed minimizer location in the generated
coordinates. ``assigned_fopt`` controls :math:`f_{\mathrm{assigned}}^*`.
``scale_factor`` controls :math:`\lambda`; when omitted, FuncCraft estimates
it internally. User-provided ``scale_factor`` must be finite and positive.

When ``scale_factor`` is not provided, FuncCraft estimates :math:`\lambda`
from the raw unscaled function. It samples 100 points uniformly in the active
function domain using a deterministic seed derived from the function seed,
evaluates the raw composed function at those points, removes non-finite
values, and takes the 25th percentile value :math:`q`. The estimate is

.. math::

   \lambda =
   \begin{cases}
   1, & \text{if } q \text{ is non-finite or } q \le 10^{-12},\\
   \min(10^5/q,\;10^8), & \text{otherwise.}
   \end{cases}

This makes typical raw values comparable across generated functions while
keeping the assigned optimum value fixed at ``assigned_fopt``.

This convention lets users control where the constructed function has its
known minimizer without needing to know the primitive function's default
domain.

Components
----------

A component is either:

- a primitive basic function, selected by ``BasicFunctionId`` or name; or
- a nested composed function, stored through ``composed_function``.

Each component owns its coordinate transform and value transform. The parent
input dimension is ``coordinate_transform.input_dimension``; the component
function works on ``coordinate_transform.output_dimension``.

By design, every component is composed from a zero-baseline function. Primitive
components use the primitive function's own optimum value internally and are
shifted to zero before the value transform. Nested composed components must
therefore have ``assigned_fopt = 0``. Only the outer/top-level
``BenchmarkFunction`` should use a nonzero ``assigned_fopt`` to set the final
reported optimum value.

Complete ``FunctionSpec`` example
---------------------------------

The following Python example builds a parent DPM function with one primitive
component and one nested composed component. ``make_composition(...)`` sets
the composition rule of a ``FunctionSpec``; nesting is expressed by passing a
child ``FunctionSpec`` to ``make_component(composed_function=...)``.

.. code-block:: python

   import funccraft as fc

   child_spec = fc.make_function_spec(
       dimension=1,
       domain=fc.make_domain(1, lower_bound=-10.0, upper_bound=10.0),
       assigned_xopt=[-4.0],
       assigned_fopt=0.0,
       scale_factor=1.0,
       seed=321,
       label="nested-child",
       components=[
           fc.make_component(
               base_function="Ackley",
               coordinate_transform=fc.make_coordinate_transform(
                   kind="none",
                   input_dimension=1,
                   output_dimension=1,
                   assigned_xopt=[-4.0],
               ),
               value_transform=fc.make_value_transform("none"),
               seed=3001,
           ),
       ],
       composition=fc.make_composition("none"),
   )

   function_spec = fc.make_function_spec(
       dimension=2,
       domain=fc.make_domain(2, lower_bound=-10.0, upper_bound=10.0),
       assigned_xopt=[2.0, -3.0],
       assigned_fopt=100.0,
       scale_factor=1.0,
       seed=123,
       label="manual-dpm-example",
       metadata=["source=docs"],
       components=[
           fc.make_component(
               base_function="Rastrigin",
               coordinate_transform=fc.make_coordinate_transform(
                   kind="rotation",
                   input_dimension=2,
                   output_dimension=2,
                   assigned_xopt=[2.0, -3.0],
                   seed=1001,
               ),
               value_transform=fc.make_value_transform("none"),
               seed=2001,
           ),
           fc.make_component(
               composed_function=child_spec,
               coordinate_transform=fc.make_coordinate_transform(
                   kind="block-rotation",
                   input_dimension=2,
                   output_dimension=1,
                   selected_indices=[0],
                   assigned_xopt=[-4.0],
                   seed=1002,
               ),
               value_transform=fc.make_value_transform(
                   "power",
                   parameters=[1.0, 1.0],
               ),
               seed=2002,
           ),
       ],
       composition=fc.make_composition(
           "dpm-bgsoftmax",
           parameters=[0.01, 1.0, 0.01],
           biases=[0.0, 20.0],
           centers=[[2.0, -3.0], [-4.0, 5.0]],
       ),
   )

   f = fc.BenchmarkFunction(function_spec)
   print(f.component_types)
   print(f([[2.0, -3.0], [0.0, 0.0]]))
   f.export_spec("manual_function.yaml")

C++ uses the same fields directly through plain structs. The nested component
stores the child spec in ``composed_function``:

.. code-block:: cpp

   #include "funccraft.h"

   #include <memory>
   #include <vector>

   int main() {
       using namespace FuncCraft;

       ComponentSpec child_ackley;
       child_ackley.base_function = BasicFunctionId::Ackley;
       child_ackley.seed = 3001;
       child_ackley.coordinate_transform.kind = CoordinateTransformKind::None;
       child_ackley.coordinate_transform.input_dimension = 1;
       child_ackley.coordinate_transform.output_dimension = 1;
       child_ackley.coordinate_transform.assigned_xopt = {-4.0};
       child_ackley.value_transform.kind = ValueTransformKind::None;

       FunctionSpec child;
       child.dimension = 1;
       child.domain.dimension = 1;
       child.domain.lower_bound = {-10.0};
       child.domain.upper_bound = {10.0};
       child.assigned_xopt = {-4.0};
       child.assigned_fopt = 0.0;
       child.scale_factor = 1.0;
       child.seed = 321;
       child.label = "nested-child";
       child.components = {child_ackley};
       child.composition.kind = CompositionKind::None;

       ComponentSpec rastrigin;
       rastrigin.base_function = BasicFunctionId::Rastrigin;
       rastrigin.seed = 2001;
       rastrigin.coordinate_transform.kind = CoordinateTransformKind::Rotation;
       rastrigin.coordinate_transform.input_dimension = 2;
       rastrigin.coordinate_transform.output_dimension = 2;
       rastrigin.coordinate_transform.assigned_xopt = {2.0, -3.0};
       rastrigin.coordinate_transform.seed = 1001;
       rastrigin.value_transform.kind = ValueTransformKind::None;

       ComponentSpec nested;
       nested.composed_function = std::make_shared<FunctionSpec>(child);
       nested.seed = 2002;
       nested.coordinate_transform.kind = CoordinateTransformKind::BlockRotation;
       nested.coordinate_transform.input_dimension = 2;
       nested.coordinate_transform.output_dimension = 1;
       nested.coordinate_transform.selected_indices = {0};
       nested.coordinate_transform.assigned_xopt = {-4.0};
       nested.coordinate_transform.seed = 1002;
       nested.value_transform.kind = ValueTransformKind::Power;
       nested.value_transform.parameters = {1.0, 1.0};

       FunctionSpec spec;
       spec.dimension = 2;
       spec.domain.dimension = 2;
       spec.domain.lower_bound = {-10.0, -10.0};
       spec.domain.upper_bound = {10.0, 10.0};
       spec.assigned_xopt = {2.0, -3.0};
       spec.assigned_fopt = 100.0;
       spec.scale_factor = 1.0;
       spec.seed = 123;
       spec.label = "manual-dpm-example";
       spec.metadata = {"source=docs"};
       spec.components = {rastrigin, nested};
       spec.composition.kind = CompositionKind::DpmBgSoftmax;
       spec.composition.parameters = {0.01, 1.0, 0.01};
       spec.composition.biases = {0.0, 20.0};
       spec.composition.centers = {{2.0, -3.0}, {-4.0, 5.0}};

       BenchmarkFunction f(spec);
       std::vector<double> values = f({{2.0, -3.0}, {0.0, 0.0}});
       f.export_spec("manual_function.yaml");
   }

Important fields:

- ``dimension`` is the ambient/search dimension.
- ``domain`` is the generated search box.
- ``components`` lists either primitive ``base_function`` components or nested
  ``composed_function`` specs.
- ``coordinate_transform.input_dimension`` is the parent dimension.
- ``coordinate_transform.output_dimension`` is the dimension seen by the
  component after transformation.
- ``composition.centers`` are full-dimensional DPM centers. They are optional
  in user-authored specs; exported specs contain materialized centers.
- ``assigned_xopt`` and ``assigned_fopt`` define the controlled global optimum.
  For a nested ``FunctionSpec`` used as a component, ``assigned_fopt`` must be
  zero.
- ``scale_factor=None`` asks FuncCraft to estimate the final value scale.

Complete ``SuiteSpec`` example
------------------------------

``SuiteSpec`` is a generator recipe. It does not store every materialized
matrix or center; those appear only after building and exporting a
``BenchmarkSuite`` manifest.

.. code-block:: python

   import funccraft as fc

   base_pool = (
       "Sphere, Ellipsoidal, Rosenbrock, Ackley, Rastrigin, Griewank, "
       "Schwefel, HappyCat, HGBat, BentCigar, Discus"
   )
   base_pool = [name.strip() for name in base_pool.split(",")]

   suite_spec = fc.make_suite_spec(
       supported_dimensions="2,5,10",
       base_functions=base_pool,
       composition_base_functions=base_pool,
       requested_number_of_functions=500,
       max_number_of_functions=0,
       min_components=2,
       max_components=4,
       max_nested_composition_depth=1,
       nested_probability=0.05,
       master_seed=2026,
       lower_bound=-100.0,
       upper_bound=100.0,
       assigned_fopt=100.0,
       xopt_domain_shrink_factor=0.8,
       suite_label="docs-example",
       coordinate_transforms=[
           fc.make_coordinate_transform_choice("none", 0.0),
           fc.make_coordinate_transform_choice("rotation", 0.5),
           fc.make_coordinate_transform_choice("affine", 0.0),
           fc.make_coordinate_transform_choice("block-rotation", 0.5),
       ],
       value_transforms=[
           fc.make_value_transform_choice("none", 0.5),
           fc.make_value_transform_choice("power", 0.25, [1.0, 1.0]),
           fc.make_value_transform_choice("oscillatory", 0.25, [0.1, 1.0]),
           fc.make_value_transform_choice("cosine-zero", 0.0, [1.0]),
       ],
       compositions=[
           fc.make_composition_choice("cpm-wsum", 0.1),
           fc.make_composition_choice("cpm-power-mean", 0.1, [3.0]),
           fc.make_composition_choice("cpm-power-mean", 0.1, [0.1]),
           fc.make_composition_choice("cpm-level-well", 0.2, [0.1, 1.0]),
           fc.make_composition_choice("dpm-softmax", 0.25, [0.01]),
           fc.make_composition_choice("dpm-bgsoftmax", 0.25, [0.01, 1.0, 0.01]),
       ],
   )

   suite = fc.BenchmarkSuite(suite_spec, dimension=10)
   f = suite.function(0)
   print(f.spec.label, f.component_types)
   suite.export_manifest("suite_manifest.yaml")

C++ suite specs use the same ``SuiteSpec`` struct and ``make_choice`` helpers:

.. code-block:: cpp

   #include "funccraft.h"

   #include <vector>

   int main() {
       using namespace FuncCraft;

       std::vector<BasicFunctionId> base_pool = {
           BasicFunctionId::Sphere,
           BasicFunctionId::Ellipsoidal,
           BasicFunctionId::Rosenbrock,
           BasicFunctionId::Ackley,
           BasicFunctionId::Rastrigin,
           BasicFunctionId::Griewank,
           BasicFunctionId::Schwefel,
           BasicFunctionId::HappyCat,
           BasicFunctionId::HGBat,
           BasicFunctionId::BentCigar,
           BasicFunctionId::Discus,
       };

       SuiteSpec spec;
       spec.supported_dimensions = "2,5,10";
       spec.base_functions = base_pool;
       spec.composition_base_functions = base_pool;
       spec.requested_number_of_functions = 500;
       spec.max_number_of_functions = 0;
       spec.min_components = 2;
       spec.max_components = 4;
       spec.max_nested_composition_depth = 1;
       spec.nested_probability = 0.05;
       spec.master_seed = 2026;
       spec.lower_bound = -100.0;
       spec.upper_bound = 100.0;
       spec.assigned_fopt = 100.0;
       spec.xopt_domain_shrink_factor = 0.8;
       spec.suite_label = "docs-example";

       spec.coordinate_transforms = {
           make_choice(CoordinateTransformKind::None, 0.0),
           make_choice(CoordinateTransformKind::Rotation, 0.5),
           make_choice(CoordinateTransformKind::Affine, 0.0),
           make_choice(CoordinateTransformKind::BlockRotation, 0.5),
       };
       spec.value_transforms = {
           make_choice(ValueTransformKind::None, 0.5),
           make_choice(ValueTransformKind::Power, 0.25, 1.0, 1.0),
           make_choice(ValueTransformKind::Oscillatory, 0.25, 0.1, 1.0),
           make_choice(ValueTransformKind::CosineZero, 0.0, 1.0),
       };
       spec.compositions = {
           make_choice(CompositionKind::CpmWeightedSum, 0.1),
           make_choice(CompositionKind::CpmPowerMean, 0.1, 3.0),
           make_choice(CompositionKind::CpmPowerMean, 0.1, 0.1),
           make_choice(CompositionKind::CpmLevelWell, 0.2, 0.1, 1.0),
           make_choice(CompositionKind::DpmSoftmax, 0.25, 0.01),
           make_choice(CompositionKind::DpmBgSoftmax, 0.25, 0.01, 1.0, 0.01),
       };

       BenchmarkSuite suite(spec, 10);
       const BenchmarkFunction& f = suite.function(0);
       suite.export_manifest("suite_manifest.yaml");
   }

Important fields:

- ``base_functions`` are mandatory primitive functions included first in the
  generated suite.
- ``composition_base_functions`` are the primitive pool used inside composed
  functions.
- Choice probabilities are fractions and each choice table must sum to one.
- ``max_nested_composition_depth=0`` means composed suite functions use only
  primitive components. Larger values allow composed functions as components.
- ``nested_probability`` controls how often each component becomes nested.
- ``xopt_domain_shrink_factor`` restricts generated optima and DPM centers to
  the central fraction of the domain.
- For generated suite functions, block-rotation subspaces are sampled by the
  suite generator. If all components in a composed function use block rotation,
  the sampled subspaces cover the full dimension. In DPM mode, the global
  component's block rotation is full-dimensional when needed.

All runtime evaluations are batched. A function evaluates
``vector<vector<double>>`` in C++ or a list of lists in Python.
