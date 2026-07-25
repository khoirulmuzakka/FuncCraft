Python examples
===============

All Python evaluations are batched: pass ``list[list[float]]`` and receive one
value per point.

Load a suite YAML
-----------------

.. code-block:: python

   import funccraft as fc

   dimension = 10
   function_index = 0

   spec = fc.load_suite_spec("my_suite.yaml")
   suite = fc.BenchmarkSuite(spec, dimension)

   points = [[0.0] * dimension, [1.0] * dimension]
   f = suite.function(function_index)
   values = f.evaluate(points)

   print(f.spec.label)
   print(f.component_types)
   print(values)

Use a packaged suite collection
-------------------------------

.. code-block:: python

   import funccraft as fc

   dimension = 10
   suite = fc.suite_collection(2026, 1).benchmark_suite(dimension)
   function_index = 0
   values = suite.evaluate(function_index, [[0.0] * dimension])

Create one function in Python
-----------------------------

For frequent editing, a YAML ``FunctionSpec`` is usually easier. The helper
functions are useful for scripts and notebooks:

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
                   input_dimension=2,
                   output_dimension=2,
                   assigned_xopt=x_star,
               ),
           ),
           fc.make_component(
               base_function="Rastrigin",
               coordinate_transform=fc.make_coordinate_transform(
                   "rotation",
                   input_dimension=2,
                   output_dimension=2,
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
   print(f.evaluate([x_star, [1.0, 1.0]]))

Load one function YAML
----------------------

.. code-block:: python

   import funccraft as fc

   spec = fc.load_function_spec("my_function.yaml")
   f = fc.BenchmarkFunction(spec)
   values = f.evaluate([[0.0, 0.0]])

Minimize with SciPy
-------------------

SciPy optimizers usually expect a single-point objective. Wrap FuncCraft's
batched interface:

.. code-block:: python

   import numpy as np
   from scipy.optimize import differential_evolution
   import funccraft as fc

   dimension = 10
   function_index = 0
   suite = fc.suite_collection(2026, 1).benchmark_suite(dimension)
   f = suite.function(function_index)
   domain = f.domain
   bounds = list(zip(domain.lower_bound, domain.upper_bound))

   def objective(x):
       point = np.asarray(x, dtype=float).tolist()
       return f.evaluate([point])[0]

   result = differential_evolution(objective, bounds, seed=1, polish=False)
   print(result.x, result.fun)

Minimize with MinionPy
----------------------

MinionPy accepts batched objective functions directly and supports multiple
initial guesses as a list of lists:

.. code-block:: python

   import funccraft as fc
   import minionpy as mpy

   dimension = 10
   function_index = 0
   suite = fc.suite_collection(2026, 1).benchmark_suite(dimension)
   f = suite.function(function_index)
   domain = f.domain

   optimizer = mpy.Minimizer(
       func=f.evaluate,
       x0=[
           [0.0] * dimension,
           [1.0] * dimension,
           [-0.5] * dimension,
       ],
       bounds=list(zip(domain.lower_bound, domain.upper_bound)),
       algo="ARRDE",
       maxevals=10000,
       callback=None,
       seed=None,
       options=None,
   )
   result = optimizer.optimize()
   print(result.x, result.fun)

Export materialized YAML
------------------------

.. code-block:: python

   f.export_spec("function_materialized.yaml")
   suite.export_manifest("suite_manifest.yaml")
