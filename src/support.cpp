#include "benchmark_function.h"
#include "suite.h"
#include "support.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>

namespace FuncCraft {
namespace detail {

namespace {

double uniform_signed01(std::mt19937_64& rng) {
    return 2.0 * uniform01(rng) - 1.0;
}

double matrix_determinant(std::vector<std::vector<double>> matrix) {
    const std::size_t n = matrix.size();
    double det = 1.0;
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t pivot = i;
        double pivot_abs = std::fabs(matrix[i][i]);
        for (std::size_t r = i + 1; r < n; ++r) {
            const double candidate = std::fabs(matrix[r][i]);
            if (candidate > pivot_abs) {
                pivot_abs = candidate;
                pivot = r;
            }
        }
        if (pivot_abs < 1.0e-15) {
            return 0.0;
        }
        if (pivot != i) {
            std::swap(matrix[pivot], matrix[i]);
            det = -det;
        }
        const double pivot_value = matrix[i][i];
        det *= pivot_value;
        for (std::size_t r = i + 1; r < n; ++r) {
            const double factor = matrix[r][i] / pivot_value;
            for (std::size_t c = i; c < n; ++c) {
                matrix[r][c] -= factor * matrix[i][c];
            }
        }
    }
    return det;
}

} // namespace

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}

void require_dimension(const std::vector<double>& x, int dimension, const std::string& name) {
    require(static_cast<int>(x.size()) == dimension, name + " dimension mismatch");
}

void map_point_between_domains(
    const std::vector<double>& point,
    const Domain& source_domain,
    const Domain& target_domain,
    std::vector<double>& out) {
    require_dimension(point, source_domain.dimension(), "mapping point");
    require(source_domain.dimension() == target_domain.dimension(), "domain mapping dimension mismatch");
    out.assign(point.size(), 0.0);
    for (std::size_t i = 0; i < point.size(); ++i) {
        const double source_lo = source_domain.lower[i];
        const double source_hi = source_domain.upper[i];
        const double target_lo = target_domain.lower[i];
        const double target_hi = target_domain.upper[i];
        if (source_hi == source_lo) {
            out[i] = 0.5 * (target_lo + target_hi);
            continue;
        }
        const double t = (point[i] - source_lo) / (source_hi - source_lo);
        out[i] = target_lo + t * (target_hi - target_lo);
    }
}

std::vector<double> map_point_between_domains(
    const std::vector<double>& point,
    const Domain& source_domain,
    const Domain& target_domain) {
    std::vector<double> mapped;
    map_point_between_domains(point, source_domain, target_domain, mapped);
    return mapped;
}

double squared_distance(const std::vector<double>& a, const std::vector<double>& b) {
    require(a.size() == b.size(), "distance dimension mismatch");
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const double d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

double weighted_sum(const std::vector<double>& weights, const std::vector<double>& z) {
    require(weights.size() == z.size(), "weight/component size mismatch");
    double result = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        require(weights[i] >= 0.0, "weights must be nonnegative");
        result += weights[i] * z[i];
    }
    return result;
}

std::string join_base_ids(const std::vector<BasicFunctionId>& ids) {
    std::ostringstream out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            out << "+";
        }
        out << to_string(ids[i]);
    }
    return out.str();
}

