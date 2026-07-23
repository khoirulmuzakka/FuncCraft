#include "benchmark_function.h"
#include "support.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <utility>

namespace FuncCraft {
namespace {

constexpr double kDefaultGeneratedXoptShrinkFactor = 0.8;

std::vector<double> dpm_component_center(const ComponentSpec& component) {
    detail::require(
        !component.coordinate_transform.assigned_xopt.empty(),
        "DPM component centers are obtained from component.coordinate_transform.assigned_xopt");
    return component.coordinate_transform.assigned_xopt;
}

std::vector<std::vector<double>> identity_matrix(int dimension) {
    std::vector<std::vector<double>> matrix(
        static_cast<std::size_t>(dimension),
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0));
    for (int i = 0; i < dimension; ++i) {
        matrix[static_cast<std::size_t>(i)][static_cast<std::size_t>(i)] = 1.0;
    }
    return matrix;
}

std::vector<double> merge_coordinate_prefix(
    const std::vector<double>& prefix,
    std::vector<double> generated,
    const std::string& name) {
    detail::require(prefix.size() <= generated.size(), name + " prefix is longer than generated point");
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        generated[i] = prefix[i];
    }
    return generated;
}

std::shared_ptr<CoordinateTransform> make_coordinate_transform(
    const CoordinateTransformSpec& spec,
    const std::vector<double>& target_xopt) {
    const CoordinateTransformKind kind = spec.kind;
    detail::require(spec.dimension > 0, "coordinate transform dimension must be positive");
    detail::require(static_cast<int>(spec.assigned_xopt.size()) == spec.dimension, "coordinate transform assigned_xopt dimension mismatch");
    detail::require(static_cast<int>(target_xopt.size()) == spec.dimension, "coordinate transform target_xopt dimension mismatch");

    const std::uint64_t seed = spec.seed;
    if (kind == CoordinateTransformKind::None) {
        return std::make_shared<IdentityTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::Rotation) {
        if (!spec.matrix.empty()) {
            return std::make_shared<RotationTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed, spec.matrix);
        }
        return std::make_shared<RotationTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::Affine) {
        if (!spec.matrix.empty()) {
            return std::make_shared<AffineTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed, spec.matrix);
        }
        return std::make_shared<AffineTransform>(spec.dimension, spec.assigned_xopt, target_xopt, seed);
    }
    if (kind == CoordinateTransformKind::BlockRotation) {
        detail::require(!spec.selected_indices.empty(), "block rotation transform needs selected indices");
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
    if (spec.kind == ValueTransformKind::None) {
        return std::make_shared<IdentityValueTransform>();
    }
    if (spec.kind == ValueTransformKind::Power) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        const double p = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<PowerValueTransform>(alpha, p);
    }
    if (spec.kind == ValueTransformKind::Oscillatory) {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        return std::make_shared<OscillatoryValueTransform>(epsilon, alpha);
    }
    if (spec.kind == ValueTransformKind::CosineZero) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        return std::make_shared<CosineZeroValueTransform>(alpha);
    }
    throw std::logic_error("unhandled value transform kind");
}

CoordinateTransformSpec materialized_coordinate_transform_spec(
    const CoordinateTransformSpec& requested,
    const CoordinateTransform& transform) {
    CoordinateTransformSpec spec = requested;
    spec.dimension = transform.input_dimension();
    spec.assigned_xopt = transform.assigned_xopt();
    spec.seed = transform.seed();
    if (spec.kind == CoordinateTransformKind::None) {
        spec.matrix = identity_matrix(spec.dimension);
        return spec;
    }
    if (spec.kind == CoordinateTransformKind::Rotation) {
        const auto* rotation = dynamic_cast<const RotationTransform*>(&transform);
        detail::require(rotation != nullptr, "rotation transform materialization mismatch");
        spec.matrix = rotation->matrix();
        return spec;
    }
    if (spec.kind == CoordinateTransformKind::Affine) {
        const auto* affine = dynamic_cast<const AffineTransform*>(&transform);
        detail::require(affine != nullptr, "affine transform materialization mismatch");
        spec.matrix = affine->matrix();
        return spec;
    }
    if (spec.kind == CoordinateTransformKind::BlockRotation) {
        const auto* block = dynamic_cast<const BlockRotationTransform*>(&transform);
        detail::require(block != nullptr, "block rotation transform materialization mismatch");
        spec.selected_indices = block->selected_indices();
        spec.matrix = block->matrix();
        return spec;
    }
    throw std::logic_error("unhandled coordinate transform kind");
}

