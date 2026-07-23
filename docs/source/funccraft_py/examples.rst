Python examples
===============

Create one function
-------------------

.. code-block:: python

   import numpy as np
   import funccraft as fc

   rng = np.random.default_rng(1)
   x_star = rng.uniform(-4.0, 4.0, size=2).tolist()

   spec = fc.make_function_spec(
       dimension=2,
       domain=fc.make_domain(2, -5.0, 5.0),
       components=[
           fc.make_component(
               base_function="Sphere",
               coordinate_transform=fc.make_coordinate_transform(
                   "none",
                   assigned_xopt=x_star,
               ),
           ),
           fc.make_component(
               base_function="Rastrigin",
               coordinate_transform=fc.make_coordinate_transform(
                   "rotation",
                   assigned_xopt=x_star,
                   seed=17,
               ),
               value_transform=fc.make_value_transform(
                   "power",
                   parameters=[1.25, 1.0],
               ),
           ),
       ],
       composition=fc.make_composition("cpm-wsum"),
       assigned_xopt=x_star,
       assigned_fopt=0.0,
   )

   f = fc.BenchmarkFunction(spec)
   before = f.evaluate([x_star, [1.0, 1.0]])
   f.export_spec("function.yaml")

   same = fc.make_benchmark_function("function.yaml")
   after = same.evaluate([x_star, [1.0, 1.0]])
   assert before == after

Create a suite spec directly
----------------------------

.. code-block:: python

   import funccraft as fc

   suite_spec = fc.make_suite_spec(
       requested_number_of_functions=500,
       min_components=2,
       max_components=4,
       max_nested_composition_depth=1,
       nested_probability=0.05,
       master_seed=1,
       compositions=[
           fc.make_composition_choice("dpm-bgsoftmax", 0.5, [0.01, 1.0, 0.01]),
           fc.make_composition_choice("dpm-softmax", 0.5, [0.01]),
       ],
       coordinate_transforms=[
           fc.make_coordinate_transform_choice("rotation", 0.5),
           fc.make_coordinate_transform_choice("block-rotation", 0.5),
       ],
       value_transforms=[
           fc.make_value_transform_choice("none", 0.5),
           fc.make_value_transform_choice("oscillatory", 0.5, [0.1, 1.0]),
       ],
   )

   suite = fc.BenchmarkSuite(suite_spec, 10)

Minimize with SciPy
-------------------

SciPy optimizers usually expect a single-point objective. Wrap FuncCraft's
batched interface:

.. code-block:: python

   import numpy as np
   from scipy.optimize import differential_evolution
   import funccraft as fc

   suite = fc.suite_collection(2026, 1).benchmark_suite(2)
   f = suite.function(0)
   domain = f.domain
   bounds = list(zip(domain.lower_bound, domain.upper_bound))

   def objective(x):
       return f([np.asarray(x, dtype=float).tolist()])[0]

   result = differential_evolution(objective, bounds, seed=1, polish=False)
   print(result.x, result.fun)

Minimize with MinionPy
----------------------

MinionPy supports multiple initial guesses as a list of lists:

.. code-block:: python

   import funccraft as fc
   import minionpy as mpy

   dimension = 2
   suite = fc.suite_collection(2026, 1).benchmark_suite(dimension)
   f = suite.function(0)

   optimizer = mpy.Minimizer(
       func=f.evaluate,
       x0=[
           [0.0] * dimension,
           [1.0] * dimension,
           [-0.5] * dimension,
       ],
       bounds=[(-10, 10)] * dimension,
       algo="ARRDE",
       maxevals=10000,
       callback=None,
       seed=None,
       options=None,
   )
   result = optimizer.optimize()
   print(result.x, result.fun)
