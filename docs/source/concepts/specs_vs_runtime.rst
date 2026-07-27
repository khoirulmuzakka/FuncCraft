Specs versus runtime objects
============================

FuncCraft uses specs for description and runtime objects for evaluation.

``SuiteSpec``
   A generator recipe for many functions. It describes pools, probabilities,
   dimensions, seeds, bounds, and requested suite size.

``FunctionSpec``
   A complete recipe for one function. It describes components, transforms,
   composition, assigned optimum, scale, seed, and metadata.

``BenchmarkSuite``
   A runtime suite created from a ``SuiteSpec`` and a chosen dimension. It
   materializes many ``BenchmarkFunction`` objects.

``BenchmarkFunction``
   A runtime object that evaluates one concrete benchmark function in batches.

Specs are the right layer for editing YAML, reviewing benchmark definitions,
or constructing functions programmatically. Runtime objects are the right layer
for evaluating points, running optimizers, inspecting optima, and exporting
materialized records.

Editable specs and materialized specs
-------------------------------------

An editable suite YAML usually leaves many values to be generated later:
component choices, transform seeds, rotations, centers, and other generated
details. Loading the suite and choosing a dimension produces concrete
``BenchmarkFunction`` objects.

An exported materialized function spec records the generated details for one
already-built function. Loading it later reconstructs that function directly.
An exported suite manifest records the normalized suite plus all generated
function specs.

See :doc:`../user_guide/yaml_specs` for YAML fields and
:doc:`../user_guide/exporting` for materialized output.
