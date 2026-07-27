Python examples
===============

This page is generated from ``funccraft.ipynb`` and mirrors the notebook workflow.

This notebook shows the main FuncCraft workflow with YAML-shaped Python dictionaries:
create one ``BenchmarkFunction``, export and reload it, create one ``BenchmarkSuite``, export and reload it, then use the shipped ``2026_v1`` benchmark suite.

A Python dictionary has the same structure as the YAML files used by FuncCraft, so the examples below can be copied directly into a YAML file later if you want a file-based workflow. The notebook keeps everything inline to avoid external input files.

.. code-block:: python

   import sys
   from pathlib import Path
   
   from matplotlib.backends.backend_pdf import PdfPages
   import matplotlib.pyplot as plt
   import numpy as np
   import textwrap
   
   sys.path.append(str(Path('..').resolve()))
   
   from funccraft import (
       BasicF,
       BasicFunctionId,
       BenchmarkFunction,
       BenchmarkSuite,
       make_benchmark_function,
       suite_collection,
   )
   
   
   def zeros(d):
       return [0.0] * d

Construction framework
----------------------

FuncCraft builds benchmark functions by composing smaller ingredients. The general form is:

.. code-block:: text

   f(x) = assigned_fopt + scale_factor * psi(phi_1(g_1(T_1(x))), ..., phi_m(g_m(T_m(x))))

The main runtime objects are:

- ``BenchmarkFunction``: one concrete callable benchmark function with a fixed dimension, domain, assigned optimum, components, transforms, composition rule, and scale factor.
- ``BenchmarkSuite``: a materialized collection of benchmark functions for one chosen dimension. Use ``suite.function(index)`` to retrieve one ``BenchmarkFunction``. Function indices are one-based.

The main spec records are:

- Function spec: describes one benchmark function.
- Suite spec: describes how many functions to generate and which base functions, coordinate transforms, value transforms, composition rules, and nesting options are allowed.

BenchmarkFunction
-----------------

This section creates one composed ``BenchmarkFunction`` from an inline Python dictionary. The dictionary mirrors a YAML function spec: it defines the dimension, domain, components, coordinate transforms, value transforms, composition rule, assigned optimum, and label. One component is itself a nested composed function.

A ``BenchmarkFunction`` evaluates batches: pass a list of points and receive one value per point.

.. code-block:: python

   dimension = 2
   rng = np.random.default_rng(1)
   assigned_xopt = rng.uniform(-4.0, 4.0, size=dimension).tolist()
   nested_xopt = [1.0, -1.0]
   
   nested_function_spec = {
       'dimension': dimension,
       'domain': {
           'dimension': dimension,
           'lower_bound': [-5.0] * dimension,
           'upper_bound': [5.0] * dimension,
       },
       'components': [
           {
               'base_function': 'Rosenbrock',
               'coordinate_transform': {
                   'kind': 'none',
                   'input_dimension': dimension,
                   'output_dimension': dimension,
                   'assigned_xopt': nested_xopt,
               },
               'value_transform': {'kind': 'none'},
           },
           {
               'base_function': 'Schwefel',
               'coordinate_transform': {
                   'kind': 'rotation',
                   'input_dimension': dimension,
                   'output_dimension': dimension,
                   'assigned_xopt': nested_xopt,
                   'seed': 17,
               },
               'value_transform': {'kind': 'power', 'parameters': [1.1, 1.0]},
           },
       ],
       'composition': {'kind': 'cpm-wsum'},
       'assigned_xopt': nested_xopt,
       'assigned_fopt': 0.0,
       'seed': 11,
       'label': 'nested-rosenbrock-rastrigin',
   }
   
   function_spec = {
       'dimension': dimension,
       'domain': {
           'dimension': dimension,
           'lower_bound': [-10.0] * dimension,
           'upper_bound': [10.0] * dimension,
       },
       'components': [
           {
               'base_function': 'Griewank',
               'coordinate_transform': {
                   'kind': 'none',
                   'input_dimension': dimension,
                   'output_dimension': dimension,
                   'assigned_xopt': assigned_xopt,
               },
               'value_transform': {'kind': 'none'},
           },
           {
               'composed_function': nested_function_spec,
               'coordinate_transform': {
                   'kind': 'rotation',
                   'input_dimension': dimension,
                   'output_dimension': dimension,
                   'assigned_xopt': assigned_xopt,
                   'seed': 31,
               },
               'value_transform': {'kind': 'power', 'parameters': [1.25, 1.0]},
           },
       ],
       'composition': {'kind': 'cpm-wsum'},
       'assigned_xopt': assigned_xopt,
       'assigned_fopt': 0.0,
       'seed': 1,
       'label': 'notebook-nested-composed-function',
   }
   
   f = BenchmarkFunction(function_spec)
   points = [zeros(dimension), assigned_xopt, [1.0] * dimension]
   values = f.evaluate(points)
   
   print('dimension:', f.dimension)
   print('label:', f.spec.label)
   print('component_types:', f.component_types)
   print('xopt:', f.get_xopt())
   print('fopt:', f.get_fopt())
   print('scale_factor:', f.scale_factor)
   print('values:', values)

