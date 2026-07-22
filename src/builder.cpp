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

Domain make_domain(const FunctionSpec& spec) {
    require(spec.dimension > 0, "function dimension must be positive");

    Domain domain(spec.dimension);
    if (!spec.domain.lower_bound.empty() || !spec.domain.upper_bound.empty()) {
        require(static_cast<int>(spec.domain.lower_bound.size()) == spec.dimension, "lower bound dimension mismatch");
        require(static_cast<int>(spec.domain.upper_bound.size()) == spec.dimension, "upper bound dimension mismatch");
        domain.lower = spec.domain.lower_bound;
        domain.upper = spec.domain.upper_bound;
    }
    require(domain.lower.size() == domain.upper.size(), "domain bound size mismatch");
    for (std::size_t i = 0; i < domain.lower.size(); ++i) {
        require(domain.lower[i] <= domain.upper[i], "domain lower bound must not exceed upper bound");
    }
    return domain;
}

std::shared_ptr<CoordinateTransform> make_coordinate_transform(
    const CoordinateTransformSpec& spec,
    const std::vector<double>& target_xopt) {
    const CoordinateTransformKind kind = spec.kind;
    require(spec.dimension > 0, "coordinate transform dimension must be positive");
    require(static_cast<int>(spec.assigned_xopt.size()) == spec.dimension, "coordinate transform assigned_xopt dimension mismatch");
    require(static_cast<int>(target_xopt.size()) == spec.dimension, "coordinate transform target_xopt dimension mismatch");

    const std::uint64_t seed = spec.seed;
    if (kind == CoordinateTransformKind::None) {
        return std::make_shared<IdentityTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::Rotation) {
        if (!spec.matrix.empty()) {
            return std::make_shared<RotationTransform>(
                spec.dimension,
                spec.assigned_xopt,
                target_xopt,
                seed,
                spec.matrix);
        }
        return std::make_shared<RotationTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::Affine) {
        if (!spec.matrix.empty()) {
            return std::make_shared<AffineTransform>(
                spec.dimension,
                spec.assigned_xopt,
                target_xopt,
                seed,
                spec.matrix);
        }
        return std::make_shared<AffineTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::BlockRotation) {
        require(!spec.selected_indices.empty(), "block rotation transform needs selected indices");
        if (!spec.matrix.empty()) {
            return std::make_shared<BlockRotationTransform>(
                spec.dimension,
                spec.selected_indices,
                spec.assigned_xopt,
                target_xopt,
                seed,
                spec.matrix);
        }
        return std::make_shared<BlockRotationTransform>(
            spec.dimension,
            spec.selected_indices,
            spec.assigned_xopt,
            target_xopt,
            seed);
    }
    throw std::logic_error("unhandled coordinate transform kind");
}

std::shared_ptr<ValueTransform> make_value_transform(const ValueTransformSpec& spec) {
    const ValueTransformKind kind = spec.kind;
    if (kind == ValueTransformKind::None) {
        return std::make_shared<IdentityValueTransform>();
    }
    if (kind == ValueTransformKind::Power) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        const double p = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<PowerValueTransform>(alpha, p);
    }
    if (kind == ValueTransformKind::Oscillatory) {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<OscillatoryValueTransform>(epsilon, alpha);
    }
    if (kind == ValueTransformKind::CosineZero) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        return std::make_shared<CosineZeroValueTransform>(alpha);
    }
    throw std::logic_error("unhandled value transform kind");
}

