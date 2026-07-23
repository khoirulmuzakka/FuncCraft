#ifndef FUNCCRAFT_SUITE_SPEC_H
#define FUNCCRAFT_SUITE_SPEC_H

/**
 * @file suite_spec.h
 * @brief High-level typed specification for benchmark suite generation.
 */

#include "core.h"

#include <cstdint>
#include <utility>
#include <vector>
#include <string>

namespace FuncCraft {

/**
 * @brief Weighted suite-generation choice.
 *
 * Probabilities are fractions and each choice table must sum to one.
 */
template <typename Kind>
struct WeightedChoice {
    Kind kind;
    double probability = 1.0;
    std::vector<double> parameters;
};

using CoordinateTransformChoice = WeightedChoice<CoordinateTransformKind>;
using ValueTransformChoice = WeightedChoice<ValueTransformKind>;
using CompositionChoice = WeightedChoice<CompositionKind>;

template <typename Kind>
inline WeightedChoice<Kind> make_choice(
    Kind kind,
    double probability = 1.0,
    std::vector<double> parameters = {}) {
    WeightedChoice<Kind> choice;
    choice.kind = kind;
    choice.probability = probability;
    choice.parameters = std::move(parameters);
    return choice;
}

template <typename Kind>
inline WeightedChoice<Kind> make_choice(
    Kind kind,
    double probability,
    double parameter) {
    return make_choice(kind, probability, std::vector<double>{parameter});
}

template <typename Kind>
inline WeightedChoice<Kind> make_choice(
    Kind kind,
    double probability,
    double parameter0,
    double parameter1) {
    return make_choice(kind, probability, std::vector<double>{parameter0, parameter1});
}

template <typename Kind>
inline WeightedChoice<Kind> make_choice(
    Kind kind,
    double probability,
    double parameter0,
    double parameter1,
    double parameter2) {
    return make_choice(kind, probability, std::vector<double>{parameter0, parameter1, parameter2});
}

inline std::vector<BasicFunctionId> all_suite_base_functions() {
    return list_basic_functions();
}

inline std::vector<CoordinateTransformChoice> all_coordinate_transform_choices() {
    return {
        make_choice(CoordinateTransformKind::None, 0.0),
        make_choice(CoordinateTransformKind::Rotation, 0.5),
        make_choice(CoordinateTransformKind::Affine, 0.0),
        make_choice(CoordinateTransformKind::BlockRotation, 0.5),
    };
}

inline std::vector<ValueTransformChoice> all_value_transform_choices() {
    return {
        make_choice(ValueTransformKind::None, 0.5),
        make_choice(ValueTransformKind::Power, 0.25, 1.0, 1.0),
        make_choice(ValueTransformKind::Oscillatory, 0.25, 0.1, 1.0),
        make_choice(ValueTransformKind::CosineZero, 0.0, 1.0),
    };
}

inline std::vector<CompositionChoice> all_composition_choices() {
    return {
        make_choice(CompositionKind::CpmWeightedSum, 0.1),
        make_choice(CompositionKind::CpmPowerMean, 0.1, 3.0),
        make_choice(CompositionKind::CpmPowerMean, 0.1, 0.1),
        make_choice(CompositionKind::CpmLevelWell, 0.2, 0.1, 1.0),
        make_choice(CompositionKind::DpmSoftmax, 0.25, 0.01),
        make_choice(CompositionKind::DpmBgSoftmax, 0.25, 0.01, 1.0, 0.01),
    };
}

/**
 * @brief Top-level generation recipe for a benchmark suite.
 *
 * This is deliberately higher level than `FunctionSpec`: it describes pools
 * and probabilities. Generated functions are exported as fully materialized
 * `FunctionSpec` objects.
 */
struct SuiteSpec {
    std::string supported_dimensions = "any";

    std::vector<BasicFunctionId> base_functions = all_suite_base_functions();
    std::vector<BasicFunctionId> composition_base_functions = all_suite_base_functions();

    std::vector<CoordinateTransformChoice> coordinate_transforms = all_coordinate_transform_choices();
    std::vector<ValueTransformChoice> value_transforms = all_value_transform_choices();
    std::vector<CompositionChoice> compositions = all_composition_choices();

    int min_components = 2;
    int max_components = 4;
    int max_nested_composition_depth = 0;
    double nested_probability = 0.0;
    int requested_number_of_functions = 0;
    int max_number_of_functions = 0;
    std::uint64_t master_seed = 1;

    double lower_bound = -100.0;
    double upper_bound = 100.0;
    double assigned_fopt = 100.0;
    double xopt_domain_shrink_factor = 0.8;

    std::string suite_label;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_SPEC_H
