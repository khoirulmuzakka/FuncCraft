Implemented mechanisms
======================

This page lists the implemented coordinate transforms, value transforms, and
composition modes. The primitive base-function ID table is in
:doc:`primitive_base_functions`. YAML parsers normalize case, spaces,
hyphens, and underscores before matching names.

Base functions
--------------

Primitive functions are exposed through ``BasicFunctionId``. Numeric IDs and
names can both be used in YAML. See :doc:`primitive_base_functions` for the
complete table.

Coordinate transforms
---------------------

Let :math:`x` be the parent point, :math:`a` be the assigned component optimum
stored in ``assigned_xopt``, and :math:`t` be the internally resolved child
optimum. The transform maps parent coordinates into child coordinates.

``none``
   Full-dimensional shift:

   .. math::

      T(x) = t + (x-a).

``rotation``
   Full-dimensional shifted rotation with orthogonal matrix :math:`R`:

   .. math::

      T(x) = t + R(x-a).

``affine``
   Full-dimensional shifted affine transform with matrix :math:`A`:

   .. math::

      T(x) = t + A(x-a).

``block-rotation``
   Subspace transform. If ``selected_indices`` defines projection :math:`P`,
   then :math:`x_{\mathrm{sub}} = Px` and

   .. math::

      T(x) = t + R(x_{\mathrm{sub}} - a).

   The child function sees only ``output_dimension`` variables.

Value transforms
----------------

Let :math:`u \ge 0` be the shifted component value before the value transform.

``none``
   .. math::

      \phi(u) = u.

``power``
   Parameters are ``[alpha, p]``:

   .. math::

      \phi(u) = \alpha u^p.

``oscillatory``
   Parameters are ``[epsilon, alpha]``:

   .. math::

      \phi(u) = u\left(1 + \epsilon\sin(\alpha u)\right).

``cosine-zero``
   Parameter is ``[alpha]``:

   .. math::

      \phi(u) = 1 - \cos(\alpha u).

Trigonometric value transforms reduce the phase modulo :math:`2\pi` internally
for more stable cross-platform numerical behavior.

Composition modes
-----------------

Let :math:`z_i` be transformed component values.

``none``
   Single-component identity:

   .. math::

      \psi(z_1) = z_1.

``cpm-wsum``
   Common-point weighted sum:

   .. math::

      \psi(z) = \sum_i w_i z_i.

``cpm-power-mean``
   Parameter is ``[p]``:

   .. math::

      \psi(z) = \left(\sum_i w_i z_i^p\right)^{1/p}.

``cpm-level-well``
   Parameters are ``[epsilon, alpha]``. Let
   :math:`s = \sum_i w_i z_i`:

   .. math::

      \psi(z) = s\left(1 + \epsilon\sin(\alpha s)\right).

``dpm-softmax``
   Parameter is ``[sharpness]``. Let :math:`c_i` be full-dimensional DPM
   centers, :math:`b_i` be DPM biases, and :math:`\gamma` be sharpness:

   .. math::

      q_i(x) = \exp(-\gamma\|x-c_i\|^2 - M),
      \qquad
      M = \max_j -\gamma\|x-c_j\|^2.

   Non-global centers are masked near the global center:

   .. math::

      m_0(x)=1,\qquad
      m_i(x)=1-\exp(-\|x-c_0\|^2),\quad i>0.

   Then

   .. math::

      \psi(x,z) =
      \frac{\sum_i q_i(x)m_i(x)(z_i+b_i)}
           {\sum_i q_i(x)m_i(x)}.

``dpm-bgsoftmax``
   Parameters are ``[sharpness, background_strength, background_sharpness]``.
   It adds a smooth background term

   .. math::

      \beta(x) =
      \rho\left(1-\exp(-\eta \min_i\|x-c_i\|)\right)

   and computes

   .. math::

      \psi(x,z) =
      \frac{\sum_i (q_i(x)m_i(x)+\beta(x))(z_i+b_i)}
           {\sum_i (q_i(x)m_i(x)+\beta(x))}.

DPM center 0 is the assigned global optimum. Other centers are deceptive
locations. DPM biases are composition parameters, not component value
transforms.

Name aliases
------------

Examples of accepted aliases:

.. list-table::
   :header-rows: 1

   * - Canonical name
     - Common aliases
   * - ``none``
     - ``identity``
   * - ``rotation``
     - ``rot``
   * - ``affine``
     - ``aff``
   * - ``block-rotation``
     - ``blockrotation``, ``blockrot``, ``brot``
   * - ``cpm-wsum``
     - ``cpmsum``, ``weighted_sum``
   * - ``cpm-power-mean``
     - ``cpmpmean``, ``power_mean``
   * - ``cpm-level-well``
     - ``cpmlwell``, ``level_well``
   * - ``dpm-softmax``
     - ``dpmsoftmax``, ``dpm``
   * - ``dpm-bgsoftmax``
     - ``dpmbgsoftmax``, ``DPM BG Softmax``