ValueTransformSpec materialized_value_transform_spec(const ValueTransformSpec& requested) {
    ValueTransformSpec spec = requested;
    if (spec.kind == ValueTransformKind::None) {
        spec.parameters.clear();
        return spec;
    }
    if (spec.kind == ValueTransformKind::Power) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        const double p = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        spec.parameters = {alpha, p};
        return spec;
    }
    if (spec.kind == ValueTransformKind::Oscillatory) {
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        spec.parameters = {epsilon, alpha};
        return spec;
    }
    if (spec.kind == ValueTransformKind::CosineZero) {
        const double alpha = spec.parameters.size() > 0 ? spec.parameters[0] : 1.0;
        spec.parameters = {alpha};
        return spec;
    }
    throw std::logic_error("unhandled value transform kind");
}

CompositionSpec materialized_composition_spec(const CompositionSpec& requested, std::size_t component_count) {
    CompositionSpec spec = requested;
    if (spec.kind == CompositionKind::None) {
        detail::require(component_count == 1, "no composition requires exactly one component");
        detail::require(spec.biases.empty(), "composition biases are only valid for DPM compositions");
        spec.parameters.clear();
        return spec;
    }
    if (spec.kind == CompositionKind::CpmWeightedSum) {
        detail::require(spec.biases.empty(), "composition biases are only valid for DPM compositions");
        spec.parameters.clear();
        return spec;
    }
    if (spec.kind == CompositionKind::CpmPowerMean) {
        detail::require(spec.biases.empty(), "composition biases are only valid for DPM compositions");
        const double p = spec.parameters.empty() ? 2.0 : spec.parameters[0];
        spec.parameters = {p};
        return spec;
    }
    if (spec.kind == CompositionKind::CpmLevelWell) {
        detail::require(spec.biases.empty(), "composition biases are only valid for DPM compositions");
        const double epsilon = spec.parameters.size() > 0 ? spec.parameters[0] : 0.1;
        const double alpha = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        spec.parameters = {epsilon, alpha};
        return spec;
    }
    if (spec.kind == CompositionKind::DpmSoftmax) {
        detail::require(spec.biases.empty() || spec.biases.size() == component_count, "DPM composition bias/component size mismatch");
        const double sharpness = spec.parameters.empty() ? 0.01 : spec.parameters[0];
        spec.parameters = {sharpness};
        return spec;
    }
    if (spec.kind == CompositionKind::DpmBgSoftmax) {
        detail::require(spec.biases.empty() || spec.biases.size() == component_count, "DPM composition bias/component size mismatch");
        const double sharpness = spec.parameters.size() > 0 ? spec.parameters[0] : 0.01;
        const double background_strength = spec.parameters.size() > 1 ? spec.parameters[1] : 1.0;
        const double background_sharpness = spec.parameters.size() > 2 ? spec.parameters[2] : 0.01;
        spec.parameters = {sharpness, background_strength, background_sharpness};
        return spec;
    }
    throw std::logic_error("unhandled composition kind");
}

std::shared_ptr<CompositionFunction> make_common_point_composition(const CompositionSpec& composition, std::size_t component_count) {
    detail::require(composition.biases.empty(), "composition biases are only valid for DPM compositions");
    if (composition.kind == CompositionKind::None) {
        detail::require(component_count == 1, "no composition requires exactly one component");
        return std::make_shared<NoneComposition>();
    }
    if (composition.kind == CompositionKind::CpmWeightedSum) {
        return std::make_shared<WeightedSumComposition>(component_count);
    }
    if (composition.kind == CompositionKind::CpmPowerMean) {
        const double p = composition.parameters.empty() ? 2.0 : composition.parameters[0];
        return std::make_shared<PowerMeanComposition>(component_count, p);
    }
    if (composition.kind == CompositionKind::CpmLevelWell) {
        const double epsilon = composition.parameters.size() > 0 ? composition.parameters[0] : 0.1;
        const double alpha = composition.parameters.size() > 1 ? composition.parameters[1] : 1.0;
        return std::make_shared<LevelWellComposition>(component_count, epsilon, alpha);
    }
    throw std::logic_error("unhandled common-point composition kind");
}

