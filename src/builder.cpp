#include "builder.h"
#include "support.h"

#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

std::shared_ptr<ValueTransform> make_value_transform(ValueTransformClass cls, double a, double b) {
    switch (cls) {
    case ValueTransformClass::None:
        return std::make_shared<IdentityValueTransform>();
    case ValueTransformClass::CosineZero:
        return std::make_shared<CosineZeroValueTransform>(a);
    case ValueTransformClass::Oscillatory:
        return std::make_shared<OscillatoryValueTransform>(a, b);
    case ValueTransformClass::Power:
        return std::make_shared<PowerValueTransform>(a, b);
    default:
        throw std::invalid_argument("unknown value transform class");
    }
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

    Component component;
    component.base = std::make_shared<BasicFunction>(id, component_dimension);
    component.coordinate_transform = std::move(coordinate_transform);
    component.value_transform = std::move(value_transform);
    components_.push_back(std::move(component));

    function_class_.base_functions.push_back(id);
    const CoordinateTransformClass t_class = components_.back().coordinate_transform->transform_class();
    const ValueTransformClass p_class = components_.back().value_transform->transform_class();
    if (components_.size() == 1) {
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

ComposedFunction FunctionBuilder::build() const {
    require(!components_.empty(), "cannot build function without components");
    std::shared_ptr<CompositionFunction> composition = composition_;
    FunctionClass cls = function_class_;
    if (!composition) {
        composition = components_.size() == 1
            ? std::shared_ptr<CompositionFunction>(std::make_shared<SingleComponentComposition>())
            : make_weighted_sum(components_.size());
        cls.composition = composition->composition_class();
    }

    FunctionMetadata metadata;
    metadata.function_class = cls;
    metadata.dimension = domain_.dimension();
    metadata.components = static_cast<int>(components_.size());
    metadata.seed = seed_;
    metadata.known_global_minimizer = x_star_;
    metadata.known_global_value = f_star_;
    metadata.parameters = parameters_;
    metadata.parameters.emplace("class_label", class_label(cls));

    return ComposedFunction(domain_, components_, composition, std::move(metadata));
}

} // namespace FuncCraft
