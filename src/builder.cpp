#include "builder.h"
#include "basicf.h"
#include "support.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {

Domain transform_output_domain(const Domain& domain, const CoordinateTransform& transform) {
    const auto* subspace = dynamic_cast<const SubspaceRotationTransform*>(&transform);
    if (subspace != nullptr) {
        const auto& indices = subspace->selected_indices();
        Domain subdomain(static_cast<int>(indices.size()));
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const int idx = indices[i];
            require(idx >= 0 && idx < domain.dimension(), "subspace rotation selected index out of range");
            subdomain.lower[i] = domain.lower[static_cast<std::size_t>(idx)];
            subdomain.upper[i] = domain.upper[static_cast<std::size_t>(idx)];
        }
        return subdomain;
    }
    if (transform.output_dimension() == domain.dimension()) {
        return domain;
    }
    return Domain(transform.output_dimension());
}

double positive_power_fast(double value, double exponent) {
    if (value == 0.0) {
        return 0.0;
    }
    if (exponent == 1.0) {
        return value;
    }
    if (exponent == 2.0) {
        return value * value;
    }
    if (exponent == 0.5) {
        return std::sqrt(value);
    }
    return std::pow(value, exponent);
}

double apply_value_transform_fast(
    ValueTransformClass value_transform_class,
    double value_alpha,
    double value_p,
    double value_epsilon,
    double u) {
    double value = u;
    switch (value_transform_class) {
    case ValueTransformClass::None:
        value = u;
        break;
    case ValueTransformClass::Power:
        value = value_alpha * positive_power_fast(u, value_p);
        break;
    case ValueTransformClass::Oscillatory:
        value = u * (1.0 + value_epsilon * std::sin(value_alpha * u));
        break;
    case ValueTransformClass::CosineZero:
        value = 1.0 - std::cos(value_alpha * u);
        break;
    case ValueTransformClass::Mixed:
        throw std::logic_error("mixed value transform is not a concrete runtime transform");
    }
    if (!std::isfinite(value)) {
        return std::numeric_limits<double>::max();
    }
    if (value < 0.0 && value >= -1.0e-12) {
        return 0.0;
    }
    return value;
}

bool finalize_component_value(
    double child_value,
    double child_fopt,
    double scale_factor,
    ValueTransformClass value_transform_class,
    double value_alpha,
    double value_p,
    double value_epsilon,
    double& out) {
    if (!std::isfinite(child_value)) {
        return false;
    }
    double shifted_value = child_value - child_fopt;
    if (shifted_value < 0.0 && shifted_value >= -1.0e-12) {
        shifted_value = 0.0;
    }
    if (shifted_value < 0.0) {
        return false;
    }
    const double transformed_value = apply_value_transform_fast(
        value_transform_class,
        value_alpha,
        value_p,
        value_epsilon,
        shifted_value);
    if (transformed_value < 0.0 || !std::isfinite(transformed_value)) {
        return false;
    }
    const double scaled_value = scale_factor * transformed_value;
    if (!std::isfinite(scaled_value)) {
        return false;
    }
    out = detail::stable_numeric_value(scaled_value);
    return true;
}

void map_component_domain(
    const std::vector<double>& domain_scale,
    const std::vector<double>& domain_offset,
    const std::vector<double>& point,
    std::vector<double>& out) {
    if (out.size() != point.size()) {
        out.resize(point.size());
    }
    for (std::size_t i = 0; i < point.size(); ++i) {
        out[i] = domain_scale[i] * point[i] + domain_offset[i];
    }
}

} // namespace

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
        primitive,
        primitive->default_domain(),
        primitive->x_opt,
        primitive->f_opt,
        1.0,
        std::move(coordinate_transform),
        std::move(value_transform));
}

FunctionBuilder& FunctionBuilder::add_component(
    ComponentEvaluator evaluator,
    Domain child_domain,
    std::vector<double> child_xopt,
    double child_fopt,
    double component_scale_factor,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform) {
    return add_component(
        std::move(evaluator),
        {},
        std::move(child_domain),
        std::move(child_xopt),
        child_fopt,
        component_scale_factor,
        std::move(coordinate_transform),
        std::move(value_transform));
}