std::shared_ptr<CompositionFunction> make_resolved_composition(const FunctionSpec& spec) {
    const CompositionSpec composition = spec.composition;
    if (detail::composition_mode(composition.kind) != CompositionMode::DPM) {
        return make_common_point_composition(composition, spec.components.size());
    }
    detail::require(
        composition.biases.empty() || composition.biases.size() == spec.components.size(),
        "DPM composition bias/component size mismatch");

    std::vector<std::vector<double>> centers;
    centers.reserve(spec.components.size());
    for (const ComponentSpec& component : spec.components) {
        centers.push_back(dpm_component_center(component));
    }

    if (composition.kind == CompositionKind::DpmSoftmax) {
        const double sharpness = composition.parameters.size() > 0 ? composition.parameters[0] : 0.01;
        return std::make_shared<DeceptiveSoftmaxComposition>(
            std::move(centers),
            sharpness,
            composition.biases);
    }
    if (composition.kind == CompositionKind::DpmBgSoftmax) {
        const double sharpness = composition.parameters.size() > 0 ? composition.parameters[0] : 0.01;
        const double background_strength = composition.parameters.size() > 1 ? composition.parameters[1] : 1.0;
        const double background_sharpness = composition.parameters.size() > 2 ? composition.parameters[2] : 0.01;
        return std::make_shared<DeceptiveBgSoftmaxComposition>(
            std::move(centers),
            sharpness,
            background_strength,
            background_sharpness,
            composition.biases);
    }
    throw std::logic_error("unhandled DPM composition kind");
}

Domain make_domain_from_spec(const FunctionSpec& spec) {
    detail::require(spec.dimension > 0, "function dimension must be positive");

    Domain domain(spec.dimension);
    if (!spec.domain.lower_bound.empty() || !spec.domain.upper_bound.empty()) {
        detail::require(static_cast<int>(spec.domain.lower_bound.size()) == spec.dimension, "lower bound dimension mismatch");
        detail::require(static_cast<int>(spec.domain.upper_bound.size()) == spec.dimension, "upper bound dimension mismatch");
        domain.lower = spec.domain.lower_bound;
        domain.upper = spec.domain.upper_bound;
    }
    detail::require(domain.lower.size() == domain.upper.size(), "domain bound size mismatch");
    for (std::size_t i = 0; i < domain.lower.size(); ++i) {
        detail::require(domain.lower[i] <= domain.upper[i], "domain lower bound must not exceed upper bound");
    }
    return domain;
}