std::uint64_t mix_seed(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

std::uint64_t indexed_seed(
    std::uint64_t seed,
    std::uint64_t role,
    std::uint64_t index0,
    std::uint64_t index1) {
    return mix_seed(
        seed
        ^ (role * 0x9E3779B97F4A7C15ULL)
        ^ ((index0 + 1ULL) * 0xBF58476D1CE4E5B9ULL)
        ^ ((index1 + 1ULL) * 0x94D049BB133111EBULL));
}

double uniform01(std::mt19937_64& rng) {
    return static_cast<double>(rng() >> 11) * 0x1.0p-53;
}

std::string normalize_spec_name(const std::string& value) {
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

CoordinateTransformKind parse_coordinate_transform_kind(const std::string& kind) {
    const std::string normalized = normalize_spec_name(kind);
    if (normalized.empty() || normalized == "none" || normalized == "identity") {
        return CoordinateTransformKind::None;
    }
    if (normalized == "rotation" || normalized == "rot") {
        return CoordinateTransformKind::Rotation;
    }
    if (normalized == "affine" || normalized == "aff") {
        return CoordinateTransformKind::Affine;
    }
    if (normalized == "blockrotation" || normalized == "blockrot" || normalized == "brot") {
        return CoordinateTransformKind::BlockRotation;
    }
    throw std::invalid_argument("unknown coordinate transform kind: " + kind);
}

ValueTransformKind parse_value_transform_kind(const std::string& kind) {
    const std::string normalized = normalize_spec_name(kind);
    if (normalized.empty() || normalized == "none" || normalized == "identity") {
        return ValueTransformKind::None;
    }
    if (normalized == "power") {
        return ValueTransformKind::Power;
    }
    if (normalized == "oscillatory" || normalized == "osc") {
        return ValueTransformKind::Oscillatory;
    }
    if (normalized == "cosinezero" || normalized == "coszero") {
        return ValueTransformKind::CosineZero;
    }
    throw std::invalid_argument("unknown value transform kind: " + kind);
}

CompositionKind parse_composition_kind(const std::string& kind) {
    const std::string normalized = normalize_spec_name(kind);
    if (normalized.empty() || normalized == "none" || normalized == "identity") {
        return CompositionKind::None;
    }
    if (normalized == "cpm" || normalized == "cpmwsum" || normalized == "weightedsum" || normalized == "sum" || normalized == "cpmsum") {
        return CompositionKind::CpmWeightedSum;
    }
    if (normalized == "cpmpowermean" || normalized == "powermean" || normalized == "cpmpmean") {
        return CompositionKind::CpmPowerMean;
    }
    if (normalized == "cpmlevelwell" || normalized == "levelwell" || normalized == "lwell" || normalized == "cpmlwell") {
        return CompositionKind::CpmLevelWell;
    }
    if (normalized == "dpmsoftmax" || normalized == "dpm" || normalized == "softmax") {
        return CompositionKind::DpmSoftmax;
    }
    if (normalized == "dpmbgsoftmax" || normalized == "dpmbg" || normalized == "bgsoftmax") {
        return CompositionKind::DpmBgSoftmax;
    }
    throw std::invalid_argument("unknown composition kind: " + kind);
}

CompositionMode composition_mode(CompositionKind kind) {
    switch (kind) {
    case CompositionKind::None:
        return CompositionMode::None;
    case CompositionKind::CpmWeightedSum:
    case CompositionKind::CpmPowerMean:
    case CompositionKind::CpmLevelWell:
        return CompositionMode::CPM;
    case CompositionKind::DpmSoftmax:
    case CompositionKind::DpmBgSoftmax:
        return CompositionMode::DPM;
    }
    throw std::logic_error("unhandled composition kind");
}

std::string to_spec_name(CoordinateTransformKind kind) {
    switch (kind) {
    case CoordinateTransformKind::None:
        return spec_name::None;
    case CoordinateTransformKind::Rotation:
        return spec_name::CoordinateRotation;
    case CoordinateTransformKind::Affine:
        return spec_name::CoordinateAffine;
    case CoordinateTransformKind::BlockRotation:
        return spec_name::CoordinateBlockRotation;
    }
    throw std::logic_error("unhandled coordinate transform kind");
}

std::string to_spec_name(ValueTransformKind kind) {
    switch (kind) {
    case ValueTransformKind::None:
        return spec_name::None;
    case ValueTransformKind::Power:
        return spec_name::ValuePower;
    case ValueTransformKind::Oscillatory:
        return spec_name::ValueOscillatory;
    case ValueTransformKind::CosineZero:
        return spec_name::ValueCosineZero;
    }
    throw std::logic_error("unhandled value transform kind");
}

std::string to_spec_name(CompositionKind kind) {
    switch (kind) {
    case CompositionKind::None:
        return spec_name::None;
    case CompositionKind::CpmWeightedSum:
        return spec_name::CpmWeightedSum;
    case CompositionKind::CpmPowerMean:
        return spec_name::CpmPowerMean;
    case CompositionKind::CpmLevelWell:
        return spec_name::CpmLevelWell;
    case CompositionKind::DpmSoftmax:
        return spec_name::DpmSoftmax;
    case CompositionKind::DpmBgSoftmax:
        return spec_name::DpmBgSoftmax;
    }
    throw std::logic_error("unhandled composition kind");
}

std::string canonical_coordinate_transform_kind(const std::string& kind) {
    return to_spec_name(parse_coordinate_transform_kind(kind));
}

std::string canonical_value_transform_kind(const std::string& kind) {
    return to_spec_name(parse_value_transform_kind(kind));
}

std::string canonical_composition_kind(const std::string& kind) {
    return to_spec_name(parse_composition_kind(kind));
}

int uniform_int(std::mt19937_64& rng, int lo, int hi) {
    require(lo <= hi, "uniform_int lower bound must not exceed upper bound");
    const auto span = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(hi) - static_cast<std::int64_t>(lo) + 1);
    const std::uint64_t limit = std::numeric_limits<std::uint64_t>::max()
        - (std::numeric_limits<std::uint64_t>::max() % span);

    std::uint64_t value = rng();
    while (value >= limit) {
        value = rng();
    }
    return lo + static_cast<int>(value % span);
}

double normal01(std::mt19937_64& rng) {
    double u1 = uniform01(rng);
    while (u1 <= 0.0) {
        u1 = uniform01(rng);
    }
    const double u2 = uniform01(rng);
    const double r = std::sqrt(-2.0 * std::log(u1));
    const double theta = 2.0 * kPi * u2;
    return r * std::cos(theta);
}

Domain centered_scaled_domain(const Domain& domain, double factor) {
    require(factor > 0.0 && factor <= 1.0, "domain scale factor must be in (0, 1]");
    Domain scaled = domain;
    for (int i = 0; i < domain.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        const double center = 0.5 * (domain.lower[idx] + domain.upper[idx]);
        const double half_width = 0.5 * (domain.upper[idx] - domain.lower[idx]) * factor;
        scaled.lower[idx] = center - half_width;
        scaled.upper[idx] = center + half_width;
    }
    return scaled;
}

std::vector<std::vector<double>> prefix_stable_latin_hypercube_points_in_domain(
    std::uint64_t seed,
    std::uint64_t role,
    const Domain& domain,
    int count) {
    require(count >= 0, "latin hypercube sample count must be nonnegative");
    require(domain.dimension() > 0, "latin hypercube domain dimension must be positive");

    std::vector<std::vector<double>> points(
        static_cast<std::size_t>(count),
        std::vector<double>(static_cast<std::size_t>(domain.dimension()), 0.0));
    if (count == 0) {
        return points;
    }

    for (int d = 0; d < domain.dimension(); ++d) {
        std::vector<int> strata(static_cast<std::size_t>(count), 0);
        for (int i = 0; i < count; ++i) {
            strata[static_cast<std::size_t>(i)] = i;
        }

        std::mt19937_64 permutation_rng(indexed_seed(seed, role, static_cast<std::uint64_t>(d), 0xC0111A7EULL));
        stable_shuffle(strata, permutation_rng);

        const auto dim = static_cast<std::size_t>(d);
        const double lo = domain.lower[dim];
        const double hi = domain.upper[dim];
        for (int i = 0; i < count; ++i) {
            std::mt19937_64 jitter_rng(indexed_seed(
                seed,
                role,
                static_cast<std::uint64_t>(d),
                static_cast<std::uint64_t>(i)));
            const double t = (static_cast<double>(strata[static_cast<std::size_t>(i)]) + uniform01(jitter_rng))
                / static_cast<double>(count);
            points[static_cast<std::size_t>(i)][dim] = lo + (hi - lo) * t;
        }
    }

    return points;
}

std::vector<std::vector<double>> prefix_stable_latin_hypercube_centers(
    std::uint64_t seed,
    std::uint64_t role,
    const Domain& domain,
    int count,
    double shrink_factor) {
    return prefix_stable_latin_hypercube_points_in_domain(
        seed,
        role,
        centered_scaled_domain(domain, shrink_factor),
        count);
}

std::vector<double> complete_prefix_stable_point(
    std::uint64_t seed,
    std::uint64_t role,
    const std::vector<double>& prefix,
    const Domain& domain,
    double shrink_factor) {
    require(static_cast<int>(prefix.size()) <= domain.dimension(), "coordinate prefix is longer than the domain dimension");
    std::vector<double> point = prefix_stable_latin_hypercube_centers(
        seed,
        role,
        domain,
        1,
        shrink_factor).front();
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        point[i] = prefix[i];
    }
    return point;
}

