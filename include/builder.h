#ifndef FUNCCRAFT_BUILDER_H
#define FUNCCRAFT_BUILDER_H

#include "composition.h"

#include <map>
#include <memory>
#include <string>

namespace FuncCraft {

class FunctionBuilder final {
public:
    explicit FunctionBuilder(int dimension);

    FunctionBuilder& domain(Domain domain);
    FunctionBuilder& seed(unsigned long long seed);
    FunctionBuilder& known_global_minimizer(std::vector<double> x_star);
    FunctionBuilder& known_global_value(double f_star);
    FunctionBuilder& add_component(
        BasicFunctionId id,
        int component_dimension,
        std::shared_ptr<CoordinateTransform> coordinate_transform,
        std::shared_ptr<ValueTransform> value_transform);
    FunctionBuilder& composition(std::shared_ptr<CompositionFunction> composition);
    FunctionBuilder& parameter(std::string key, std::string value);

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

std::shared_ptr<ValueTransform> make_value_transform(ValueTransformClass cls, double a = 1.0, double b = 1.0);
std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components);

} // namespace FuncCraft

#endif // FUNCCRAFT_BUILDER_H
