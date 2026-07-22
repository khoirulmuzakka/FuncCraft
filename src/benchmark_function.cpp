#include "benchmark_function.h"
#include "support.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace FuncCraft {
namespace {

std::vector<double> dpm_component_center(const ComponentSpec& component) {
    detail::require(
        !component.coordinate_transform.assigned_xopt.empty(),
        "DPM component centers are obtained from component.coordinate_transform.assigned_xopt");
    return component.coordinate_transform.assigned_xopt;
}

std::shared_ptr<CompositionFunction> make_resolved_composition(const FunctionSpec& spec) {
    const CompositionSpec composition = spec.composition;
    if (detail::composition_mode(composition.kind) != CompositionMode::DPM) {
        return make_composition(composition, spec.components.size());
    }

    std::vector<std::vector<double>> centers;
    std::vector<double> biases;
    centers.reserve(spec.components.size());
    biases.reserve(spec.components.size());
    for (const ComponentSpec& component : spec.components) {
        centers.push_back(dpm_component_center(component));
        biases.push_back(component.f_bias);
    }

    if (composition.kind == CompositionKind::DpmSoftmax) {
        const double sharpness = composition.parameters.size() > 0 ? composition.parameters[0] : 0.01;
        return std::make_shared<DeceptiveSoftmaxComposition>(
            std::move(centers),
            std::move(biases),
            sharpness);
    }
    if (composition.kind == CompositionKind::DpmBgSoftmax) {
        const double sharpness = composition.parameters.size() > 0 ? composition.parameters[0] : 0.01;
        const double background_strength = composition.parameters.size() > 1 ? composition.parameters[1] : 1.0;
        const double background_sharpness = composition.parameters.size() > 2 ? composition.parameters[2] : 0.01;
        return std::make_shared<DeceptiveBgSoftmaxComposition>(
            std::move(centers),
            std::move(biases),
            sharpness,
            background_strength,
            background_sharpness);
    }
    throw std::logic_error("unhandled DPM composition kind");
}

std::vector<double> random_point_in_domain(std::mt19937_64& rng, const Domain& domain) {
    const int dimension = domain.dimension();
    std::vector<double> x(static_cast<std::size_t>(dimension), 0.0);
    for (int i = 0; i < dimension; ++i) {
        const double lo = domain.lower[static_cast<std::size_t>(i)];
        const double hi = domain.upper[static_cast<std::size_t>(i)];
        x[static_cast<std::size_t>(i)] = lo + (hi - lo) * detail::uniform01(rng);
    }
    return x;
}

double percentile(std::vector<double> values, double q) {
    if (values.empty()) {
        return 0.0;
    }
    if (q <= 0.0) {
        return *std::min_element(values.begin(), values.end());
    }
    if (q >= 1.0) {
        return *std::max_element(values.begin(), values.end());
    }
    const std::size_t index = static_cast<std::size_t>(q * static_cast<double>(values.size() - 1));
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(index), values.end());
    return values[index];
}

double estimate_lambda(const ComposedFunction& raw_function, const Domain& domain, std::uint64_t seed) {
    constexpr std::size_t kSampleCount = 100;
    constexpr double kTargetScale = 1.0e5;
    constexpr double kMinRepresentativeScale = 1.0e-12;
    constexpr double kMaxLambda = 1.0e8;

    std::mt19937_64 rng(detail::mix_seed(seed ^ 0xD1CEB00B5EEDULL));
    std::vector<std::vector<double>> batch;
    batch.reserve(kSampleCount);
    for (std::size_t i = 0; i < kSampleCount; ++i) {
        batch.push_back(random_point_in_domain(rng, domain));
    }

    std::vector<double> values = raw_function(batch);
    values.erase(
        std::remove_if(values.begin(), values.end(), [](double value) {
            return !std::isfinite(value);
        }),
        values.end());

    const double q = percentile(std::move(values), 0.25);
    if (!std::isfinite(q) || q <= kMinRepresentativeScale) {
        return 1.0;
    }
    return std::min(kTargetScale / q, kMaxLambda);
}