std::vector<std::vector<double>> random_rotation_matrix(std::mt19937_64& rng, int dimension) {
    require(dimension > 0, "rotation matrix dimension must be positive");

    std::vector<std::vector<double>> q(
        static_cast<std::size_t>(dimension),
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0));
    std::vector<double> column(static_cast<std::size_t>(dimension), 0.0);

    for (int col = 0; col < dimension; ++col) {
        bool accepted = false;
        for (int attempt = 0; attempt < 128 && !accepted; ++attempt) {
            for (double& value : column) {
                value = uniform_signed01(rng);
            }
            for (int prev = 0; prev < col; ++prev) {
                double dot = 0.0;
                for (int row = 0; row < dimension; ++row) {
                    dot += column[static_cast<std::size_t>(row)] *
                        q[static_cast<std::size_t>(row)][static_cast<std::size_t>(prev)];
                }
                for (int row = 0; row < dimension; ++row) {
                    column[static_cast<std::size_t>(row)] -=
                        dot * q[static_cast<std::size_t>(row)][static_cast<std::size_t>(prev)];
                }
            }

            double norm_sq = 0.0;
            for (double value : column) {
                norm_sq += value * value;
            }
            if (norm_sq <= 1.0e-14) {
                continue;
            }

            const double inv_norm = 1.0 / std::sqrt(norm_sq);
            for (int row = 0; row < dimension; ++row) {
                q[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] =
                    column[static_cast<std::size_t>(row)] * inv_norm;
            }
            accepted = true;
        }
        require(accepted, "could not generate a stable rotation matrix");
    }

    if (matrix_determinant(q) < 0.0) {
        for (int row = 0; row < dimension; ++row) {
            q[static_cast<std::size_t>(row)][0] = -q[static_cast<std::size_t>(row)][0];
        }
    }
    return q;
}

