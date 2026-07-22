#ifndef FUNCCRAFT_BUILDER_H
#define FUNCCRAFT_BUILDER_H

/**
 * @file builder.h
 * @brief Low-level helpers for assembling benchmark functions.
 *
 * This header exposes the runtime ingredients used to build benchmark
 * functions as well as the low-level callable type returned by the builder.
 */

#include "basicf.h"
#include "composition.h"
#include "coordinate_transforms.h"
#include "core.h"
#include "function_spec.h"
#include "value_transforms.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace FuncCraft {

/**
 * @brief Runtime callable for a composed benchmark function.
 *
 * The callable consumes a batch of input vectors and returns one scalar value
 * per input vector. It intentionally carries no metadata or helper methods.
 */
using ComposedFunction = std::function<std::vector<double>(const std::vector<std::vector<double>>& X)>;

/**
 * @brief Low-level builder that stores components and materializes specs.
 *
 * Use this class when you want to assemble a function from primitive
 * ingredients directly. It produces either a plain-data spec or a runtime
 * callable, depending on the entry point you use.
 */
class FunctionBuilder final {
public:
    explicit FunctionBuilder(int dimension);

    /**
     * @brief Set the benchmark domain.
     */
    FunctionBuilder& domain(Domain domain);
    /**
     * @brief Record the seed used to generate this function.
     */
    FunctionBuilder& seed(unsigned long long seed);
    /**
     * @brief Set the assigned global minimizer in search coordinates.
     */
    FunctionBuilder& assigned_xopt(std::vector<double> x_opt);
    /**
     * @brief Set the assigned global optimum value.
     */
    FunctionBuilder& assigned_fopt(double f_opt);
    /**
     * @brief Set the requested scale factor. Leave unset for internal estimation.
     */
    FunctionBuilder& scale_factor(std::optional<double> scale_factor);
    /**
     * @brief Add one primitive component to the composed function.
     */
    FunctionBuilder& add_component(
        BasicFunctionId id,
        int component_dimension,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform,
        double f_bias = 0.0);
    /**
     * @brief Set the composition rule for all accumulated components.
     */
    FunctionBuilder& composition(std::shared_ptr<CompositionFunction> composition);
    /**
     * @brief Materialize the final composed runtime callable.
     */
    ComposedFunction build() const;
    /**
     * @brief Materialize the plain-data specification accumulated so far.
     *
     * The returned spec includes the public construction request captured by
     * the builder.
     */
    FunctionSpec build_spec() const;
    /**
     * @brief Return the plain-data function specification accumulated so far.
     */
    FunctionSpec spec() const;

private:
    Domain domain_;
    unsigned long long seed_ = 0;
    std::vector<double> assigned_xopt_;
    double assigned_fopt_ = 0.0;
    std::optional<double> scale_factor_;
    std::vector<ComponentSpec> component_specs_;
    std::shared_ptr<CompositionFunction> composition_;
    FunctionClass function_class_;
};

/**
 * @brief Parse a basic-function name into a runtime identifier.
 */
BasicFunctionId parse_basic_function_id(const std::string& name);
/**
 * @brief Create a runtime basic function from a plain name and dimension.
 *
 * The returned object owns the selected primitive function instance.
 */
std::shared_ptr<BasicF> make_basic_function(const std::string& name, int dimension);
/**
 * @brief Build a domain object from a plain function specification.
 *
 * Missing bounds fall back to the default `[-100, 100]` interval per axis.
 */
Domain make_domain(const FunctionSpec& spec);
/**
 * @brief Create a runtime coordinate transform from a plain transform spec.
 *
 * The spec must provide the full ambient dimension, assigned xopt, and any
 * family-specific indices or parameters. `target_xopt` is computed internally
 * during benchmark materialization.
 */
std::shared_ptr<CoordinateTransform> make_coordinate_transform(
    const CoordinateTransformSpec& spec,
    const std::vector<double>& target_xopt);
/**
 * @brief Create a runtime value transform from a plain value-transform spec.
 *
 * The spec selects the family and supplies its scalar parameters.
 */
std::shared_ptr<ValueTransform> make_value_transform(const ValueTransformSpec& spec);
/**
 * @brief Create a runtime composition rule from a plain composition spec.
 *
 * The component count is used to validate or fill in family defaults.
 */
std::shared_ptr<CompositionFunction> make_composition(const CompositionSpec& spec, std::size_t component_count);
/**
 * @brief Create a weighted-sum composition with uniform weights.
 */
std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components);

} // namespace FuncCraft

#endif // FUNCCRAFT_BUILDER_H