void append_leaf_base_functions(const ComponentSpec& component, std::vector<BasicFunctionId>& ids) {
    if (component.composed_function) {
        for (const ComponentSpec& child : component.composed_function->components) {
            append_leaf_base_functions(child, ids);
        }
        return;
    }
    detail::require(component.base_function.has_value(), "basic component requires base_function");
    ids.push_back(*component.base_function);
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

int coordinate_transform_output_dimension(const CoordinateTransformSpec& transform) {
    detail::require(transform.dimension > 0, "coordinate transform dimension must be positive");
    return transform.dimension;
}

BenchmarkFunction::BenchmarkFunction(FunctionSpec spec) {
    FunctionSpec resolved_spec = std::move(spec);
    const Domain input_domain = make_domain_from_spec(resolved_spec);
    detail::require(!resolved_spec.components.empty(), "benchmark function needs at least one component");

    resolved_spec.assigned_xopt = detail::complete_prefix_stable_point(
        resolved_spec.seed,
        detail::kAssignedXoptSeedRole,
        resolved_spec.assigned_xopt,
        input_domain,
        kDefaultGeneratedXoptShrinkFactor);

    const bool dpm_mode = detail::composition_mode(resolved_spec.composition.kind) == CompositionMode::DPM;
    std::vector<std::vector<double>> generated_dpm_centers;
    if (dpm_mode && resolved_spec.components.size() > 1) {
        generated_dpm_centers = detail::prefix_stable_latin_hypercube_centers(
            resolved_spec.seed,
            detail::kDpmCenterSeedRole,
            input_domain,
            static_cast<int>(resolved_spec.components.size()) - 1,
            kDefaultGeneratedXoptShrinkFactor);
    }

    for (std::size_t component_index = 0; component_index < resolved_spec.components.size(); ++component_index) {
        ComponentSpec& component_spec = resolved_spec.components[component_index];
        CoordinateTransformSpec& transform = component_spec.coordinate_transform;
        if (transform.dimension == 0) {
            transform.dimension = resolved_spec.dimension;
        }
        detail::require(transform.dimension == resolved_spec.dimension, "coordinate transform dimension must match function dimension");
        detail::require(
            static_cast<int>(transform.assigned_xopt.size()) <= transform.dimension,
            "coordinate transform assigned_xopt prefix is longer than the transform dimension");
        if (dpm_mode && component_index > 0) {
            transform.assigned_xopt = merge_coordinate_prefix(
                transform.assigned_xopt,
                generated_dpm_centers[component_index - 1],
                "DPM component assigned_xopt");
        } else if (transform.assigned_xopt.empty()) {
            transform.assigned_xopt = resolved_spec.assigned_xopt;
        } else if (static_cast<int>(transform.assigned_xopt.size()) < transform.dimension) {
            transform.assigned_xopt = detail::complete_prefix_stable_point(
                component_spec.seed != 0 ? component_spec.seed : transform.seed,
                detail::kAssignedXoptSeedRole,
                transform.assigned_xopt,
                input_domain,
                kDefaultGeneratedXoptShrinkFactor);
        }
        const int component_dimension = coordinate_transform_output_dimension(transform);
        if (component_spec.composed_function) {
            detail::require(
                component_spec.composed_function->dimension == component_dimension,
                "composed component dimension must match coordinate transform output dimension");
        }
    }

    resolved_spec.composition = materialized_composition_spec(resolved_spec.composition, resolved_spec.components.size());
    std::shared_ptr<CompositionFunction> composition = make_resolved_composition(resolved_spec);
    FunctionBuilder builder(resolved_spec.dimension);
    builder.domain(input_domain).composition(composition);

    FunctionClass function_class;
    function_class.composition = composition->composition_class();

    for (ComponentSpec& component_spec : resolved_spec.components) {
        Domain child_domain;
        std::vector<double> child_xopt;
        double child_fopt = 0.0;
        std::function<std::vector<double>(const std::vector<std::vector<double>>&)> child_eval;

        if (component_spec.composed_function) {
            auto child = std::make_shared<BenchmarkFunction>(*component_spec.composed_function);
            component_spec.composed_function = std::make_shared<FunctionSpec>(child->spec());
            child_domain = child->domain();
            child_xopt = child->spec().assigned_xopt;
            child_fopt = child->assigned_fopt();
            child_eval = [child](const std::vector<std::vector<double>>& X) {
                return (*child)(X);
            };
        } else {
            detail::require(component_spec.base_function.has_value(), "basic component requires base_function");
            const int component_dimension = coordinate_transform_output_dimension(component_spec.coordinate_transform);
            auto primitive = std::make_shared<BasicF>(*component_spec.base_function, component_dimension);
            child_domain = primitive->default_domain();
            child_xopt = primitive->x_opt;
            child_fopt = primitive->f_opt;
            child_eval = [primitive](const std::vector<std::vector<double>>& X) {
                return (*primitive)(X);
            };
        }

        const std::vector<double> target_xopt = detail::map_point_between_domains(
            child_xopt,
            child_domain,
            input_domain);
        auto coordinate_transform = make_coordinate_transform(component_spec.coordinate_transform, target_xopt);
        auto value_transform = make_value_transform(component_spec.value_transform);
        const CoordinateTransformClass t_class = coordinate_transform->transform_class();
        const ValueTransformClass v_class = value_transform->transform_class();
        component_spec.coordinate_transform = materialized_coordinate_transform_spec(
            component_spec.coordinate_transform,
            *coordinate_transform);
        component_spec.value_transform = materialized_value_transform_spec(component_spec.value_transform);

        builder.add_component(
            std::move(child_eval),
            std::move(child_domain),
            std::move(child_xopt),
            child_fopt,
            std::move(coordinate_transform),
            std::move(value_transform));

        if (function_class.base_functions.empty()) {
            function_class.coordinate_transform = t_class;
            function_class.value_transform = v_class;
        } else {
            if (function_class.coordinate_transform != t_class) {
                function_class.coordinate_transform = CoordinateTransformClass::Mixed;
            }
            if (function_class.value_transform != v_class) {
                function_class.value_transform = ValueTransformClass::Mixed;
            }
        }
        append_leaf_base_functions(component_spec, function_class.base_functions);
    }

    const ComposedFunction raw_function = builder.build();
    if (resolved_spec.label.empty()) {
        resolved_spec.label = class_label(function_class);
    }
    spec_ = std::move(resolved_spec);
    domain_ = input_domain;

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

FunctionSpec load_function_spec(const std::string& path) {
    try {
        return detail::function_spec_from_yaml(YAML::LoadFile(path));
    } catch (const YAML::Exception& e) {
        throw std::invalid_argument(std::string("failed to load function spec: ") + e.what());
    }
}

BenchmarkFunction make_benchmark_function(const std::string& path) {
    return BenchmarkFunction(load_function_spec(path));
}

} // namespace FuncCraft