std::shared_ptr<CompositionFunction> make_composition(const CompositionSpec& spec, std::size_t component_count) {
    const CompositionKind kind = spec.kind;
    if (kind == CompositionKind::None) {
        require(component_count == 1, "no composition requires exactly one component");
        return std::make_shared<NoneComposition>();
    }
    if (kind == CompositionKind::CpmWeightedSum) {
        return std::make_shared<WeightedSumComposition>(component_count);
    }
    if (kind == CompositionKind::CpmPowerMean) {
        const double p = spec.parameters.empty() ? 2.0 : spec.parameters[0];
        return std::make_shared<PowerMeanComposition>(component_count, p);
    }
    if (kind == CompositionKind::CpmLevelWell) {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<LevelWellComposition>(component_count, epsilon, alpha);
    }
    if (kind == CompositionKind::DpmSoftmax) {
        throw std::invalid_argument("DPM softmax composition must be built from component coordinate-transform assigned_xopt");
    }
    if (kind == CompositionKind::DpmBgSoftmax) {
        throw std::invalid_argument("DPM bg softmax composition must be built from component coordinate-transform assigned_xopt");
    }
    throw std::logic_error("unhandled composition kind");
}

std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components) {
    require(components > 0, "weighted sum needs at least one component");
    return std::make_shared<WeightedSumComposition>(std::vector<double>(components, 1.0));
}

FunctionBuilder::FunctionBuilder(int dimension)
    : domain_(dimension),
      assigned_xopt_(static_cast<std::size_t>(dimension), 0.0) {
    require(dimension > 0, "function dimension must be positive");
}

FunctionBuilder& FunctionBuilder::domain(Domain domain) {
    require(domain.dimension() > 0, "domain must have positive dimension");
    domain_ = std::move(domain);
    if (assigned_xopt_.empty() || static_cast<int>(assigned_xopt_.size()) != domain_.dimension()) {
        assigned_xopt_.assign(static_cast<std::size_t>(domain_.dimension()), 0.0);
    }
    return *this;
}

FunctionBuilder& FunctionBuilder::seed(unsigned long long seed) {
    seed_ = seed;
    return *this;
}

FunctionBuilder& FunctionBuilder::assigned_xopt(std::vector<double> x_opt) {
    require_dimension(x_opt, domain_.dimension(), "assigned_xopt");
    assigned_xopt_ = std::move(x_opt);
    return *this;
}

FunctionBuilder& FunctionBuilder::assigned_fopt(double f_opt) {
    assigned_fopt_ = f_opt;
    return *this;
}

FunctionBuilder& FunctionBuilder::scale_factor(std::optional<double> scale_factor) {
    scale_factor_ = scale_factor;
    return *this;
}