Plot the composed function as a 3D surface over its two-dimensional domain.

.. code-block:: python

   grid_x = np.linspace(-10.0, 10.0, 120)
   grid_y = np.linspace(-10.0, 10.0, 120)
   xx, yy = np.meshgrid(grid_x, grid_y)
   grid_points = [[float(x), float(y)] for x, y in zip(xx.ravel(), yy.ravel())]
   values = np.asarray(f(grid_points), dtype=float).reshape(xx.shape)
   plot_values = values
   
   fig = plt.figure(figsize=(8, 6))
   ax = fig.add_subplot(111, projection='3d')
   ax.plot_surface(xx, yy, plot_values, cmap='viridis', linewidth=0.0, antialiased=True, alpha=0.95)
   ax.contour(xx, yy, plot_values, zdir='z', offset=float(np.nanpercentile(plot_values, 5)), cmap='viridis', linewidths=0.5)
   ax.scatter([assigned_xopt[0]], [assigned_xopt[1]], [float(np.nanmin(plot_values))], color='crimson', s=40)
   ax.set_title(f.spec.label, fontsize=10)
   ax.set_xlabel('x1')
   ax.set_ylabel('x2')
   ax.set_zlabel('f')
   ax.view_init(elev=18, azim=-135)
   plt.tight_layout()
   plt.show()

Export the materialized function spec to YAML and reload it. The exported file contains generated matrices, resolved scale factor, and other values needed to reproduce this exact function.

.. code-block:: python

   function_yaml_path = Path('manual_function.yaml')
   f.export_spec(str(function_yaml_path))
   reloaded_f = make_benchmark_function(str(function_yaml_path))
   
   check_points = [zeros(dimension), assigned_xopt, [1.0] * dimension]
   print('wrote:', function_yaml_path.resolve())
   print('same label:', reloaded_f.spec.label)
   print('original:', f(check_points))
   print('reloaded:', reloaded_f(check_points))

BenchmarkSuite
--------------

This section creates a custom ``BenchmarkSuite`` from an inline Python dictionary. The dictionary mirrors a YAML suite spec and lists all currently available mechanism families. Set an option's probability to ``0.0`` to keep it visible but disable it.

