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

For implemented linear transforms, FuncCraft uses

.. math::

   T(x) = x_{\mathrm{base}}^* + M (x - x_{\mathrm{assigned}}^*).

``assigned_xopt`` is the desired optimum location in the generated/search
coordinates. The base-function target :math:`x_{\mathrm{base}}^*` is resolved
inside ``BenchmarkFunction`` from the selected primitive function and the
active domain scaling. Users do not provide the target point.

This convention lets users control where the constructed function has its
known minimizer without needing to know the primitive function's default
domain.

Components
----------

A component is either:

- a primitive basic function, selected by ``BasicFunctionId`` or name; or
- a nested composed function, stored through ``composed_function``.

Each component owns its coordinate transform and value transform. Component
input dimension is inferred from the coordinate transform.

Composition modes
-----------------

FuncCraft separates composition into three public modes:

- ``none``: identity composition for a single component;
- CPM: common-point compositions, where components share the same assigned
  optimum;
- DPM: deceptive-point compositions, where components have separate assigned
  centers and non-global components can create traps through DPM biases.

DPM biases are part of ``CompositionSpec`` because they define the deceptive
composition landscape. They are not value-transform or component-level biases.

Scaling and optimum assignment
------------------------------

``assigned_xopt`` controls the constructed minimizer location in the generated
coordinates. ``assigned_fopt`` controls the function value at that location.
``scale_factor`` controls the multiplicative value scale. When
``scale_factor`` is omitted, FuncCraft estimates it internally.

All runtime evaluations are batched. A function evaluates
``vector<vector<double>>`` in C++ or a list of lists in Python.
