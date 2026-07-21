#ifndef FUNCCRAFT_SUITE_SPEC_H
#define FUNCCRAFT_SUITE_SPEC_H

/**
 * @file suite_spec.h
 * @brief Plain-data specification types for benchmark suite generation.
 *
 * These types are intentionally serializable-friendly and contain only
 * primitive data such as strings, integers, and numeric vectors.
 */

#include <string>
#include <vector>

namespace FuncCraft {

/**
 * @brief Plain-data description of a weighted choice.
 */
struct ChoiceSpec {
    std::string kind;
    double probability = 1.0;
    std::vector<double> parameters;
};

/**
 * @brief Build a choice spec with an optional parameter payload.
 */
inline ChoiceSpec make_choice_spec(
    std::string kind,
    double probability = 1.0,
    std::vector<double> parameters = {}) {
    ChoiceSpec spec;
    spec.kind = std::move(kind);
    spec.probability = probability;
    spec.parameters = std::move(parameters);
    return spec;
}

/**
 * @brief Build a choice spec for families with one scalar parameter.
 */
inline ChoiceSpec make_choice_spec(
    std::string kind,
    double probability,
    double parameter) {
    return make_choice_spec(std::move(kind), probability, std::vector<double>{parameter});
}

/**
 * @brief Build a choice spec for families with two scalar parameters.
 */
inline ChoiceSpec make_choice_spec(
    std::string kind,
    double probability,
    double parameter0,
    double parameter1) {
    return make_choice_spec(std::move(kind), probability, std::vector<double>{parameter0, parameter1});
}

/**
 * @brief Top-level plain-data specification for a generated benchmark suite.
 */
struct SuiteSpec {
    std::string supported_dimensions = "any";
    std::vector<int> base_functions = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 36, 37, 38, 39,
    };
    std::vector<ChoiceSpec> base_function_coord_transforms = {
        make_choice_spec("rotation", 0.3),
        make_choice_spec("affine", 0.7),
    };
    std::vector<ChoiceSpec> coord_transforms = {
        make_choice_spec("rotation", 0.3),
        make_choice_spec("affine", 0.3),
        make_choice_spec("blockrotation", 0.4),
    };
    std::vector<ChoiceSpec> value_transforms = {
        make_choice_spec("osc", 0.8),
        make_choice_spec("power", 0.2),
    };
    std::vector<ChoiceSpec> composition_functions = {
        make_choice_spec("cpmlwell", 0.25),
        make_choice_spec("cpmsum", 0.25),
        ChoiceSpec{"dpmsoftmax", 0.5, {0.01, 1.0, 0.01}},
    };
    std::vector<int> base_functions_for_compositions = {
        0, 2, 4, 8, 9, 10, 11, 12,
        15, 16, 19, 20, 21, 22, 23,
    };
    int max_components = 10;
    int requested_number_of_functions = 0;
    int max_number_of_functions = 0;
    unsigned long long master_seed = 1;
    double lower_bound = -100.0;
    double upper_bound = 100.0;
    double f_opt = 100.0;
    std::string suite_label = "";
};

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_SPEC_H
