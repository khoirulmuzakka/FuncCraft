#ifndef FUNCCRAFT_FUNCTION_SPEC_H
#define FUNCCRAFT_FUNCTION_SPEC_H

/**
 * @file function_spec.h
 * @brief Specification types for FuncCraft benchmark functions.
 *
 * Specs describe both what the user wants to build and what was actually
 * materialized at runtime:
 * - which primitive components are used;
 * - where component minima are assigned in the generated/search coordinates;
 * - which coordinate/value transforms and composition family are requested;
 * - which optimum location/value should be controlled by construction.
 * - concrete transform matrices when known.
 */

#include "core.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace FuncCraft {

struct FunctionSpec;

/**
 * @brief Canonical public names used when serializing specs.
 *
 * Parsers should be permissive by lowercasing and removing spaces, hyphens,
 * and underscores before matching. Exporters should write these canonical
 * names.
 */
namespace spec_name {
inline constexpr const char* None = "none";

inline constexpr const char* CoordinateRotation = "rotation";
inline constexpr const char* CoordinateAffine = "affine";
inline constexpr const char* CoordinateSubspaceRotation = "subspace-rotation";

inline constexpr const char* ValuePower = "power";
inline constexpr const char* ValueOscillatory = "oscillatory";
inline constexpr const char* ValueCosineZero = "cosine-zero";
inline constexpr const char* ValueHuber = "huber";
inline constexpr const char* ValueLog = "log";
inline constexpr const char* ValueSoftplusThreshold = "softplus-threshold";
inline constexpr const char* ValueDeadZone = "dead-zone";
inline constexpr const char* ValueSaturating = "saturating";
inline constexpr const char* ValuePiecewisePower = "piecewise-power";
inline constexpr const char* ValueNoisySmooth = "noisy-smooth";

inline constexpr const char* CpmWeightedSum = "cpm-wsum";
inline constexpr const char* CpmPowerMean = "cpm-power-mean";
inline constexpr const char* CpmLevelWell = "cpm-level-well";
inline constexpr const char* CpmMax = "cpm-max";
inline constexpr const char* CpmSmoothMax = "cpm-smoothmax";
inline constexpr const char* CpmConstraintPenalty = "cpm-constraint-penalty";
inline constexpr const char* CpmLexicographic = "cpm-lexicographic";
inline constexpr const char* CpmProduct = "cpm-product";
inline constexpr const char* CpmMaxPlusMean = "cpm-max-plus-mean";
inline constexpr const char* CpmCvar = "cpm-cvar";
inline constexpr const char* SparseActive = "sparse-active";
inline constexpr const char* DpmSoftmax = "dpm-softmax";
inline constexpr const char* DpmBgSoftmax = "dpm-bgsoftmax";
} // namespace spec_name

/**
 * @brief Axis-aligned search domain requested by the user.
 */
struct DomainSpec {
    int dimension = 0;
    std::vector<double> lower_bound;
    std::vector<double> upper_bound;
};

/**
 * @brief Coordinate-transform request/materialization for one component.
 *
 * `input_dimension` is the parent/search dimension. `output_dimension` is the
 * dimension seen by the component function after the transform. `assigned_xopt`
 * is output-dimensional: for full transforms this is the full generated/search
 * coordinate, while for subspace rotation it is the selected subspace coordinate.
 * The corresponding transform target is computed internally from the selected
 * base function and benchmark domain.
 *
 * `selected_indices` is only meaningful for subspace rotation. If it is empty,
 * suite generation may choose the subspace. `matrix` is empty until the
 * concrete linear transform is generated or loaded.
 */
struct CoordinateTransformSpec {
    CoordinateTransformKind kind = CoordinateTransformKind::None;
    int input_dimension = 0;
    int output_dimension = 0;
    std::vector<double> assigned_xopt;
    std::vector<int> selected_indices;
    std::vector<double> parameters;
    std::vector<std::vector<double>> matrix;
    std::uint64_t seed = 0;
};

/**
 * @brief User-facing value-transform request for one component.
 *
 * Parameter conventions:
 * - `Power`: `parameters[0] = alpha`, `parameters[1] = p`.
 * - `Oscillatory`: `parameters[0] = epsilon`, `parameters[1] = alpha`.
 * - `CosineZero`: `parameters[0] = alpha`.
 * - `Huber`: `parameters[0] = delta`.
 * - `Log`: `parameters[0] = alpha`.
 * - `SoftplusThreshold`: `parameters[0] = tau`, `parameters[1] = alpha`.
 * - `DeadZone`: `parameters[0] = tau`, `parameters[1] = p`.
 * - `Saturating`: `parameters[0] = cap`, `parameters[1] = c`.
 * - `PiecewisePower`: `parameters[0] = tau`, `parameters[1] = p1`,
 *   `parameters[2] = p2`.
 * - `NoisySmooth`: `parameters[0] = epsilon`, `parameters[1] = alpha`.
 */