FunctionBuilder& FunctionBuilder::add_component(
    BasicFunctionId id,
    int component_dimension,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform,
    double f_bias) {
    require(static_cast<bool>(coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(value_transform), "value transform is null");
    require(component_dimension > 0, "component dimension must be positive");
    require(coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    require(coordinate_transform->output_dimension() == component_dimension, "component transform output dimension mismatch");

    ComponentSpec component_spec;
    component_spec.base_function = id;
    component_spec.component_dimension = component_dimension;
    component_spec.coordinate_transform = coordinate_transform->export_spec();
    component_spec.value_transform = value_transform->export_spec();
    component_spec.f_bias = f_bias;
    component_spec.seed = coordinate_transform->seed();
    component_specs_.push_back(std::move(component_spec));

    function_class_.base_functions.push_back(id);
    const CoordinateTransformClass t_class = coordinate_transform->transform_class();
    const ValueTransformClass p_class = value_transform->transform_class();
    if (component_specs_.size() == 1) {
        function_class_.coordinate_transform = t_class;
        function_class_.value_transform = p_class;
    } else {
        if (function_class_.coordinate_transform != t_class) {
            function_class_.coordinate_transform = CoordinateTransformClass::Mixed;
        }
        if (function_class_.value_transform != p_class) {
            function_class_.value_transform = ValueTransformClass::Mixed;
        }
    }
    return *this;
}

FunctionBuilder& FunctionBuilder::composition(std::shared_ptr<CompositionFunction> composition) {
    require(static_cast<bool>(composition), "composition is null");
    composition_ = std::move(composition);
    function_class_.composition = composition_->composition_class();
    return *this;
}

FunctionSpec FunctionBuilder::spec() const {
    return build_spec();
}

FunctionSpec FunctionBuilder::build_spec() const {
    require(!component_specs_.empty(), "cannot build function without components");

    FunctionClass cls = function_class_;
    const std::shared_ptr<CompositionFunction> composition = composition_
        ? composition_
        : (component_specs_.size() == 1
            ? std::shared_ptr<CompositionFunction>(std::make_shared<NoneComposition>())
            : make_weighted_sum(component_specs_.size()));
    if (function_class_.composition == CompositionClass::None) {
        cls.composition = composition->composition_class();
    }

    FunctionSpec spec;
    spec.dimension = domain_.dimension();
    spec.domain.dimension = domain_.dimension();
    spec.domain.lower_bound = domain_.lower;
    spec.domain.upper_bound = domain_.upper;
    spec.seed = seed_;
    spec.components.reserve(component_specs_.size());
    spec.components = component_specs_;
    const CompositionSpec composition_spec = composition->export_spec();
    spec.composition.kind = composition_spec.kind;
    spec.composition.parameters = composition_spec.parameters;
    spec.composition.seed = composition_spec.seed;
    spec.assigned_xopt = assigned_xopt_;
    spec.assigned_fopt = assigned_fopt_;
    spec.scale_factor = scale_factor_;
    spec.label = class_label(cls);
    return spec;
}

ComposedFunction FunctionBuilder::build() const {
    const FunctionSpec built_spec = build_spec();

    struct RuntimeComponent {
        std::shared_ptr<BasicF> basic_function;
        std::shared_ptr<CoordinateTransform> coordinate_transform;
        std::shared_ptr<ValueTransform> value_transform;
        Domain native_domain;
        std::vector<double> target_xopt;
        std::vector<double> native_xopt;
    };

    auto components = std::make_shared<std::vector<RuntimeComponent>>();
    components->reserve(component_specs_.size());
    const Domain domain = domain_;
    for (const ComponentSpec& component_spec : component_specs_) {
        const auto primitive = make_basicf_ptr(component_spec.base_function, component_spec.component_dimension);
        const Domain native_domain = primitive->default_domain();
        const std::vector<double> target_xopt = detail::map_point_between_domains(
            primitive->x_opt,
            native_domain,
            domain);
        components->push_back(RuntimeComponent{
            primitive,
            make_coordinate_transform(component_spec.coordinate_transform, target_xopt),
            make_value_transform(component_spec.value_transform),
            native_domain,
            target_xopt,
            primitive->x_opt,
        });
    }

    std::shared_ptr<CompositionFunction> composition = composition_
        ? composition_
        : make_composition(built_spec.composition, built_spec.components.size());
    const int dimension = built_spec.dimension;
    const double penalty = std::numeric_limits<double>::infinity();

    return [components, composition, dimension, penalty, domain](const std::vector<std::vector<double>>& X) {
        std::vector<double> values;
        values.reserve(X.size());
        std::vector<double> component_values(components->size(), 0.0);
        std::vector<double> transformed(static_cast<std::size_t>(dimension), 0.0);
        std::vector<double> normalized(static_cast<std::size_t>(dimension), 0.0);
        for (const auto& x : X) {
            require_dimension(x, dimension, "benchmark function input");
            bool invalid = false;
            for (std::size_t component_index = 0; component_index < components->size(); ++component_index) {
                const auto& component = (*components)[component_index];
                component.coordinate_transform->apply(x, transformed);
                if (detail::squared_distance(transformed, component.target_xopt) <= 1.0e-24) {
                    normalized = component.native_xopt;
                } else {
                    detail::map_point_between_domains(transformed, domain, component.native_domain, normalized);
                }
                const double raw_value = component.basic_function->evaluate(normalized.data());
                if (!std::isfinite(raw_value)) {
                    invalid = true;
                    break;
                }
                // Value transforms are defined on nonnegative inputs.
                // Shift by the primitive optimum so negative-valued base functions
                // remain valid.
                double shifted_value = raw_value - component.basic_function->f_opt;
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
