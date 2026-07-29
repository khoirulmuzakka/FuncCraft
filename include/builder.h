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
#include "value_transforms.h"

#include <functional>
#include <memory>
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
using ComponentEvaluator = std::function<std::vector<double>(const std::vector<std::vector<double>>& X)>;
using ScalarFunction = std::function<double(const std::vector<double>& x)>;
using ComponentScalarEvaluator = std::function<double(const std::vector<double>& x)>;

/**
 * @brief Low-level builder that stores runtime components.
 *
 * Use this class when you want to assemble a callable from already
 * materialized primitive objects. Public specs are owned by higher-level code.
 */
class FunctionBuilder final {
public:
    explicit FunctionBuilder(int dimension);

    /**
     * @brief Set the benchmark domain.
     */
    FunctionBuilder& domain(Domain domain);
    /**
     * @brief Add one primitive component to the composed function.
     */
    FunctionBuilder& add_component(
        BasicFunctionId id,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform);
    /**
     * @brief Add one already-materialized function component.
     */
    FunctionBuilder& add_component(
        ComponentEvaluator evaluator,
        Domain child_domain,
        std::vector<double> child_xopt,
        double child_fopt,
        double component_scale_factor,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform);
    FunctionBuilder& add_component(
        ComponentEvaluator evaluator,
        ComponentScalarEvaluator scalar_evaluator,
        std::shared_ptr<BasicF> primitive,
        Domain child_domain,
        std::vector<double> child_xopt,
        double child_fopt,
        double component_scale_factor,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform);
    /**
     * @brief Set the composition rule for all accumulated components.
     */
    FunctionBuilder& composition(std::shared_ptr<CompositionFunction> composition);
    /**
     * @brief Materialize the final composed runtime callable.
     */
    ComposedFunction build() const;
    /**
     * @brief Materialize a scalar callable for internal composed-component evaluation.
     */
    ScalarFunction build_scalar() const;

private:
    Domain domain_;
    std::shared_ptr<CompositionFunction> composition_;

    struct RuntimeDomainMap {
        std::vector<double> source_lower;
        std::vector<double> source_range;
        std::vector<double> target_lower;
        std::vector<double> target_range;
    };

    struct RuntimeValueTransform {
        ValueTransformClass kind = ValueTransformClass::None;
        double alpha = 1.0;
        double p = 1.0;
        double epsilon = 0.1;
    };

    struct RuntimeComponent {
        ComponentEvaluator evaluate;
        ComponentScalarEvaluator evaluate_scalar;
        std::shared_ptr<BasicF> primitive;
        std::shared_ptr<CoordinateTransform> coordinate_transform;
        RuntimeValueTransform value_transform;
        RuntimeDomainMap domain_map;
        std::vector<double> target_xopt;
        std::vector<double> child_xopt;
        double child_fopt = 0.0;
        double scale_factor = 1.0;
    };

    static RuntimeDomainMap make_runtime_domain_map(const Domain& source_domain, const Domain& target_domain);
    static RuntimeValueTransform make_runtime_value_transform(const ValueTransform& value_transform);
    static void map_component_domain(
        const RuntimeDomainMap& map,
        const std::vector<double>& point,
        std::vector<double>& out);
    static double apply_runtime_value_transform(const RuntimeValueTransform& transform, double u);
    static bool finalize_component_value(const RuntimeComponent& component, double child_value, double& out);
    static bool evaluate_component(
        const RuntimeComponent& component,
        const std::vector<double>& x,
        std::vector<double>& transformed,
        std::vector<double>& child_input,
        double& out);

    std::vector<RuntimeComponent> runtime_components_;
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
 * @brief Create a weighted-sum composition with uniform weights.
 */
std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components);

} // namespace FuncCraft

#endif // FUNCCRAFT_BUILDER_H
