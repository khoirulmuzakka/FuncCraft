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
   Full-dimensional shifted affine transform with matrix :math:`A`. The
   generated matrix starts from a random rotation and scales each output row
   independently by a geometric factor between ``1`` and ``100``:

   .. math::

      T(x) = t + A(x-a).

``subspace-rotation``
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

``huber``
   Parameter is ``[delta]``:

   .. math::

      \phi(u) =
      \begin{cases}
      u^2/(2\delta), & u \le \delta,\\
      u-\delta/2, & u > \delta.
      \end{cases}

``log``
   Parameter is ``[alpha]``:

   .. math::

      \phi(u) = \log(1+\alpha u)/\alpha.

``softplus-threshold``
   Parameters are ``[tau, alpha]``. With
   :math:`\operatorname{sp}(v)=\log(1+\exp(v))`,

   .. math::

      \phi(u) =
      \frac{\operatorname{sp}(\alpha(u-\tau))
      -\operatorname{sp}(-\alpha\tau)}{\alpha}.

``dead-zone``
   Parameters are ``[tau, p]``:

   .. math::

      \phi(u) = \max(0,u-\tau)^p.

``saturating``
   Parameters are ``[cap, c]``:

   .. math::

      \phi(u) = \frac{\mathrm{cap}\,u}{u+c}.

``piecewise-power``
   Parameters are ``[tau, p1, p2]``:

   .. math::

      \phi(u) =
      \begin{cases}
      u^{p_1}, & u \le \tau,\\
      \tau^{p_1} + (u-\tau)^{p_2}, & u > \tau.
      \end{cases}

``noisy-smooth``
   Parameters are ``[epsilon, alpha]``:

   .. math::

      \phi(u) =
      u\left(1+\epsilon\sin(\alpha u)
      \sin(0.371\alpha u+1.2345)\right).

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

``cpm-max``
   Maximum component value:

   .. math::

      \psi(z)=\max_i z_i.

``cpm-smoothmax``
   Parameter is ``[beta]``. With :math:`z_{\max}=\max_i z_i`,

   .. math::

      \psi(z)=z_{\max}+
      \frac{\log\sum_i \exp(\beta(z_i-z_{\max}))-\log m}{\beta}.

``cpm-constraint-penalty``
   Parameters are ``[rho, p]``:

   .. math::

      \psi(z)=z_1+\rho\sum_{i=2}^m z_i^p.

``cpm-lexicographic``
   Parameter is ``[decay]``:

   .. math::

      \psi(z)=\sum_{i=1}^m \mathrm{decay}^{i-1}z_i.

``cpm-product``
   Parameter is ``[alpha]``:

   .. math::

      \psi(z)=\frac{\prod_i(1+\alpha z_i)-1}{\alpha}.

``cpm-max-plus-mean``
   Parameter is ``[lambda]``:

   .. math::

      \psi(z)=\lambda\max_i z_i+
      (1-\lambda)\frac{1}{m}\sum_i z_i.

``cpm-cvar``
   Parameter is ``[quantile]``. Let :math:`z_{(1)}\ge\cdots\ge z_{(m)}`
   and :math:`k=\lceil \mathrm{quantile}\,m\rceil`:

   .. math::

      \psi(z)=\frac{1}{k}\sum_{i=1}^k z_{(i)}.

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
   * - ``subspace-rotation``
     - ``srot``
   * - ``cpm-wsum``
     - ``cpmsum``, ``weighted_sum``
   * - ``cpm-power-mean``
     - ``cpmpmean``, ``power_mean``
   * - ``cpm-level-well``
     - ``cpmlwell``, ``level_well``
   * - ``cpm-max``
     - ``max``, ``worstcase``
   * - ``cpm-smoothmax``
     - ``smoothmax``, ``logsumexp``
   * - ``cpm-constraint-penalty``
     - ``constraintpenalty``, ``penalty``
   * - ``cpm-lexicographic``
     - ``lexicographic``, ``lex``
   * - ``cpm-product``
     - ``product``
   * - ``cpm-max-plus-mean``
     - ``maxplusmean``, ``maxmean``
   * - ``cpm-cvar``
     - ``cvar``, ``worstquantile``
   * - ``dpm-softmax``
     - ``dpmsoftmax``, ``dpm``
   * - ``dpm-bgsoftmax``
     - ``dpmbgsoftmax``, ``DPM BG Softmax``
