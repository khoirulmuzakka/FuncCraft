#include "builder.h"
#include "basicf.h"
#include "support.h"

#include <map>
#include <cmath>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {

std::string normalize_token(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (ch == '_' || ch == '-' || std::isspace(ch)) {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized;
}

} // namespace

BasicFunctionId parse_basic_function_id(const std::string& name) {
    const std::string normalized = normalize_token(name);
    for (BasicFunctionId id : list_basic_functions()) {
        if (normalize_token(to_string(id)) == normalized) {
            return id;
        }
    }
    throw std::invalid_argument("unknown basic function name: " + name);
}

std::shared_ptr<BasicF> make_basic_function(const std::string& name, int dimension) {
    return make_basicf_ptr(parse_basic_function_id(name), dimension);
}

std::map<std::string, std::string> parse_parameters(const std::vector<std::string>& parameters) {
    std::map<std::string, std::string> result;
    for (const std::string& entry : parameters) {
        const std::size_t pos = entry.find('=');
        if (pos == std::string::npos) {
            result[entry] = "";
        } else {
            result[entry.substr(0, pos)] = entry.substr(pos + 1);
        }
    }
    return result;
}

Domain make_domain(const FunctionSpec& spec) {
    require(spec.dimension > 0, "function dimension must be positive");

    Domain domain(spec.dimension);
    if (!spec.lower_bound.empty() || !spec.upper_bound.empty()) {
        require(static_cast<int>(spec.lower_bound.size()) == spec.dimension, "lower bound dimension mismatch");
        require(static_cast<int>(spec.upper_bound.size()) == spec.dimension, "upper bound dimension mismatch");
        domain.lower = spec.lower_bound;
        domain.upper = spec.upper_bound;
    }
    require(domain.lower.size() == domain.upper.size(), "domain bound size mismatch");
    for (std::size_t i = 0; i < domain.lower.size(); ++i) {
        require(domain.lower[i] <= domain.upper[i], "domain lower bound must not exceed upper bound");
    }
    return domain;
}

std::shared_ptr<CoordinateTransform> make_coordinate_transform(const TransformSpec& spec) {
    const std::string kind = normalize_token(spec.kind);
    require(spec.dimension > 0, "coordinate transform dimension must be positive");
    require(static_cast<int>(spec.source_point.size()) == spec.dimension, "coordinate transform source point dimension mismatch");
    require(static_cast<int>(spec.target_point.size()) == spec.dimension, "coordinate transform target point dimension mismatch");

    const std::uint64_t seed = static_cast<std::uint64_t>(static_cast<std::uint32_t>(spec.seed));
    if (kind.empty() || kind == "identity" || kind == "none") {
        return std::make_shared<IdentityTransform>(spec.dimension, spec.source_point, spec.target_point, seed);
    }
    if (kind == "rotation" || kind == "rot") {
        return std::make_shared<RotationTransform>(spec.dimension, spec.source_point, spec.target_point, seed);
    }
    if (kind == "affine" || kind == "aff") {
        return std::make_shared<AffineTransform>(spec.dimension, spec.source_point, spec.target_point, seed);
    }
    if (kind == "blockrotation" || kind == "blockrot" || kind == "brot") {
        require(!spec.selected_indices.empty(), "block rotation transform needs selected indices");
        return std::make_shared<BlockRotationTransform>(
            spec.dimension,
            spec.selected_indices,
            spec.source_point,
            spec.target_point,
            seed);
    }
    throw std::invalid_argument("unknown coordinate transform kind: " + spec.kind);
}

std::shared_ptr<ValueTransform> make_value_transform(const ValueTransformSpec& spec) {
    const std::string kind = normalize_token(spec.kind);
    if (kind.empty() || kind == "identity" || kind == "none") {
        return std::make_shared<IdentityValueTransform>();
    }
    if (kind == "power") {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        const double p = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<PowerValueTransform>(alpha, p);
    }
    if (kind == "oscillatory" || kind == "osc") {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<OscillatoryValueTransform>(epsilon, alpha);
    }
    if (kind == "cosinezero" || kind == "coszero") {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        return std::make_shared<CosineZeroValueTransform>(alpha);
    }
    throw std::invalid_argument("unknown value transform kind: " + spec.kind);
}

std::shared_ptr<CompositionFunction> make_composition(const CompositionSpec& spec, std::size_t component_count) {
    const std::string kind = normalize_token(spec.kind);
    if (kind.empty() || kind == "singlecomponent" || kind == "single") {
        require(component_count == 1, "single-component composition requires exactly one component");
        return std::make_shared<SingleComponentComposition>();
    }
    if (kind == "weightedsum" || kind == "sum" || kind == "cpm") {
        std::vector<double> weights = spec.weights.empty()
            ? std::vector<double>(component_count, 1.0)
            : spec.weights;
        return std::make_shared<WeightedSumComposition>(std::move(weights));
    }
    if (kind == "powermean" || kind == "powermeancomposition") {
        const double p = spec.parameters.empty() ? 2.0 : spec.parameters[0];
        std::vector<double> weights = spec.weights.empty()
            ? std::vector<double>(component_count, 1.0)
            : spec.weights;
        return std::make_shared<PowerMeanComposition>(std::move(weights), p);
    }
    if (kind == "levelwell" || kind == "lvlwell") {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        std::vector<double> weights = spec.weights.empty()
            ? std::vector<double>(component_count, 1.0)
            : spec.weights;
        return std::make_shared<LevelWellComposition>(std::move(weights), epsilon, alpha);
    }
    if (kind == "deceptivesoftmax" || kind == "dpm") {
        const double sharpness = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        const double local_selection_radius = spec.parameters.size() > 1 ? spec.parameters[1] : 0.0;
        std::vector<std::vector<double>> centers = spec.centers;
        std::vector<double> offsets = spec.offsets;
        require(!centers.empty(), "deceptive softmax composition needs centers");
        if (offsets.empty()) {
            offsets.assign(centers.size(), 0.0);
        }
        return std::make_shared<DeceptiveSoftmaxComposition>(
            std::move(centers),
            std::move(offsets),
            sharpness,
            local_selection_radius);
    }
    throw std::invalid_argument("unknown composition kind: " + spec.kind);
}