std::vector<std::vector<double>> random_affine_matrix(std::mt19937_64& rng, int dimension) {
    auto matrix = random_rotation_matrix(rng, dimension);
    for (int row = 0; row < dimension; ++row) {
        const double exponent = dimension > 1
            ? static_cast<double>(row) / static_cast<double>(dimension - 1)
            : 0.0;
        const double scale = std::pow(10.0, 2.0 * exponent);
        for (int col = 0; col < dimension; ++col) {
            matrix[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] *= scale;
        }
    }
    return matrix;
}

std::vector<double> yaml_double_vector(const YAML::Node& node, const std::string& field) {
    std::vector<double> values;
    if (!node) {
        return values;
    }
    if (!node.IsSequence()) {
        throw std::invalid_argument(field + " must be a sequence");
    }
    values.reserve(node.size());
    for (const YAML::Node& item : node) {
        values.push_back(item.as<double>());
    }
    return values;
}

std::vector<int> yaml_int_vector(const YAML::Node& node, const std::string& field) {
    std::vector<int> values;
    if (!node) {
        return values;
    }
    if (!node.IsSequence()) {
        throw std::invalid_argument(field + " must be a sequence");
    }
    values.reserve(node.size());
    for (const YAML::Node& item : node) {
        values.push_back(item.as<int>());
    }
    return values;
}

