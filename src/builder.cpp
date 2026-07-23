#include "builder.h"
#include "basicf.h"
#include "support.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

BasicFunctionId parse_basic_function_id(const std::string& name) {
    const std::string normalized = normalize_spec_name(name);
    for (BasicFunctionId id : list_basic_functions()) {
        if (normalize_spec_name(to_string(id)) == normalized) {
            return id;
        }
    }
    throw std::invalid_argument("unknown basic function name: " + name);
}

std::shared_ptr<BasicF> make_basic_function(const std::string& name, int dimension) {
    return make_basicf_ptr(parse_basic_function_id(name), dimension);
}

std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components) {
    require(components > 0, "weighted sum needs at least one component");
    return std::make_shared<WeightedSumComposition>(std::vector<double>(components, 1.0));
}

FunctionBuilder::FunctionBuilder(int dimension)
    : domain_(dimension) {
    require(dimension > 0, "function dimension must be positive");
}

FunctionBuilder& FunctionBuilder::domain(Domain domain) {
    require(domain.dimension() > 0, "domain must have positive dimension");
    domain_ = std::move(domain);
    return *this;
}

FunctionBuilder& FunctionBuilder::add_component(
    BasicFunctionId id,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform) {
    require(static_cast<bool>(coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(value_transform), "value transform is null");
    require(coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    require(coordinate_transform->output_dimension() > 0, "component dimension must be positive");

    const auto primitive = make_basicf_ptr(id, coordinate_transform->output_dimension());
    ComponentEvaluator evaluator = [primitive](const std::vector<std::vector<double>>& X) {
        return (*primitive)(X);
    };
    return add_component(
        std::move(evaluator),
        primitive->default_domain(),
        primitive->x_opt,
        primitive->f_opt,
        std::move(coordinate_transform),
        std::move(value_transform));
}

FunctionBuilder& FunctionBuilder::add_component(
    ComponentEvaluator evaluator,
    Domain child_domain,
    std::vector<double> child_xopt,
    double child_fopt,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform) {
    require(static_cast<bool>(evaluator), "component evaluator is empty");
    require(static_cast<bool>(coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(value_transform), "value transform is null");
    require(coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    require(coordinate_transform->output_dimension() == child_domain.dimension(), "component transform output dimension mismatch");
    require_dimension(child_xopt, child_domain.dimension(), "component child_xopt");

    runtime_components_.push_back(RuntimeComponent{
        std::move(evaluator),
        coordinate_transform,
        value_transform,
        std::move(child_domain),
        coordinate_transform->target_xopt(),
        std::move(child_xopt),
        child_fopt,
    });
    return *this;
}

FunctionBuilder& FunctionBuilder::composition(std::shared_ptr<CompositionFunction> composition) {
    require(static_cast<bool>(composition), "composition is null");
    composition_ = std::move(composition);
    return *this;
}

ComposedFunction FunctionBuilder::build() const {
    require(!runtime_components_.empty(), "cannot build function without components");
    auto components = std::make_shared<std::vector<RuntimeComponent>>(runtime_components_);
    std::shared_ptr<CompositionFunction> composition = composition_
        ? composition_
        : (runtime_components_.size() == 1
            ? std::shared_ptr<CompositionFunction>(std::make_shared<NoneComposition>())
            : make_weighted_sum(runtime_components_.size()));
    const int dimension = domain_.dimension();
    const double penalty = std::numeric_limits<double>::infinity();
    const Domain domain = domain_;

    return [components, composition, dimension, penalty, domain](const std::vector<std::vector<double>>& X) {
        std::vector<double> values;
        values.reserve(X.size());
        std::vector<double> component_values(components->size(), 0.0);
        std::vector<double> transformed(static_cast<std::size_t>(dimension), 0.0);
        std::vector<double> child_input(static_cast<std::size_t>(dimension), 0.0);
        for (const auto& x : X) {
            require_dimension(x, dimension, "benchmark function input");
            bool invalid = false;
            for (std::size_t component_index = 0; component_index < components->size(); ++component_index) {
                const auto& component = (*components)[component_index];
                component.coordinate_transform->apply(x, transformed);
                if (detail::squared_distance(transformed, component.target_xopt) <= 1.0e-24) {
                    child_input = component.child_xopt;
                } else {
                    detail::map_point_between_domains(transformed, domain, component.child_domain, child_input);
                }
                const std::vector<double> child_values = component.evaluate({child_input});
                if (child_values.size() != 1 || !std::isfinite(child_values.front())) {
                    invalid = true;
                    break;
                }
                double shifted_value = child_values.front() - component.child_fopt; //before the component value, the minimum value must still be preserved (which is zero) by design
                if (shifted_value < 0.0 && shifted_value >= -1.0e-12) {
                    shifted_value = 0.0;
                }
                if (shifted_value < 0.0) {
                    invalid = true;
                    break;
                }
                const double transformed_value = component.value_transform->apply(shifted_value);
                if (!std::isfinite(transformed_value)) {
                    invalid = true;
                    break;
                }
                component_values[component_index] = transformed_value;
            }
            if (invalid) {
                values.push_back(penalty);
                continue;
            }
            const double composed = composition->apply(x, component_values);
            if (!std::isfinite(composed)) {
                values.push_back(penalty);
                continue;
            }
            values.push_back(composed);
        }
        return values;
    };
}

} // namespace FuncCraft
