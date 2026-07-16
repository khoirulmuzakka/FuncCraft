#ifndef FUNCCRAFT_BUILDER_H
#define FUNCCRAFT_BUILDER_H

/**
 * @file builder.h
 * @brief Fluent builder for constructing composed benchmark functions.
 *
 * Use this API to assemble a benchmark from primitive base functions,
 * coordinate transforms, value transforms, and a composition rule.
 */

#include "composition.h"

#include <map>
#include <memory>
#include <string>

namespace FuncCraft {

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
     * @brief Set the known global minimizer.
     */
    FunctionBuilder& known_global_minimizer(std::vector<double> x_star);
    /**
     * @brief Set the known global value.
     */
    FunctionBuilder& known_global_value(double f_star);
    /**
     * @brief Add one primitive component to the composed function.
     */
    FunctionBuilder& add_component(
        BasicFunctionId id,
        int component_dimension,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform);
    /**
     * @brief Set the composition rule for all accumulated components.
     */
    FunctionBuilder& composition(std::shared_ptr<CompositionFunction> composition);
    /**
     * @brief Store an arbitrary metadata parameter.
     */
    FunctionBuilder& parameter(std::string key, std::string value);

    /**
     * @brief Materialize the final composed function.
     */
    ComposedFunction build() const;

private:
    Domain domain_;
    unsigned long long seed_ = 0;
    std::vector<double> x_star_;
    double f_star_ = 0.0;
    std::vector<Component> components_;
    std::shared_ptr<CompositionFunction> composition_;
    FunctionClass function_class_;
    std::map<std::string, std::string> parameters_;
};

/**
 * @brief Create a value transform instance from its enum class.
 */
std::shared_ptr<ValueTransform> make_value_transform(ValueTransformClass cls, double a = 1.0, double b = 1.0);
/**
 * @brief Create a weighted-sum composition with uniform weights.
 */
std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components);

} // namespace FuncCraft

#endif // FUNCCRAFT_BUILDER_H
