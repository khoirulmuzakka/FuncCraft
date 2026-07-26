Editable YAML specs
===================

YAML is the recommended way to configure FuncCraft suites because it is easy
to read, edit, review, and version. A YAML file describes what should be
generated; FuncCraft turns it into runtime benchmark functions for a chosen
dimension.

There are two main spec types:

``SuiteSpec``
   A generator recipe for many functions. This is the usual entry point.

``FunctionSpec``
   A complete recipe for one function. Use this when you want to hand-design
   a single benchmark function.

Suite YAML
----------

Save a file such as ``my_suite.yaml``:

.. code-block:: yaml

   supported_dimensions: any
   base_functions: [1, 9, 10, 11, 12]
   composition_base_functions: [9, 10, 11, 12]

   coordinate_transforms:
     - kind: rotation
       probability: 0.5
     - kind: blockrotation
       probability: 0.5

   value_transforms:
     - kind: none
       probability: 0.5
     - kind: osc
       probability: 0.5

   compositions:
     - kind: cpmsum
       probability: 0.5
     - kind: dpmsoftmax
       probability: 0.5
       parameters: [0.005]

   min_components: 2
   max_components: 4
   max_nested_composition_depth: 1
   nested_probability: 0.1
   requested_number_of_functions: 500
   master_seed: 1
   lower_bound: -100
   upper_bound: 100
   assigned_fopt: 100.0
   xopt_domain_shrink_factor: 0.8
   suite_label: my-suite

Load it in Python:

.. code-block:: python

   import funccraft as fc

   dimension = 10
   spec = fc.load_suite_spec("my_suite.yaml")
   suite = fc.BenchmarkSuite(spec, dimension)

Load it in C++:

.. code-block:: cpp

   #include "funccraft.h"

   int main() {
       const int dimension = 10;
       FuncCraft::SuiteSpec spec =
           FuncCraft::load_suite_spec("my_suite.yaml");
       FuncCraft::BenchmarkSuite suite(spec, dimension);
   }

Suite fields
------------

``supported_dimensions``
   A comma-separated list such as ``"2,5,10,20"`` or ``any``.

``base_functions``
   Primitive functions that are included as mandatory single-function
   benchmarks in the generated suite. Values may be numeric IDs or names.

``composition_base_functions``
   Primitive pool used inside composed functions and nested composed
   components.

``coordinate_transforms``, ``value_transforms``, ``compositions``
   Choice tables. Each entry has ``kind``, ``probability``, and optional
   ``parameters``. Probabilities in each table are fractions and should sum to
   one.

``min_components`` and ``max_components``
   Range for the number of components in composed functions.

``max_nested_composition_depth``
   ``0`` means composed functions use only primitive components. Larger values
   allow components to be composed functions.

``nested_probability``
   Probability that a component becomes a nested composed function when
   nesting is allowed.

``requested_number_of_functions``
   Number of generated functions requested from the suite.

``max_number_of_functions``
   Optional hard cap. ``0`` means no explicit cap beyond the requested count.

``master_seed``
   Seed for the suite generator.

``lower_bound`` and ``upper_bound``
   Search-domain bounds for generated functions.

``assigned_fopt``
   Assigned optimum value for generated functions.

``xopt_domain_shrink_factor``
   Fraction of the domain used for generated optimum locations and DPM
   centers. ``0.8`` means the central 80 percent of the domain.

``suite_label``
   Human-readable label stored with generated specs.

Function YAML
-------------

A function YAML describes one benchmark function directly:

.. code-block:: yaml

   dimension: 2
   domain:
     dimension: 2
     lower_bound: [-5.0, -5.0]
     upper_bound: [5.0, 5.0]
   assigned_xopt: [1.0, -2.0]
   assigned_fopt: 0.0
   scale_factor: 1.0
   seed: 7
   label: example-function
   components:
     - base_function: Sphere
       coordinate_transform:
         kind: none
         input_dimension: 2
         output_dimension: 2
         assigned_xopt: [1.0, -2.0]
       value_transform:
         kind: none
     - base_function: Rastrigin
       coordinate_transform:
         kind: rotation
         input_dimension: 2
         output_dimension: 2
         assigned_xopt: [1.0, -2.0]
         seed: 17
       value_transform:
         kind: power
         parameters: [1.25, 1.0]
   composition:
     kind: cpm-wsum

Load it in Python:

.. code-block:: python

   import funccraft as fc

   spec = fc.load_function_spec("my_function.yaml")
   f = fc.BenchmarkFunction(spec)
   values = f.evaluate([[0.0, 0.0], [1.0, -2.0]])

Load it in C++:

.. code-block:: cpp

   FuncCraft::FunctionSpec spec =
       FuncCraft::load_function_spec("my_function.yaml");
   FuncCraft::BenchmarkFunction f(spec);

Function fields
---------------

``dimension`` and ``domain``
   Ambient/search dimension and bounds.

``components``
   A list of primitive or nested components. A primitive component has
   ``base_function``. A nested component has ``composed_function``.

``coordinate_transform``
   Maps the parent point into the component input. ``input_dimension`` is the
   parent dimension. ``output_dimension`` is the component dimension. For
   block rotation, ``selected_indices`` chooses the subspace.

``value_transform``
   Optional scalar transform of the nonnegative component value.

``composition``
   Combines component values. DPM compositions may also specify full
   dimensional ``centers`` and component ``biases``.

``assigned_xopt`` and ``assigned_fopt``
   Desired optimum location and value for the constructed function.

``scale_factor``
   Final value scale. Set it to a positive number for direct control, or omit
   it to let FuncCraft estimate it.

``seed``, ``label``, ``metadata``
   Optional generation and bookkeeping fields.

Names and aliases
-----------------

Spec parsers normalize names before matching: case, spaces, hyphens, and
underscores are ignored. These inputs are equivalent:

.. code-block:: yaml

   kind: dpm-bgsoftmax
   kind: dpmbgsoftmax
   kind: DPM BG Softmax

For the full list of mechanisms and parameter conventions, see
:doc:`mechanisms`.