std::vector<std::string> yaml_string_vector(const YAML::Node& node, const std::string& field) {
    std::vector<std::string> values;
    if (!node) {
        return values;
    }
    if (!node.IsSequence()) {
        throw std::invalid_argument(field + " must be a sequence");
    }
    values.reserve(node.size());
    for (const YAML::Node& item : node) {
        values.push_back(item.as<std::string>());
    }
    return values;
}

std::vector<std::vector<double>> yaml_double_matrix(const YAML::Node& node, const std::string& field) {
    std::vector<std::vector<double>> values;
    if (!node) {
        return values;
    }
    if (!node.IsSequence()) {
        throw std::invalid_argument(field + " must be a sequence");
    }
    values.reserve(node.size());
    for (const YAML::Node& row : node) {
        values.push_back(yaml_double_vector(row, field + " row"));
    }
    return values;
}

BasicFunctionId yaml_basic_function_id(const YAML::Node& node) {
    require(static_cast<bool>(node), "base_function is required");
    if (node.IsScalar()) {
        const std::string name = node.as<std::string>();
        const std::string normalized = normalize_spec_name(name);
        for (BasicFunctionId id : list_basic_functions()) {
            if (normalize_spec_name(to_string(id)) == normalized) {
                return id;
            }
        }
        try {
            return static_cast<BasicFunctionId>(node.as<int>());
        } catch (const YAML::Exception&) {
            throw std::invalid_argument("unknown base_function: " + name);
        }
    }
    throw std::invalid_argument("base_function must be a string or integer");
}

CoordinateTransformSpec coordinate_transform_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("coordinate_transform must be a map");
    }
    CoordinateTransformSpec spec;
    spec.kind = parse_coordinate_transform_kind(node["kind"] ? node["kind"].as<std::string>() : "");
    spec.dimension = node["dimension"] ? node["dimension"].as<int>() : 0;
    spec.seed = node["seed"] ? node["seed"].as<std::uint64_t>() : 0;
    spec.selected_indices = yaml_int_vector(node["selected_indices"], "selected_indices");
    spec.assigned_xopt = yaml_double_vector(node["assigned_xopt"], "assigned_xopt");
    spec.parameters = yaml_double_vector(node["parameters"], "transform parameters");
    spec.matrix = yaml_double_matrix(node["matrix"], "matrix");
    return spec;
}

ValueTransformSpec value_transform_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("value_transform must be a map");
    }
    ValueTransformSpec spec;
    spec.kind = parse_value_transform_kind(node["kind"] ? node["kind"].as<std::string>() : "");
    spec.seed = node["seed"] ? node["seed"].as<std::uint64_t>() : 0;
    spec.parameters = yaml_double_vector(node["parameters"], "value transform parameters");
    return spec;
}

CompositionSpec composition_spec_from_yaml(const YAML::Node& node) {
    if (!node || node.IsNull()) {
        return {};
    }
    if (!node.IsMap()) {
        throw std::invalid_argument("composition must be a map");
    }
    CompositionSpec spec;
    spec.kind = parse_composition_kind(node["kind"] ? node["kind"].as<std::string>() : "");
    spec.seed = node["seed"] ? node["seed"].as<std::uint64_t>() : 0;
    spec.parameters = yaml_double_vector(node["parameters"], "composition parameters");
    spec.biases = yaml_double_vector(node["biases"], "composition biases");
    return spec;
}

ComponentSpec component_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("component spec must be a map");
    }
    ComponentSpec spec;
    if (node["composed_function"]) {
        spec.composed_function = std::make_shared<FunctionSpec>(function_spec_from_yaml(node["composed_function"]));
    } else {
        require(static_cast<bool>(node["base_function"]), "basic component requires base_function");
        spec.base_function = yaml_basic_function_id(node["base_function"]);
    }
    spec.coordinate_transform = coordinate_transform_spec_from_yaml(node["coordinate_transform"]);
    spec.value_transform = value_transform_spec_from_yaml(node["value_transform"]);
    spec.seed = node["seed"] ? node["seed"].as<std::uint64_t>() : 0;
    return spec;
}