FunctionBuilder& FunctionBuilder::add_component(
    ComponentEvaluator evaluator,
    std::shared_ptr<BasicF> primitive,
    Domain child_domain,
    std::vector<double> child_xopt,
    double child_fopt,
    double component_scale_factor,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform) {
    require(static_cast<bool>(evaluator), "component evaluator is empty");
    require(static_cast<bool>(coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(value_transform), "value transform is null");
    require(std::isfinite(component_scale_factor), "component scale_factor must be finite");
    require(component_scale_factor > 0.0, "component scale_factor must be positive");
    require(coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    Domain transform_domain = transform_output_domain(domain_, *coordinate_transform);
    require(coordinate_transform->output_dimension() == transform_domain.dimension(), "component transform domain dimension mismatch");
    require(coordinate_transform->output_dimension() == child_domain.dimension(), "component transform output dimension mismatch");
    require_dimension(child_xopt, child_domain.dimension(), "component child_xopt");

    const ValueTransformClass value_transform_class = value_transform->transform_class();
    double value_alpha = 1.0;
    double value_p = 1.0;
    double value_epsilon = 0.1;
    if (value_transform_class == ValueTransformClass::Power) {
        const auto* typed = dynamic_cast<const PowerValueTransform*>(value_transform.get());
        require(typed != nullptr, "power value transform type mismatch");
        value_alpha = typed->alpha();
        value_p = typed->p();
    } else if (value_transform_class == ValueTransformClass::Oscillatory) {
        const auto* typed = dynamic_cast<const OscillatoryValueTransform*>(value_transform.get());
        require(typed != nullptr, "oscillatory value transform type mismatch");
        value_alpha = typed->alpha();
        value_epsilon = typed->epsilon();
    } else if (value_transform_class == ValueTransformClass::CosineZero) {
        const auto* typed = dynamic_cast<const CosineZeroValueTransform*>(value_transform.get());
        require(typed != nullptr, "cosine-zero value transform type mismatch");
        value_alpha = typed->alpha();
    }

    std::vector<double> domain_scale(static_cast<std::size_t>(transform_domain.dimension()), 1.0);
    std::vector<double> domain_offset(static_cast<std::size_t>(transform_domain.dimension()), 0.0);
    for (int i = 0; i < transform_domain.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        const double source_lo = transform_domain.lower[idx];
        const double source_hi = transform_domain.upper[idx];
        const double target_lo = child_domain.lower[idx];
        const double target_hi = child_domain.upper[idx];
        if (source_hi == source_lo) {
            domain_scale[idx] = 0.0;
            domain_offset[idx] = 0.5 * (target_lo + target_hi);
            continue;
        }
        domain_scale[idx] = (target_hi - target_lo) / (source_hi - source_lo);
        domain_offset[idx] = target_lo - domain_scale[idx] * source_lo;
    }

    runtime_components_.push_back(RuntimeComponent{
        std::move(evaluator),
        std::move(primitive),
        coordinate_transform,
        value_transform_class,
        coordinate_transform->target_xopt(),
        std::move(child_xopt),
        std::move(domain_scale),
        std::move(domain_offset),
        child_fopt,
        component_scale_factor,
        value_alpha,
        value_p,
        value_epsilon,
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
        std::vector<double> child_input;
        std::vector<std::vector<double>> child_batch(1);
        for (const auto& x : X) {
            require_dimension(x, dimension, "benchmark function input");
            bool invalid = false;
            for (std::size_t component_index = 0; component_index < components->size(); ++component_index) {
                const auto& component = (*components)[component_index];
                component.coordinate_transform->apply(x, transformed);
                if (detail::squared_distance(transformed, component.target_xopt) <= 1.0e-24) {
                    child_input = component.child_xopt;
                } else {
                    map_component_domain(component.domain_scale, component.domain_offset, transformed, child_input);
                }

                double child_value = 0.0;
                if (component.primitive) {
                    child_value = component.primitive->evaluate(child_input.data());
                } else {
                    child_batch.front() = child_input;
                    const std::vector<double> child_values = component.evaluate(child_batch);
                    if (child_values.size() != 1 || !std::isfinite(child_values.front())) {
                        invalid = true;
                        break;
                    }
                    child_value = child_values.front();
                }

                double component_value = 0.0;
                if (!finalize_component_value(
                        child_value,
                        component.child_fopt,
                        component.scale_factor,
                        component.value_transform_class,
                        component.value_alpha,
                        component.value_p,
                        component.value_epsilon,
                        component_value)) {
                    invalid = true;
                    break;
                }
                component_values[component_index] = component_value;
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