std::shared_ptr<CompositionFunction> make_weighted_sum(std::size_t components) {
    require(components > 0, "weighted sum needs at least one component");
    return std::make_shared<WeightedSumComposition>(std::vector<double>(components, 1.0));
}

FunctionBuilder::FunctionBuilder(int dimension)
    : domain_(dimension),
      x_star_(static_cast<std::size_t>(dimension), 0.0) {
    require(dimension > 0, "function dimension must be positive");
}

FunctionBuilder& FunctionBuilder::domain(Domain domain) {
    require(domain.dimension() > 0, "domain must have positive dimension");
    domain_ = std::move(domain);
    if (x_star_.empty() || static_cast<int>(x_star_.size()) != domain_.dimension()) {
        x_star_.assign(static_cast<std::size_t>(domain_.dimension()), 0.0);
    }
    return *this;
}

FunctionBuilder& FunctionBuilder::seed(unsigned long long seed) {
    seed_ = seed;
    return *this;
}

FunctionBuilder& FunctionBuilder::known_global_minimizer(std::vector<double> x_star) {
    require_dimension(x_star, domain_.dimension(), "known global minimizer");
    x_star_ = std::move(x_star);
    return *this;
}

FunctionBuilder& FunctionBuilder::known_global_value(double f_star) {
    f_star_ = f_star;
    return *this;
}

FunctionBuilder& FunctionBuilder::add_component(
    BasicFunctionId id,
    int component_dimension,
    std::shared_ptr<CoordinateTransform> coordinate_transform,
    std::shared_ptr<ValueTransform> value_transform) {
    require(static_cast<bool>(coordinate_transform), "coordinate transform is null");
    require(static_cast<bool>(value_transform), "value transform is null");
    require(component_dimension > 0, "component dimension must be positive");
    require(coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
    require(coordinate_transform->output_dimension() == component_dimension, "component transform output dimension mismatch");

    ComponentSpec component_spec;
    component_spec.base_function = to_string(id);
    component_spec.component_dimension = component_dimension;
    component_spec.coordinate_transform = coordinate_transform->spec();
    component_spec.value_transform = value_transform->spec();
    component_spec.seed = static_cast<int>(coordinate_transform->seed());
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

FunctionBuilder& FunctionBuilder::parameter(std::string key, std::string value) {
    parameters_[std::move(key)] = std::move(value);
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
            ? std::shared_ptr<CompositionFunction>(std::make_shared<SingleComponentComposition>())
            : make_weighted_sum(component_specs_.size()));
    if (function_class_.composition == CompositionClass::None) {
        cls.composition = composition->composition_class();
    }

    FunctionSpec spec;
    spec.dimension = domain_.dimension();
    spec.lower_bound = domain_.lower;
    spec.upper_bound = domain_.upper;
    spec.seed = seed_;
    spec.component_specs = component_specs_;
    spec.composition_spec = composition->spec();
    spec.function_class_label = class_label(cls);
    spec.known_global_minimizer = x_star_;
    spec.known_global_value = f_star_;
    spec.parameters.reserve(parameters_.size());
    for (const auto& [key, value] : parameters_) {
        spec.parameters.push_back(key + "=" + value);
    }
    return spec;
}

ComposedFunction FunctionBuilder::build() const {
    const FunctionSpec built_spec = build_spec();

    struct RuntimeComponent {
        std::shared_ptr<BasicF> basic_function;
        std::shared_ptr<CoordinateTransform> coordinate_transform;
        std::shared_ptr<ValueTransform> value_transform;
    };

    auto components = std::make_shared<std::vector<RuntimeComponent>>();
    components->reserve(built_spec.component_specs.size());
    for (const ComponentSpec& component_spec : built_spec.component_specs) {
        components->push_back(RuntimeComponent{
            make_basic_function(component_spec.base_function, component_spec.component_dimension),
            make_coordinate_transform(component_spec.coordinate_transform),
            make_value_transform(component_spec.value_transform),
        });
    }

    std::shared_ptr<CompositionFunction> composition = make_composition(
        built_spec.composition_spec,
        built_spec.component_specs.size());
    const int dimension = built_spec.dimension;
    const double bias = built_spec.known_global_value;
    const double penalty = std::numeric_limits<double>::infinity();

    return [components, composition, dimension, bias, penalty](const std::vector<std::vector<double>>& X) {
        std::vector<double> values;
        values.reserve(X.size());
        for (const auto& x : X) {
            require_dimension(x, dimension, "benchmark function input");
            std::vector<double> component_values;
            component_values.reserve(components->size());
            bool invalid = false;
            for (const auto& component : *components) {
                const auto transformed = component.coordinate_transform->apply(x);
                const double raw_value = (*component.basic_function)(std::vector<std::vector<double>>{transformed}).front();
                if (!std::isfinite(raw_value)) {
                    invalid = true;
                    break;
                }
                const double scaled_value = component.basic_function->lambda * raw_value;
                const double transformed_value = component.value_transform->apply(scaled_value);
                if (!std::isfinite(transformed_value)) {
                    invalid = true;
                    break;
                }
                component_values.push_back(transformed_value);
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
            values.push_back(bias + composed);
        }
        return values;
    };
}

} // namespace FuncCraft
