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
    constexpr std::size_t kTargetSamples = 100;
    constexpr std::size_t kBatchSize = 64;
    constexpr std::size_t kMaxAttempts = kTargetSamples * 8;
    constexpr double kTargetScale = 1.0e5;
    constexpr double kMinRepresentativeScale = 1.0e-12;
    constexpr double kMaxLambda = 1.0e8;

    std::mt19937_64 rng(detail::mix_seed(seed ^ 0xD1CEB00B5EEDULL));
    std::vector<double> values;
    values.reserve(kTargetSamples);

    std::vector<std::vector<double>> batch;
    batch.reserve(kBatchSize);

    std::size_t attempts = 0;
    while (values.size() < kTargetSamples && attempts < kMaxAttempts) {
        batch.clear();
        const std::size_t remaining = kTargetSamples - values.size();
        const std::size_t batch_count = std::min<std::size_t>(kBatchSize, remaining);
        for (std::size_t i = 0; i < batch_count && attempts < kMaxAttempts; ++i, ++attempts) {
            batch.push_back(random_point_in_domain(rng, domain));
        }
        if (batch.empty()) {
            break;
        }

        const std::vector<double> raw_values = raw_function(batch);
        for (double value : raw_values) {
            if (std::isfinite(value)) {
                values.push_back(value);
            }
        }
    }

    if (values.empty()) {
        return 1.0;
    }

    const double q90 = percentile(std::move(values), 0.9);
    if (!std::isfinite(q90) || q90 <= kMinRepresentativeScale) {
        return 1.0;
    }
    return std::min(kTargetScale / q90, kMaxLambda);
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

    const ComposedFunction raw_function = builder.build();
    lambda_ = estimate_lambda(raw_function, domain_, static_cast<std::uint64_t>(spec_.seed));
    bias_ = spec_.known_global_value;
    function_ = [raw_function, lambda = lambda_, bias = bias_](const std::vector<std::vector<double>>& X) {
        std::vector<double> values = raw_function(X);
        for (double& value : values) {
            value = saturating_apply_scale_and_bias(value, lambda, bias);
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

double BenchmarkFunction::lambda() const {
    return lambda_;
}

double BenchmarkFunction::bias() const {
    return bias_;
}

const FunctionSpec& BenchmarkFunction::spec() const {
    return spec_;
}

YAML::Node BenchmarkFunction::export_spec() const {
    YAML::Node node;
    node["format"] = "funccraft.benchmark_function";
    node["format_version"] = 1;
    node["function_spec"] = detail::function_spec_to_yaml(spec_);
    node["runtime"]["lambda"] = lambda_;
    node["runtime"]["bias"] = bias_;
    return node;
}

void BenchmarkFunction::export_spec(const std::string& path) const {
    detail::write_yaml_file(path, export_spec());
}

} // namespace FuncCraft
