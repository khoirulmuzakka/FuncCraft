#ifndef FUNCCRAFT_SUITE_H
#define FUNCCRAFT_SUITE_H

/**
 * @file suite.h
 * @brief Declarative benchmark-suite specification and container API.
 *
 * A suite is now described by a plain-data `SuiteSpec` plus one fixed ambient
 * dimension. The suite container consumes that spec, resolves its sampling
 * rules deterministically, and stores blueprints only. Concrete
 * `BenchmarkFunction` objects are created lazily when the caller requests a
 * specific function index.
 */

#include "benchmark_function.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace FuncCraft {

/**
 * @brief Plain-data specification for a benchmark suite.
 *
 * The suite spec is intended to be serializable to YAML/JSON. It describes:
 * - the admissible dimensions,
 * - the mandatory base-function inventory,
 * - probabilistic transform families for mandatory and composed functions,
 * - the base-function pool for composed functions,
 * - and the suite-wide random seed and domain/optimum settings.
 *
 * String-encoded fields use compact human-readable syntax:
 * - `supported_dimensions` accepts forms like `any`, `10,20,30`, `10-20`, or `>5`.
 * - `ChoiceSpec::kind` names the transform/composition family.
 *
 * If a choice list is empty, the implementation falls back to built-in
 * defaults. Those defaults are balanced enough to produce a usable suite
 * without requiring the caller to specify every distribution.
 */
struct ChoiceSpec {
    /**
     * @brief Family name such as `rotation`, `affine`, `osc`, or `dpm`.
     *
     * The string is normalized by the suite builder, so separators and case
     * differences are ignored.
     */
    std::string kind;
    /**
     * @brief Relative sampling probability for this choice.
     *
     * Probabilities are normalized within each choice list. A zero value keeps
     * the entry available in the spec but makes it unsampled.
     */
    double probability = 1.0;
    /**
     * @brief Optional numeric parameters for the selected family.
     *
     * The interpretation depends on the family. For example, value transform
     * families may use one or two scalar parameters, and composition families
     * may use exponents or sharpness values here.
     */
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

struct SuiteSpec {
    /**
     * @brief Supported suite dimensions.
     *
     * Use a compact range expression such as `any`, `10,20,30`, `10-20`, or
     * `>5`. The builder resolves this into a concrete set of dimensions.
     */
    std::string supported_dimensions = "any";
    /**
     * @brief Mandatory base-function inventory.
     *
     * Empty means "use all currently registered base functions".
     */
    std::vector<int> base_functions = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
    };
    /**
     * @brief Sampling policy for coordinate transforms used by mandatory base functions.
     */
    std::vector<ChoiceSpec> base_function_coord_transforms = {
        make_choice_spec("rotation", 0.3),
        make_choice_spec("affine", 0.7),
    };
    /**
     * @brief Sampling policy for coordinate transforms used by composed functions.
     *
     * If a composed function samples `blockrotation`, the suite will partition
     * the fixed ambient dimension into disjoint blocks and assign one block to
     * each component.
     */
    std::vector<ChoiceSpec> coord_transforms = {
        make_choice_spec("rotation", 0.3),
        make_choice_spec("affine", 0.3),
        make_choice_spec("blockrotation", 0.4),
    };
    /**
     * @brief Sampling policy for value transforms.
     */
    std::vector<ChoiceSpec> value_transforms = {
        make_choice_spec("osc", 0.8),
        make_choice_spec("power", 0.2),
    };
    /**
     * @brief Sampling policy for composition families.
     */
    std::vector<ChoiceSpec> composition_functions = {
        make_choice_spec("cpmlwell", 0.25),
        make_choice_spec("cpmsum", 0.25),
        make_choice_spec("dpmsoftmax", 0.5),
    };
    /**
     * @brief Base functions that may be reused when generating composed functions.
     *
     * Empty means "reuse the mandatory base-function inventory".
     *
     * The default pool is curated to avoid near-duplicate landscapes while
     * still covering the main families, including Gallagher.
     */
    std::vector<int> base_functions_for_compositions = {
        0, 2, 4, 8, 9, 10, 11, 12,
        15, 16, 19, 20, 21, 22, 23,
    };
    /**
     * @brief Requested number of functions to generate.
     *
     * This is an input field. The suite constructor uses it to decide how many
     * blueprints to generate. If it is zero, the suite falls back to the
     * mandatory base-function count.
     */
    int requested_number_of_functions = 0;
    /**
     * @brief Total number of functions generated by the suite.
     *
     * This field is populated by the suite constructor after generation.
     */
    int max_number_of_functions = 0;
    /**
     * @brief Master seed used to derive all other random seeds in the suite.
     */
    unsigned long long master_seed = 1;
    /**
     * @brief Lower bound of the axis-aligned suite domain.
     */
    double lower_bound = -100.0;
    /**
     * @brief Upper bound of the axis-aligned suite domain.
     */
    double upper_bound = 100.0;
    /**
     * @brief Target optimum value used when generating suite members.
     */
    double f_opt = 100.0;
    /**
     * @brief Optional human-readable suite label.
     */
    std::string suite_label = "";
};

/**
 * @brief Deterministic collection of generated benchmark functions.
 */
class BenchmarkSuite final {
public:
    /**
     * @brief Construct a benchmark suite from a declarative suite spec.
     *
     * The constructor does not instantiate benchmark functions. It only
     * normalizes the spec, validates the requested ambient dimension, and
     * prepares the internal blueprints.
     */
    BenchmarkSuite(SuiteSpec spec, int dimension);

    /**
     * @brief Return the number of generated functions.
     */
    int size() const;
    /**
     * @brief Return the maximum number of lazily generatable functions.
     *
     * This is the number of blueprints prepared by the constructor, so it is
     * the upper bound on valid function indices for this suite.
     */
    int max_number_of_functions() const;
    /**
     * @brief Return the theoretical upper bound implied by the suite spec.
     *
     * This is a combinatorial bound computed from the available base functions,
     * transform families, and composition families.
     */
    std::uint64_t theoretical_max_number_of_functions() const;
    /**
     * @brief Return the fixed ambient dimension of this suite.
     */
    int dimension() const;
    /**
     * @brief Build one generated function lazily for the suite dimension.
     */
    BenchmarkFunction function(int index) const;
    /**
     * @brief Batch-evaluate one generated function at multiple points.
     */
    std::vector<double> operator()(int index, const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the normalized suite specification used to build this suite.
     */
    const SuiteSpec& spec() const;

private:
    struct FunctionBlueprint {
        bool composed = false;
        BasicFunctionId base_function = BasicFunctionId::Sphere;
        std::vector<BasicFunctionId> component_bases;
        ChoiceSpec base_transform_choice;
        std::vector<ChoiceSpec> coord_transform_choices;
        std::vector<ChoiceSpec> value_transform_choices;
        ChoiceSpec composition_choice;
        int component_count = 1;
        std::uint64_t seed = 0;
    };

    BenchmarkFunction build_function(const FunctionBlueprint& blueprint) const;
    bool supports_dimension(int dimension) const;

    SuiteSpec spec_;
    int dimension_ = 0;
    std::uint64_t theoretical_max_number_of_functions_ = 0;
    std::vector<FunctionBlueprint> blueprints_;
    std::vector<int> supported_dimensions_;
};

/**
 * @brief Build a suite directly from a declarative suite spec.
 */
BenchmarkSuite make_benchmark_suite(SuiteSpec spec, int dimension);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_H