.. code-block:: python

   suite_base_functions = [
       'Sphere', 'Ellipsoidal', 'SumDifferentPowers', 'BuecheRastrigin', 'LinearSlope', 'AttractiveSector',
       'StepEllipsoidal', 'StepRastrigin', 'Rosenbrock', 'Ackley', 'Rastrigin', 'Griewank', 'Schwefel', 'SharpRidge', 'Weierstrass', 'SchafferF7',
       'SchafferF7Cond1000', 'GriewankRosenbrock', 'Gallagher21', 'Katsuura', 'LunacekBiRastrigin', 'Zakharov', 'Levy', 'Michalewicz', 'DixonPrice',
       'BentCigar', 'HappyCat', 'HGBat', 'HCF', 'SchafferF6', 'Step', 'Quartic', 'Exponential', 'StyblinskiTang',
   ]
   
   suite_spec = {
       'supported_dimensions': '2',
       'base_functions': suite_base_functions,
       'composition_base_functions': suite_base_functions,
       'coordinate_transforms': [
           {'kind': 'none', 'probability': 0.0, 'parameters': []},
           {'kind': 'rotation', 'probability': 0.5, 'parameters': []},
           {'kind': 'affine', 'probability': 0.0, 'parameters': []},
           {'kind': 'blockrotation', 'probability': 0.5, 'parameters': []},
       ],
       'value_transforms': [
           {'kind': 'none', 'probability': 0.5, 'parameters': []},
           {'kind': 'power', 'probability': 0.25, 'parameters': [1.0, 1.0]},
           {'kind': 'osc', 'probability': 0.25, 'parameters': [0.1, 1.0]},
           {'kind': 'cosine-zero', 'probability': 0.0, 'parameters': [1.0]},
       ],
       'compositions': [
           {'kind': 'cpmsum', 'probability': 0.1, 'parameters': []},
           {'kind': 'cpmpmean', 'probability': 0.1, 'parameters': [3.0]},
           {'kind': 'cpmpmean', 'probability': 0.1, 'parameters': [0.1]},
           {'kind': 'cpmlwell', 'probability': 0.2, 'parameters': []},
           {'kind': 'dpmsoftmax', 'probability': 0.25, 'parameters': [0.01]},
           {'kind': 'dpmbgsoftmax', 'probability': 0.25, 'parameters': [0.01, 1.0, 0.01]},
       ],
       'min_components': 2,
       'max_components': 4,
       'max_nested_composition_depth': 1,
       'nested_probability': 0.1,
       'requested_number_of_functions': 200,
       'master_seed': 1,
       'lower_bound': -100.0,
       'upper_bound': 100.0,
       'assigned_fopt': 100.0,
       'xopt_domain_shrink_factor': 0.8,
       'suite_label': 'notebook-suite',
   }
   
   suite = BenchmarkSuite(suite_spec, 2)
   print('size:', suite.size)
   print('dimension:', suite.dimension)
   print('theoretical_max_number_of_functions:', suite.theoretical_max_number_of_functions)
   for idx in range(1, min(5, len(suite)) + 1):
       function = suite.function(idx)
       print(f'F{idx}: {function.component_types} | {function.spec.label}')

Export the materialized suite manifest to YAML and reload it. The manifest contains the normalized suite spec and every generated function spec, so the exact benchmark set can be reused later.

.. code-block:: python

   suite_yaml_path = Path('generated_suite_manifest.yaml')
   suite.export_manifest(str(suite_yaml_path))
   reloaded_suite = BenchmarkSuite(str(suite_yaml_path), suite.dimension)
   print('wrote:', suite_yaml_path.resolve())
   print('reloaded size:', len(reloaded_suite))
   print('F1 label:', reloaded_suite.function(1).spec.label)

Shipped suite: FuncCraft Benchmark Suite 2026 v1
------------------------------------------------

FuncCraft ships the versioned ``2026_v1`` benchmark suite. Load it through ``suite_collection(year=2026, version=1)``, then materialize a ``BenchmarkSuite`` at the dimension you want.

.. code-block:: python

   collection = suite_collection(year=2026, version=1)
   shipped_suite = collection.benchmark_suite(dimension=2)
   
   print(collection)
   print('collection name:', collection.name)
   print('suite size:', shipped_suite.size)
   print('suite dimension:', shipped_suite.dimension)
   
   function = shipped_suite.function(1)
   print('F1 label:', function.spec.label)
   print('F1 component_types:', function.component_types)

Plot functions 1 through 500 from the shipped suite at dimension 2 as 3D surfaces and save the pages to a PDF.