double saturating_apply_scale_and_bias(double raw_value, double lambda, double bias) {
    constexpr double kCap = 1.0e300;
    if (!std::isfinite(raw_value)) {
        return kCap;
    }
    double scaled = lambda * raw_value;
    if (!std::isfinite(scaled) || scaled > kCap) {
        scaled = kCap;
    } else if (scaled < -kCap) {
        scaled = -kCap;
    }
    double value = bias + scaled;
    if (!std::isfinite(value) || value > kCap) {
        return kCap;
    }
    if (value < -kCap) {
        return -kCap;
    }
    return value;
}

} // namespace

BenchmarkFunction::BenchmarkFunction(FunctionSpec spec) {
    FunctionSpec resolved_spec = std::move(spec);
    const Domain input_domain = make_domain(resolved_spec);

    for (ComponentSpec& component_spec : resolved_spec.components) {
        const int component_dimension = component_spec.component_dimension > 0
            ? component_spec.component_dimension
            : resolved_spec.dimension;
        component_spec.component_dimension = component_dimension;

        CoordinateTransformSpec& transform = component_spec.coordinate_transform;
        if (transform.dimension == 0) {
            transform.dimension = resolved_spec.dimension;
        }
        if (transform.assigned_xopt.empty()) {
            transform.assigned_xopt = !resolved_spec.assigned_xopt.empty()
                ? resolved_spec.assigned_xopt
                : std::vector<double>(static_cast<std::size_t>(resolved_spec.dimension), 0.0);
        }
    }

    FunctionBuilder builder(resolved_spec.dimension);
    builder.domain(input_domain);
    builder.seed(resolved_spec.seed);
    if (!resolved_spec.assigned_xopt.empty()) {
        builder.assigned_xopt(resolved_spec.assigned_xopt);
    }
    builder.assigned_fopt(resolved_spec.assigned_fopt);
    builder.scale_factor(resolved_spec.scale_factor);

    for (const ComponentSpec& component_spec : resolved_spec.components) {
        const BasicF primitive(component_spec.base_function, component_spec.component_dimension);
        const std::vector<double> target_xopt = detail::map_point_between_domains(
            primitive.x_opt,
            primitive.default_domain(),
            input_domain);
        builder.add_component(
            component_spec.base_function,
            component_spec.component_dimension,
            make_coordinate_transform(component_spec.coordinate_transform, target_xopt),
            make_value_transform(component_spec.value_transform),
            component_spec.f_bias);
    }
    builder.composition(make_resolved_composition(resolved_spec));

    const ComposedFunction raw_function = builder.build();
    spec_ = builder.build_spec();
    domain_ = make_domain(spec_);

    scale_factor_ = spec_.scale_factor.value_or(estimate_lambda(raw_function, domain_, spec_.seed));
    assigned_fopt_ = spec_.assigned_fopt;
    spec_.scale_factor = scale_factor_;
    spec_.assigned_fopt = assigned_fopt_;
    function_ = [raw_function, scale_factor = scale_factor_, assigned_fopt = assigned_fopt_](const std::vector<std::vector<double>>& X) {
        std::vector<double> values = raw_function(X);
        for (double& value : values) {
            value = saturating_apply_scale_and_bias(value, scale_factor, assigned_fopt);
        }
        return values;
    };
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

double BenchmarkFunction::scale_factor() const {
    return scale_factor_;
}

double BenchmarkFunction::assigned_fopt() const {
    return assigned_fopt_;
}

const FunctionSpec& BenchmarkFunction::spec() const {
    return spec_;
}

YAML::Node BenchmarkFunction::export_spec() const {
    YAML::Node node;
    node["format"] = "funccraft.benchmark_function";
    node["format_version"] = 1;
    node["function_spec"] = detail::function_spec_to_yaml(spec_);
    return node;
}

void BenchmarkFunction::export_spec(const std::string& path) const {
    detail::write_yaml_file(path, export_spec());
}

FunctionSpec load_function_spec_yaml(const std::string& path) {
    try {
        return detail::function_spec_from_yaml(YAML::LoadFile(path));
    } catch (const YAML::Exception& e) {
        throw std::invalid_argument(std::string("failed to load function spec from YAML: ") + e.what());
    }
}

BenchmarkFunction make_benchmark_function_from_yaml(const std::string& path) {
    return BenchmarkFunction(load_function_spec_yaml(path));
}

} // namespace FuncCraft
