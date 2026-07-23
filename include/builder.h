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

private:
    Domain domain_;
    std::shared_ptr<CompositionFunction> composition_;

    struct RuntimeComponent {
        ComponentEvaluator evaluate;
        std::shared_ptr<CoordinateTransform> coordinate_transform;
        std::shared_ptr<ValueTransform> value_transform;
        Domain child_domain;
        std::vector<double> target_xopt;
        std::vector<double> child_xopt;
        double child_fopt = 0.0;
    };
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