.. code-block:: python

   def wrap_title(idx, label):
       fields = {}
       for part in label.split(';'):
           if '=' in part:
               key, value = part.split('=', 1)
               fields[key.strip()] = value.strip()
       lines = [f'{idx}:']
       cpt = '; '.join(f'{key}={fields[key]}' for key in ('C', 'P', 'T') if key in fields)
       if cpt:
           lines.append(cpt)
       g_value = fields.get('G', '')
       if g_value:
           terms = g_value.split('+')
           if len(terms) <= 1:
               lines.append('G=' + g_value)
           else:
               best_split = 1
               best_score = None
               for split in range(1, len(terms)):
                   left = '+'.join(terms[:split])
                   right = '+'.join(terms[split:])
                   score = max(len(left), len(right))
                   if best_score is None or score < best_score:
                       best_score = score
                       best_split = split
               lines.append('G=' + '+'.join(terms[:best_split]))
               lines.append('+'.join(terms[best_split:]))
       return '\n'.join(lines)
   
   
   selected_indices = list(range(1, 501))
   grid_x = np.linspace(-100.0, 100.0, 50)
   grid_y = np.linspace(-100.0, 100.0, 50)
   xx, yy = np.meshgrid(grid_x, grid_y)
   grid_points = [[float(x), float(y)] for x, y in zip(xx.ravel(), yy.ravel())]
   
   num_columns = 4
   num_rows = 6
   per_page = num_columns * num_rows
   pdf_path = Path('2D_plot_log.pdf')
   
   with PdfPages(pdf_path) as pdf:
       for page_start in range(0, len(selected_indices), per_page):
           page_indices = selected_indices[page_start:page_start + per_page]
           fig = plt.figure(figsize=(16, 24))
           for pos, idx in enumerate(page_indices, start=1):
               function = shipped_suite.function(idx)
               values = np.asarray(function(grid_points), dtype=float).reshape(xx.shape)
               plot_values = np.log10(np.maximum(values, 1.0e-300))
               zmin = float(np.nanpercentile(plot_values, 5))
               ax = fig.add_subplot(num_rows, num_columns, pos, projection='3d')
               ax.plot_surface(xx, yy, plot_values, cmap='viridis', linewidth=0.3, antialiased=True)
               ax.contour(xx, yy, plot_values, zdir='z', offset=zmin, cmap='viridis', linewidths=0.5)
               ax.set_title(wrap_title(idx, function.spec.label), fontsize=8, pad=14)
               ax.set_xlabel('x1', fontsize=7)
               ax.set_ylabel('x2', fontsize=7)
               ax.set_zlabel('log10(f)', fontsize=7)
               ax.tick_params(labelsize=6)
               ax.view_init(elev=8, azim=-135)
           plt.tight_layout()
           pdf.savefig(fig, bbox_inches='tight')
           plt.close(fig)
   
   print(f'Saved {pdf_path}')

Minimize function ``F45`` from the shipped suite at dimension 10 with MinionPy.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   try:
       import minionpy as mpy
   except ImportError:
       print('MinionPy is not installed. Install it with: python -m pip install minionpy')
   else:
       dimension = 10
       function_index = 45
       shipped_suite_10d = collection.benchmark_suite(dimension=dimension)
       f45 = shipped_suite_10d.function(function_index)
       domain = f45.domain
       bounds = list(zip(domain.lower_bound, domain.upper_bound))
   
       optimizer = mpy.Minimizer(
           func=f45.evaluate,
           x0=[ #Minion support multiple initial guesses
               [1.0] * dimension,
               [-1.0] * dimension,
           ],
           bounds=bounds,
           algo='ARRDE',
           maxevals=50_000,
           callback=None,
           seed=1,
           options=None,
       )
       result = optimizer.optimize()
       best_error = abs(result.fun - f45.get_fopt())
   
       print('function:', f'F{function_index}')
       print('label:', f45.spec.label)
       print('dimension:', f45.dimension)
       print('assigned fopt:', f45.get_fopt())
       print('best value:', result.fun)
       print('|best - fopt|:', best_error)
       print('best x:', result.x)

