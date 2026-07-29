#include "builder.h"
#include "basicf.h"
#include "runtime_profile.h"
#include "support.h"

#include <cmath>
#include <limits>
#include <memory>
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

FunctionBuilder::RuntimeDomainMap FunctionBuilder::make_runtime_domain_map(
    const Domain& source_domain,
    const Domain& target_domain) {
    require(source_domain.dimension() == target_domain.dimension(), "runtime domain map dimension mismatch");
    RuntimeDomainMap map;
    map.source_lower.resize(static_cast<std::size_t>(source_domain.dimension()));
    map.source_range.resize(static_cast<std::size_t>(source_domain.dimension()));
    map.target_lower.resize(static_cast<std::size_t>(source_domain.dimension()));
    map.target_range.resize(static_cast<std::size_t>(source_domain.dimension()));

    for (int i = 0; i < source_domain.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        map.source_lower[idx] = source_domain.lower[idx];
        map.source_range[idx] = source_domain.upper[idx] - source_domain.lower[idx];
        map.target_lower[idx] = target_domain.lower[idx];
        map.target_range[idx] = target_domain.upper[idx] - target_domain.lower[idx];
    }
    return map;
}

FunctionBuilder::RuntimeValueTransform FunctionBuilder::make_runtime_value_transform(
    const ValueTransform& value_transform) {
    RuntimeValueTransform runtime;
    runtime.kind = value_transform.transform_class();

    if (runtime.kind == ValueTransformClass::Power) {
        const auto* typed = dynamic_cast<const PowerValueTransform*>(&value_transform);
        require(typed != nullptr, "power value transform type mismatch");
        runtime.alpha = typed->alpha();
        runtime.p = typed->p();
    } else if (runtime.kind == ValueTransformClass::Oscillatory) {
        const auto* typed = dynamic_cast<const OscillatoryValueTransform*>(&value_transform);
        require(typed != nullptr, "oscillatory value transform type mismatch");
        runtime.alpha = typed->alpha();
        runtime.epsilon = typed->epsilon();
    } else if (runtime.kind == ValueTransformClass::CosineZero) {
        const auto* typed = dynamic_cast<const CosineZeroValueTransform*>(&value_transform);
        require(typed != nullptr, "cosine-zero value transform type mismatch");
        runtime.alpha = typed->alpha();
    } else if (runtime.kind == ValueTransformClass::Mixed) {
        throw std::logic_error("mixed value transform is not a concrete runtime transform");
    }
    return runtime;
}

void FunctionBuilder::map_component_domain(
    const RuntimeDomainMap& map,
    const std::vector<double>& point,
    std::vector<double>& out) {
    if (out.size() != point.size()) {
        out.resize(point.size());
    }
    for (std::size_t i = 0; i < point.size(); ++i) {
        if (map.source_range[i] == 0.0) {
            out[i] = map.target_lower[i] + 0.5 * map.target_range[i];
            continue;
        }
        const double t = (point[i] - map.source_lower[i]) / map.source_range[i];
        out[i] = map.target_lower[i] + t * map.target_range[i];
    }
}

