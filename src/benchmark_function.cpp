#include "benchmark_function.h"
#include "builder.h"

#include <utility>

namespace FuncCraft {

BenchmarkFunction::BenchmarkFunction(FunctionSpec spec) {
    FunctionBuilder builder(spec.dimension);
    if (!spec.lower_bound.empty() || !spec.upper_bound.empty()) {
        builder.domain(make_domain(spec));
    }
    builder.seed(spec.seed);
    if (!spec.known_global_minimizer.empty()) {
        builder.known_global_minimizer(spec.known_global_minimizer);
    }
    builder.known_global_value(spec.known_global_value);
    for (const std::string& parameter : spec.parameters) {
        const std::size_t pos = parameter.find('=');
        if (pos != std::string::npos) {
            builder.parameter(parameter.substr(0, pos), parameter.substr(pos + 1));
        }
    }
    for (const ComponentSpec& component_spec : spec.component_specs) {
        builder.add_component(
            parse_basic_function_id(component_spec.base_function),
            component_spec.component_dimension,
            make_coordinate_transform(component_spec.coordinate_transform),
            make_value_transform(component_spec.value_transform));
    }
    builder.composition(make_composition(spec.composition_spec, spec.component_specs.size()));

    spec_ = builder.build_spec();
    domain_ = make_domain(spec_);
    function_ = builder.build();
}

std::vector<double> BenchmarkFunction::operator()(const std::vector<std::vector<double>>& X) const {
    return function_(X);
}

const Domain& BenchmarkFunction::domain() const {
    return domain_;
}

int BenchmarkFunction::dimension() const {
    return domain_.dimension();
}

const FunctionSpec& BenchmarkFunction::spec() const {
    return spec_;
}

} // namespace FuncCraft