Primitive base functions
------------------------

Use the string names below in a component spec's ``base_function`` field, or
use the matching ``BasicFunctionId.<name>`` enum when constructing ``BasicF``
directly:

.. list-table::
   :header-rows: 1

   * - ID
     - Name
     - Modality
     - Conditioning
     - Properties
   * - 1
     - ``Sphere``
     - Unimodal
     - Well-conditioned
     - Separable, convex.
   * - 2
     - ``Ellipsoidal``
     - Unimodal
     - Ill-conditioned
     - Separable with axis scaling.
   * - 3
     - ``SumDifferentPowers``
     - Unimodal
     - Heterogeneous
     - Separable, different coordinate exponents.
   * - 4
     - ``BuecheRastrigin``
     - Multimodal
     - Moderate
     - Separable, asymmetric, highly periodic.
   * - 5
     - ``LinearSlope``
     - Unimodal
     - Well-conditioned
     - Separable, nonsmooth at optimum.
   * - 6
     - ``AttractiveSector``
     - Unimodal
     - Strongly anisotropic
     - Nonseparable when externally rotated.
   * - 7
     - ``StepEllipsoidal``
     - Unimodal
     - Ill-conditioned
     - Nonsmooth step landscape.
   * - 8
     - ``StepRastrigin``
     - Multimodal
     - Moderate
     - Separable, discontinuous step Rastrigin.
   * - 9
     - ``Rosenbrock``
     - Unimodal
     - Valley-conditioned
     - Nonseparable, narrow curved valley.
   * - 10
     - ``Ackley``
     - Multimodal
     - Moderate
     - Nonseparable, many local minima.
   * - 11
     - ``Rastrigin``
     - Multimodal
     - Moderate
     - Separable, highly periodic.
   * - 12
     - ``Griewank``
     - Multimodal
     - Moderate
     - Nonseparable oscillatory product term.
   * - 13
     - ``Schwefel``
     - Multimodal
     - Deceptive
     - Separable, rugged.
   * - 14
     - ``SharpRidge``
     - Unimodal
     - Ridge-conditioned
     - Nonseparable ridge-shaped valley.
   * - 15
     - ``Weierstrass``
     - Multimodal
     - Rugged
     - Fractal ruggedness; nonseparable when rotated.
   * - 16
     - ``SchafferF7``
     - Multimodal
     - Moderate
     - Nonseparable pairwise radial coupling.
   * - 17
     - ``SchafferF7Cond1000``
     - Multimodal
     - High-conditioned
     - Conditioned Schaffer F7 variant.
   * - 18
     - ``GriewankRosenbrock``
     - Multimodal
     - Valley-conditioned
     - Nonseparable funnel-like composition.
   * - 19
     - ``Gallagher21``
     - Multimodal
     - Peak-conditioned
     - Nonseparable, few dominant peaks.
   * - 20
     - ``Katsuura``
     - Multimodal
     - Rugged
     - Nonseparable, highly rugged.
   * - 21
     - ``LunacekBiRastrigin``
     - Multimodal
     - Deceptive
     - Double-funnel landscape.
   * - 22
     - ``Zakharov``
     - Unimodal
     - Coupled
     - Nonseparable polynomial coupling.
   * - 23
     - ``Levy``
     - Multimodal
     - Moderate
     - Nonseparable periodic structure.
   * - 24
     - ``Michalewicz``
     - Multimodal
     - Sharp minima
     - Nonseparable, many sharp local minima.
   * - 25
     - ``DixonPrice``
     - Unimodal
     - Valley-conditioned
     - Nonseparable curved valley.
   * - 26
     - ``BentCigar``
     - Unimodal
     - Extreme ill-conditioned
     - Separable with extreme axis scaling.
   * - 27
     - ``HappyCat``
     - Multimodal
     - Ridge-like
     - Nonseparable flat ridge structure.
   * - 28
     - ``HGBat``
     - Multimodal
     - Ridge-like
     - Nonseparable flat ridge structure.
   * - 29
     - ``HCF``
     - Unimodal
     - Moderate
     - Separable, exponential growth with L1 norm.
   * - 30
     - ``SchafferF6``
     - Multimodal
     - Moderate
     - Nonseparable pairwise radial coupling.
   * - 31
     - ``Step``
     - Unimodal
     - Piecewise constant
     - Separable step landscape.
   * - 32
     - ``Quartic``
     - Unimodal
     - Polynomial
     - Separable degree-4 polynomial.
   * - 33
     - ``Exponential``
     - Unimodal
     - Smooth
     - Separable smooth basin.
   * - 34
     - ``StyblinskiTang``
     - Multimodal
     - Moderate
     - Separable, many local minima.