double FunctionBuilder::apply_runtime_value_transform(const RuntimeValueTransform& transform, double u) {
    double value = u;
    switch (transform.kind) {
    case ValueTransformClass::None:
        value = u;
        break;
    case ValueTransformClass::Power:
        value = transform.alpha * positive_power_fast(u, transform.p);
        break;
    case ValueTransformClass::Oscillatory:
        value = u * (1.0 + transform.epsilon * std::sin(transform.alpha * u));
        break;
    case ValueTransformClass::CosineZero:
        value = 1.0 - std::cos(transform.alpha * u);
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

bool FunctionBuilder::finalize_component_value(
    const EvaluableComponent& component,
    double child_value,
    double& out) {
    FUNCCRAFT_PROFILE_SCOPE(FinalizeValue);
    if (!std::isfinite(child_value)) {
        return false;
    }

    double shifted_value = child_value - component.child_fopt;
    if (shifted_value < 0.0 && shifted_value >= -1.0e-12) {
        shifted_value = 0.0;
    }
    if (shifted_value < 0.0) {
        return false;
    }

    const double transformed_value = apply_runtime_value_transform(component.value_transform, shifted_value);
    if (transformed_value < 0.0 || !std::isfinite(transformed_value)) {
        return false;
    }

    const double scaled_value = component.scale_factor * transformed_value;
    if (!std::isfinite(scaled_value)) {
        return false;
    }
    out = detail::stable_numeric_value(scaled_value);
    return true;
}

bool FunctionBuilder::evaluate_component(
    const EvaluableComponent& component,
    const std::vector<double>& x,
    std::vector<double>& transformed,
    std::vector<double>& child_input,
    double& out) {
    FUNCCRAFT_PROFILE_SCOPE(ComponentTotal);
    {
        FUNCCRAFT_PROFILE_SCOPE(CoordinateTransform);
        component.coordinate_transform->apply(x, transformed);
    }
    if (detail::squared_distance(transformed, component.target_xopt) <= 1.0e-24) {
        child_input = component.child_xopt;
    } else {
        FUNCCRAFT_PROFILE_SCOPE(DomainMap);
        map_component_domain(component.domain_map, transformed, child_input);
    }

    double child_value = 0.0;
    if (component.primitive) {
        FUNCCRAFT_PROFILE_SCOPE(PrimitiveEvaluate);
        child_value = component.primitive->evaluate(child_input.data());
    } else {
        FUNCCRAFT_PROFILE_SCOPE(NestedEvaluate);
        if (component.evaluate_scalar) {
            child_value = component.evaluate_scalar(child_input);
        } else {
            const std::vector<double> child_values = component.evaluate({child_input});
            if (child_values.size() != 1) {
                return false;
            }
            child_value = child_values.front();
        }
        if (!std::isfinite(child_value)) {
            return false;
        }
    }
    return finalize_component_value(component, child_value, out);
}

FunctionBuilder& FunctionBuilder::add_component(ResolvedComponent component) {
    require(
        static_cast<bool>(component.primitive)
            || static_cast<bool>(component.scalar_evaluator)
            || static_cast<bool>(component.evaluator),
        "component needs a primitive, scalar evaluator, or batch evaluator");
    require(static_cast<bool>(component.coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(component.value_transform), "value transform is null");
    require(std::isfinite(component.child_fopt), "component child_fopt must be finite");
    require(std::isfinite(component.scale_factor), "component scale_factor must be finite");
    require(component.scale_factor > 0.0, "component scale_factor must be positive");
    require(component.coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    require(component.coordinate_transform->output_dimension() > 0, "component dimension must be positive");
    Domain transform_domain = transform_output_domain(domain_, *component.coordinate_transform);
    require(
        component.coordinate_transform->output_dimension() == transform_domain.dimension(),
        "component transform domain dimension mismatch");
    require(
        component.coordinate_transform->output_dimension() == component.child_domain.dimension(),
        "component transform output dimension mismatch");
    require_dimension(component.child_xopt, component.child_domain.dimension(), "component child_xopt");

    evaluable_components_.push_back(EvaluableComponent{
        std::move(component.evaluator),
        std::move(component.scalar_evaluator),
        std::move(component.primitive),
        component.coordinate_transform,
        make_runtime_value_transform(*component.value_transform),
        make_runtime_domain_map(transform_domain, component.child_domain),
        component.coordinate_transform->target_xopt(),
        std::move(component.child_xopt),
        component.child_fopt,
        component.scale_factor,
    });
    return *this;
}

FunctionBuilder& FunctionBuilder::composition(std::shared_ptr<CompositionFunction> composition) {
    require(static_cast<bool>(composition), "composition is null");
    composition_ = std::move(composition);
    return *this;
}

ScalarFunction FunctionBuilder::build_scalar() const {
    require(!evaluable_components_.empty(), "cannot build function without components");
    auto components = std::make_shared<std::vector<EvaluableComponent>>(evaluable_components_);
    std::shared_ptr<CompositionFunction> composition = composition_
        ? composition_
        : (evaluable_components_.size() == 1
            ? std::shared_ptr<CompositionFunction>(std::make_shared<NoneComposition>())
            : make_weighted_sum(evaluable_components_.size()));
    const int dimension = domain_.dimension();
    const double penalty = std::numeric_limits<double>::infinity();
    struct Scratch {
        std::vector<double> component_values;
        std::vector<double> transformed;
        std::vector<double> child_input;
    };
    auto scratch = std::make_shared<Scratch>();
    scratch->component_values.assign(components->size(), 0.0);
    scratch->transformed.assign(static_cast<std::size_t>(dimension), 0.0);

    return [components, composition, dimension, penalty, scratch](const std::vector<double>& x) {
        require_dimension(x, dimension, "benchmark function input");
        bool invalid = false;
        for (std::size_t component_index = 0; component_index < components->size(); ++component_index) {
            const auto& component = (*components)[component_index];
            if (!evaluate_component(
                    component,
                    x,
                    scratch->transformed,
                    scratch->child_input,
                    scratch->component_values[component_index])) {
                invalid = true;
                break;
            }
        }
        if (invalid) {
            return penalty;
        }
        double composed = 0.0;
        {
            FUNCCRAFT_PROFILE_SCOPE(Composition);
            composed = composition->apply(x, scratch->component_values);
        }
        return std::isfinite(composed) ? composed : penalty;
    };
}

ComposedFunction FunctionBuilder::build() const {
    ScalarFunction scalar = build_scalar();
    return [scalar = std::move(scalar)](const std::vector<std::vector<double>>& X) {
        std::vector<double> values;
        values.reserve(X.size());
        for (const auto& x : X) {
            values.push_back(scalar(x));
        }
        return values;
    };
}

} // namespace FuncCraft