YAML::Node function_spec_to_yaml(const FunctionSpec& spec) {
    auto double_vector = [](const std::vector<double>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (double value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto int_vector = [](const std::vector<int>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (int value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto string_vector = [](const std::vector<std::string>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (const std::string& value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto double_matrix = [&double_vector](const std::vector<std::vector<double>>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const auto& row : values) {
            node.push_back(double_vector(row));
        }
        return node;
    };
    auto domain_spec = [&double_vector](const DomainSpec& domain) {
        YAML::Node node;
        node["dimension"] = domain.dimension;
        node["lower_bound"] = double_vector(domain.lower_bound);
        node["upper_bound"] = double_vector(domain.upper_bound);
        return node;
    };
    auto transform_spec = [&](const CoordinateTransformSpec& transform) {
        YAML::Node node;
        node["kind"] = to_spec_name(transform.kind);
        node["dimension"] = transform.dimension;
        node["seed"] = transform.seed;
        node["selected_indices"] = int_vector(transform.selected_indices);
        node["assigned_xopt"] = double_vector(transform.assigned_xopt);
        node["parameters"] = double_vector(transform.parameters);
        node["matrix"] = double_matrix(transform.matrix);
        return node;
    };
    auto value_transform_spec = [&double_vector](const ValueTransformSpec& transform) {
        YAML::Node node;
        node["kind"] = to_spec_name(transform.kind);
        node["seed"] = transform.seed;
        node["parameters"] = double_vector(transform.parameters);
        return node;
    };
    auto composition_spec = [&](const CompositionSpec& composition) {
        YAML::Node node;
        node["kind"] = to_spec_name(composition.kind);
        node["seed"] = composition.seed;
        node["parameters"] = double_vector(composition.parameters);
        if (!composition.biases.empty()) {
            node["biases"] = double_vector(composition.biases);
        }
        return node;
    };

    YAML::Node components(YAML::NodeType::Sequence);
    for (const ComponentSpec& component : spec.components) {
        YAML::Node node;
        if (component.composed_function) {
            node["composed_function"] = function_spec_to_yaml(*component.composed_function);
        } else {
            require(component.base_function.has_value(), "basic component requires base_function");
            node["base_function"] = to_string(*component.base_function);
        }
        node["coordinate_transform"] = transform_spec(component.coordinate_transform);
        node["value_transform"] = value_transform_spec(component.value_transform);
        node["seed"] = component.seed;
        components.push_back(node);
    }

    YAML::Node node;
    node["dimension"] = spec.dimension;
    node["domain"] = domain_spec(spec.domain);
    node["components"] = components;
    node["composition"] = composition_spec(spec.composition);
    node["assigned_xopt"] = double_vector(spec.assigned_xopt);
    node["assigned_fopt"] = spec.assigned_fopt;
    if (spec.scale_factor.has_value()) {
        node["scale_factor"] = *spec.scale_factor;
    }
    node["seed"] = spec.seed;
    node["label"] = spec.label;
    node["metadata"] = string_vector(spec.metadata);
    return node;
}

FunctionSpec function_spec_from_yaml(const YAML::Node& root) {
    const YAML::Node node = root["function_spec"] ? root["function_spec"] : root;
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("function spec YAML must be a map");
    }

    FunctionSpec spec;
    spec.dimension = node["dimension"] ? node["dimension"].as<int>() : 0;
    if (node["domain"]) {
        const YAML::Node domain = node["domain"];
        require(domain.IsMap(), "domain must be a map");
        spec.domain.dimension = domain["dimension"] ? domain["dimension"].as<int>() : spec.dimension;
        spec.domain.lower_bound = yaml_double_vector(domain["lower_bound"], "domain.lower_bound");
        spec.domain.upper_bound = yaml_double_vector(domain["upper_bound"], "domain.upper_bound");
    }
    if (node["components"]) {
        if (!node["components"].IsSequence()) {
            throw std::invalid_argument("components must be a sequence");
        }
        spec.components.reserve(node["components"].size());
        for (const YAML::Node& component : node["components"]) {
            spec.components.push_back(component_spec_from_yaml(component));
        }
    }
    spec.composition = composition_spec_from_yaml(node["composition"]);
    spec.assigned_xopt = yaml_double_vector(node["assigned_xopt"], "assigned_xopt");
    spec.assigned_fopt = node["assigned_fopt"] ? node["assigned_fopt"].as<double>() : 0.0;
    if (node["scale_factor"] && !node["scale_factor"].IsNull()) {
        spec.scale_factor = node["scale_factor"].as<double>();
    }
    if (root["runtime"] && root["runtime"]["scale_factor"] && !root["runtime"]["scale_factor"].IsNull()) {
        spec.scale_factor = root["runtime"]["scale_factor"].as<double>();
    }
    if (root["runtime"] && root["runtime"]["assigned_fopt"] && !root["runtime"]["assigned_fopt"].IsNull()) {
        spec.assigned_fopt = root["runtime"]["assigned_fopt"].as<double>();
    }
    spec.seed = node["seed"] ? node["seed"].as<std::uint64_t>() : 0;
    spec.label = node["label"] ? node["label"].as<std::string>() : "";
    spec.metadata = yaml_string_vector(node["metadata"], "metadata");
    return spec;
}

YAML::Node suite_spec_to_yaml(const SuiteSpec& spec) {
    auto base_function_vector = [](const std::vector<BasicFunctionId>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (BasicFunctionId value : values) {
            node.push_back(static_cast<int>(value));
        }
        return node;
    };
    auto double_vector = [](const std::vector<double>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (double value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto coordinate_choice_vector = [&double_vector](const std::vector<CoordinateTransformChoice>& choices) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const CoordinateTransformChoice& choice : choices) {
            YAML::Node item;
            item["kind"] = to_spec_name(choice.kind);
            item["probability"] = choice.probability;
            item["parameters"] = double_vector(choice.parameters);
            node.push_back(item);
        }
        return node;
    };
    auto value_choice_vector = [&double_vector](const std::vector<ValueTransformChoice>& choices) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const ValueTransformChoice& choice : choices) {
            YAML::Node item;
            item["kind"] = to_spec_name(choice.kind);
            item["probability"] = choice.probability;
            item["parameters"] = double_vector(choice.parameters);
            node.push_back(item);
        }
        return node;
    };
    auto composition_choice_vector = [&double_vector](const std::vector<CompositionChoice>& choices) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const CompositionChoice& choice : choices) {
            YAML::Node item;
            item["kind"] = to_spec_name(choice.kind);
            item["probability"] = choice.probability;
            item["parameters"] = double_vector(choice.parameters);
            node.push_back(item);
        }
        return node;
    };

    YAML::Node node;
    node["supported_dimensions"] = spec.supported_dimensions;
    node["base_functions"] = base_function_vector(spec.base_functions);
    node["composition_base_functions"] = base_function_vector(spec.composition_base_functions);
    node["coordinate_transforms"] = coordinate_choice_vector(spec.coordinate_transforms);
    node["value_transforms"] = value_choice_vector(spec.value_transforms);
    node["compositions"] = composition_choice_vector(spec.compositions);
    node["min_components"] = spec.min_components;
    node["max_components"] = spec.max_components;
    node["max_nested_composition_depth"] = spec.max_nested_composition_depth;
    node["nested_probability"] = spec.nested_probability;
    node["requested_number_of_functions"] = spec.requested_number_of_functions;
    node["max_number_of_functions"] = spec.max_number_of_functions;
    node["master_seed"] = spec.master_seed;
    node["lower_bound"] = spec.lower_bound;
    node["upper_bound"] = spec.upper_bound;
    node["assigned_fopt"] = spec.assigned_fopt;
    node["xopt_domain_shrink_factor"] = spec.xopt_domain_shrink_factor;
    node["suite_label"] = spec.suite_label;
    return node;
}

void write_yaml_file(const std::string& path, const YAML::Node& node) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open YAML output file: " + path);
    }
    out << node;
    out << '\n';
}

} // namespace detail
} // namespace FuncCraft