Plot the raw primitive base functions directly, without coordinate transforms or value transforms. Each plot uses the primitive's default domain.

.. code-block:: python

   base_function_names = [
     'Sphere', 'Ellipsoidal', 'SumDifferentPowers', 'BuecheRastrigin', 'LinearSlope', 'AttractiveSector',
       'StepEllipsoidal', 'StepRastrigin', 'Rosenbrock', 'Ackley', 'Rastrigin', 'Griewank', 'Schwefel', 'SharpRidge', 'Weierstrass', 'SchafferF7',
       'SchafferF7Cond1000', 'GriewankRosenbrock', 'Gallagher21', 'Katsuura', 'LunacekBiRastrigin', 'Zakharov', 'Levy', 'Michalewicz', 'DixonPrice', 
       'BentCigar', 'HappyCat', 'HGBat', 'HCF', 'SchafferF6', 'Step', 'Quartic', 'Exponential', 'StyblinskiTang',
   ]
   
   def wrap_title(idx, label, width=14):
       wrapped = textwrap.wrap(label, width=width, break_long_words=False, break_on_hyphens=False)
       return f"{idx}:\n" + "\n".join(wrapped)
   
   pdf_path = Path('pure_base_functions_dim2.pdf')
   num_columns = 4
   num_rows = 6
   per_page = num_columns * num_rows
   
   with PdfPages(pdf_path) as pdf:
       for page_start in range(0, len(base_function_names), per_page):
           page_names = base_function_names[page_start:page_start + per_page]
           fig = plt.figure(figsize=(16, 24))
           for local_pos, base_name in enumerate(page_names, start=1):
               global_pos = page_start + local_pos 
               function = BasicF(getattr(BasicFunctionId, base_name), 2)
               default_domain = function.default_domain
               s=1
               sh=0
               grid_x = np.linspace(s*default_domain.lower[0]-sh, s*default_domain.upper[0]+sh, 50)
               grid_y = np.linspace(s*default_domain.lower[0]-sh, s*default_domain.upper[0]+sh, 50)
               xx, yy = np.meshgrid(grid_x, grid_y)
               grid_points = [[float(x), float(y)] for x, y in zip(xx.ravel(), yy.ravel())]
   
               values = np.asarray(function(grid_points), dtype=float).reshape(xx.shape)
               zmin = float(np.nanpercentile(values, 5))
               zmax = float(np.nanpercentile(values, 95))
               ax = fig.add_subplot(num_rows, num_columns, local_pos, projection="3d")
               try :
                   ax.plot_surface(xx, yy, values, cmap="viridis", linewidth=0.3, antialiased=True)
               except : 
                   continue
               ax.contour(xx, yy, values, zdir="z", offset=zmin, cmap="viridis", linewidths=0.5)
               ax.set_title(wrap_title(global_pos, base_name), fontsize=9, pad=12)
               ax.set_xlabel("x1", fontsize=7)
               ax.set_ylabel("x2", fontsize=7)
               ax.set_zlabel("f", fontsize=7)
               ax.set_zlim(zmin, zmax)
               ax.tick_params(labelsize=6)
               ax.view_init(elev=18, azim=-135)
           plt.tight_layout()
           pdf.savefig(fig, bbox_inches="tight")
           plt.show()
           plt.close(fig)
   
   print(f"Saved {pdf_path}")
