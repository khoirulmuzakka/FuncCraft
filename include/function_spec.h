#ifndef FUNCCRAFT_FUNCTION_SPEC_H
#define FUNCCRAFT_FUNCTION_SPEC_H

/**
 * @file function_spec.h
 * @brief Specification types for FuncCraft benchmark functions.
 *
 * Specs describe both what the user wants to build and what was actually
 * materialized for reproducibility:
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
inline constexpr const char* CoordinateBlockRotation = "block-rotation";

inline constexpr const char* ValuePower = "power";
inline constexpr const char* ValueOscillatory = "oscillatory";
inline constexpr const char* ValueCosineZero = "cosine-zero";

inline constexpr const char* CpmWeightedSum = "cpm-wsum";
inline constexpr const char* CpmPowerMean = "cpm-power-mean";
inline constexpr const char* CpmLevelWell = "cpm-level-well";
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
 * `assigned_xopt` is the desired component optimizer in the generated/search
 * coordinates. The corresponding transform target is computed internally from
 * the selected base function and benchmark domain.
 *
 * `selected_indices` is only meaningful for block rotation. If it is empty,
 * suite generation may choose the subspace. `matrix` is empty until the
 * concrete linear transform is generated or loaded.
 */
struct CoordinateTransformSpec {
    CoordinateTransformKind kind = CoordinateTransformKind::None;
    int dimension = 0;
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
 * inferred from the coordinate transform output dimension.
 */
struct ComponentSpec {
    std::optional<BasicFunctionId> base_function;
    std::shared_ptr<FunctionSpec> composed_function;
    CoordinateTransformSpec coordinate_transform;
    ValueTransformSpec value_transform;
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
 * - `DpmSoftmax`: `parameters[0] = sharpness`.
 * - `DpmBgSoftmax`: `parameters[0] = sharpness`,
 *   `parameters[1] = background_strength`,
 *   `parameters[2] = background_sharpness`.
 *
 * `biases` are used only by DPM families for local-minimum traps. Empty means
 * all DPM trap biases are zero. Non-DPM families do not accept biases.
 *
 * DPM families use each component coordinate transform's `assigned_xopt` as
 * the component center.
 */
struct CompositionSpec {
    CompositionKind kind = CompositionKind::None;
    std::vector<double> parameters;
    std::vector<double> biases;
};

/**
 * @brief Public high-level specification for one benchmark function.
 *
 * This is the object a user should write by hand, construct from Python, or
 * provide in a concise YAML file. It intentionally omits runtime-only details
 * such as generated matrices and primitive target points.
 *
 * `assigned_xopt` and `assigned_fopt` control the constructed optimum location
 * and value. `scale_factor = std::nullopt` means the builder should determine
 * a scale factor internally.
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
