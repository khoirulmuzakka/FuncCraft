#ifndef FUNCCRAFT_FUNCTION_SPEC_H
#define FUNCCRAFT_FUNCTION_SPEC_H

/**
 * @file function_spec.h
 * @brief Plain-data specification types for benchmark generation.
 *
 * These types are intentionally serializable-friendly and contain only
 * primitive data such as strings, integers, and numeric vectors.
 */

#include <string>
#include <vector>

namespace FuncCraft {

/**
 * @brief Serializable description of a coordinate transform.
 *
 * This structure is intentionally plain data. It is suitable for JSON/YAML
 * export and should not depend on runtime transform classes.
 *
 * Expected conventions:
 * - `kind` identifies the transform family, such as `rotation`, `affine`, or `block_rotation`.
 * - `dimension` is the full ambient dimension of the input and output vectors.
 * - `seed` controls deterministic transform generation.
 * - `selected_indices` is used by block/subspace transforms and is interpreted as 0-based indices.
 * - `source_point` and `target_point` are full-length vectors with size `dimension`.
 * - `parameters` stores extra scalar parameters for transform families that need them.
 * - `matrix` stores the concrete linear transform used at runtime when the
 *   transform family has one.
 */
struct TransformSpec {
    std::string kind;
    int dimension = 0;
    int seed = 0;
    std::vector<int> selected_indices;
    std::vector<double> source_point;
    std::vector<double> target_point;
    std::vector<double> parameters;
    std::vector<std::vector<double>> matrix;
};

/**
 * @brief Serializable description of a 1D value transform.
 *
 * The transform is applied to the scalar output of a base function before
 * composition. The `kind` field selects the family, while `parameters`
 * stores any family-specific scalars in order.
 */
struct ValueTransformSpec {
    std::string kind;
    int seed = 0;
    std::vector<double> parameters;
};

/**
 * @brief Serializable description of one benchmark component.
 *
 * A component combines one base function with one coordinate transform and
 * one value transform. The `component_dimension` records the dimension of the
 * transformed point passed to the base function.
 */
struct ComponentSpec {
    std::string base_function;
    int component_dimension = 0;
    TransformSpec coordinate_transform;
    ValueTransformSpec value_transform;
    int seed = 0;
};

/**
 * @brief Serializable description of the composition rule.
 *
 * The `kind` field selects the composition family. The remaining fields are
 * family-specific payloads:
 * - `parameters` for scalar parameters such as exponents or sharpness values;
 * - `centers` and `offsets` for deceptive multi-point compositions;
 * - `weights` for weighted aggregation families.
 *
 * The composition spec is still plain data, so it can be loaded from or saved
 * to text formats without constructing runtime objects.
 */
struct CompositionSpec {
    std::string kind;
    int seed = 0;
    std::vector<double> parameters;
    std::vector<std::vector<double>> centers;
    std::vector<double> offsets;
    std::vector<double> weights;
};

/**
 * @brief Top-level plain-data specification for a generated benchmark function.
 *
 * This is the format consumed by `BenchmarkFunction`. It fully describes the
 * function using only serializable primitives and nested spec records.
 * Builder helpers convert this spec into runtime objects, while the spec
 * itself remains safe to serialize to YAML or JSON.
 *
 * Relevant descriptive fields that used to live in a separate metadata record
 * are stored directly here:
 * - `function_class_label` identifies the composed function family. It is
 *   optional on input and is typically populated by the builder after
 *   normalization.
 * - `known_global_minimizer` stores the known optimizer when available.
 * - `known_global_value` stores the corresponding function value.
 * - `parameters` stores extra string tags such as `key=value` pairs.
 */
struct FunctionSpec {
    int dimension = 0;
    std::vector<double> lower_bound;
    std::vector<double> upper_bound;
    std::vector<ComponentSpec> component_specs;
    CompositionSpec composition_spec;
    int seed = 0;
    /**
     * @brief Human-readable class label for the composed function.
     *
     * This is a plain string, suitable for serialization and printing. A
     * builder may leave it empty on input and fill it in after construction.
     */
    std::string function_class_label;
    std::vector<double> known_global_minimizer;
    double known_global_value = 0.0;
    std::vector<std::string> parameters;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_FUNCTION_SPEC_H