struct ValueTransformSpec {
    ValueTransformKind kind = ValueTransformKind::None;
    std::vector<double> parameters;
};

/**
 * @brief User-facing component request.
 *
 * A component uses `composed_function` when it is set; otherwise it uses
 * `base_function` as a primitive component. The component input dimension is
 * inferred from the coordinate transform output dimension. `scale_factor =
 * std::nullopt` means the builder should estimate a component multiplier from
 * transformed component values before composition.
 */
struct ComponentSpec {
    std::optional<BasicFunctionId> base_function;
    std::shared_ptr<FunctionSpec> composed_function;
    CoordinateTransformSpec coordinate_transform;
    ValueTransformSpec value_transform;
    std::optional<double> scale_factor = std::nullopt;
    std::uint64_t seed = 0;
};

/**
 * @brief User-facing composition request.
 *
 * Supported families:
 * - `None`: no composition for exactly one component.
 * - `CpmWeightedSum`: common-point weighted-sum composition.
 * - `CpmPowerMean`: common-point weighted power mean.
 * - `CpmLevelWell`: common-point level-well composition.
 * - `CpmMax`: common-point worst-component maximum.
 * - `CpmSmoothMax`: common-point smooth maximum.
 * - `CpmConstraintPenalty`: first component plus penalties from the rest.
 * - `CpmLexicographic`: priority-weighted component sum.
 * - `CpmProduct`: multiplicative component aggregation.
 * - `CpmMaxPlusMean`: blend of worst component and average component.
 * - `CpmCvar`: average of the worst quantile of components.
 * - `SparseActive`: point-dependent active component selection.
 * - `DpmSoftmax`: deceptive-point softmax composition.
 * - `DpmBgSoftmax`: deceptive-point softmax with a smooth background term.
 *
 * `weights` are intentionally not part of this public spec. Weight policy is
 * owned by the composition implementation. If user-configurable weights become
 * necessary, add an explicit high-level weight policy instead of exposing the
 * runtime vector directly.
 *
 * Parameter conventions:
 * - `CpmPowerMean`: `parameters[0] = p`.
 * - `CpmLevelWell`: `parameters[0] = epsilon`, `parameters[1] = alpha`.
 * - `CpmSmoothMax`: `parameters[0] = beta`.
 * - `CpmConstraintPenalty`: `parameters[0] = rho`, `parameters[1] = p`.
 * - `CpmLexicographic`: `parameters[0] = decay`.
 * - `CpmProduct`: `parameters[0] = alpha`.
 * - `CpmMaxPlusMean`: `parameters[0] = lambda`.
 * - `CpmCvar`: `parameters[0] = quantile`.
 * - `SparseActive`: `parameters[0] = frequency`.
 * - `DpmSoftmax`: `parameters[0] = sharpness`.
 * - `DpmBgSoftmax`: `parameters[0] = sharpness`,
 *   `parameters[1] = background_strength`,
 *   `parameters[2] = background_sharpness`.
 *
 * `biases` are used only by DPM families for local-minimum traps. Empty means
 * all DPM trap biases are zero. Non-DPM families do not accept biases.
 *
 * DPM families use `centers` as full-dimensional softmax centers. Empty means
 * the builder should resolve centers internally. This is separate from
 * subspace-rotation component `assigned_xopt`, which may be subspace-dimensional.
 */
struct CompositionSpec {
    CompositionKind kind = CompositionKind::None;
    std::vector<double> parameters;
    std::vector<double> biases;
    std::vector<std::vector<double>> centers;
};

/**
 * @brief Public high-level specification for one benchmark function.
 *
 * This is the object a user should write by hand, construct from Python, or
 * provide in a concise YAML file. User-authored specs may omit materialized
 * details such as generated matrices, DPM centers, and scale factors; exported
 * materialized specs include them.
 *
 * `assigned_xopt` and `assigned_fopt` control the constructed optimum location
 * and value. Top-level `scale_factor = std::nullopt` means the builder should
 * determine a final post-composition scale factor internally.
 */
struct FunctionSpec {
    int dimension = 0;
    DomainSpec domain;
    std::vector<ComponentSpec> components;
    CompositionSpec composition;
    std::vector<double> assigned_xopt;
    double assigned_fopt = 0.0;
    std::optional<double> scale_factor = std::nullopt;
    std::uint64_t seed = 0;
    std::string label;
    std::vector<std::string> metadata;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_FUNCTION_SPEC_H
