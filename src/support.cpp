#include "support.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
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

void map_point_to_default_domain(const std::vector<double>& point, const Domain& domain, std::vector<double>& out) {
    require_dimension(point, domain.dimension(), "mapping point");
    constexpr double kDefaultDomainLower = -5.0;
    constexpr double kDefaultDomainUpper = 5.0;

    out.assign(point.size(), 0.0);
    for (std::size_t i = 0; i < point.size(); ++i) {
        const double lo = domain.lower[i];
        const double hi = domain.upper[i];
        if (hi == lo) {
            out[i] = 0.0;
            continue;
        }
        const double t = (point[i] - lo) / (hi - lo);
        out[i] = kDefaultDomainLower + t * (kDefaultDomainUpper - kDefaultDomainLower);
    }
}

std::vector<double> map_point_from_default_domain(const std::vector<double>& point, const Domain& domain) {
    require_dimension(point, domain.dimension(), "inverse mapping point");
    constexpr double kDefaultDomainLower = -5.0;
    constexpr double kDefaultDomainUpper = 5.0;

    std::vector<double> mapped(point.size(), 0.0);
    for (std::size_t i = 0; i < point.size(); ++i) {
        const double lo = domain.lower[i];
        const double hi = domain.upper[i];
        if (hi == lo) {
            mapped[i] = lo;
            continue;
        }
        const double t = (point[i] - kDefaultDomainLower) / (kDefaultDomainUpper - kDefaultDomainLower);
        mapped[i] = lo + t * (hi - lo);
    }
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

double uniform01(std::mt19937_64& rng) {
    return std::generate_canonical<double, 53>(rng);
}

int uniform_int(std::mt19937_64& rng, int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
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

TransformSpec transform_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("coordinate_transform must be a map");
    }
    TransformSpec spec;
    spec.kind = node["kind"] ? node["kind"].as<std::string>() : "";
    spec.dimension = node["dimension"] ? node["dimension"].as<int>() : 0;
    spec.seed = node["seed"] ? node["seed"].as<int>() : 0;
    spec.selected_indices = yaml_int_vector(node["selected_indices"], "selected_indices");
    spec.source_point = yaml_double_vector(node["source_point"], "source_point");
    spec.target_point = yaml_double_vector(node["target_point"], "target_point");
    spec.parameters = yaml_double_vector(node["parameters"], "transform parameters");
    spec.matrix = yaml_double_matrix(node["matrix"], "matrix");
    return spec;
}

ValueTransformSpec value_transform_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("value_transform must be a map");
    }
    ValueTransformSpec spec;
    spec.kind = node["kind"] ? node["kind"].as<std::string>() : "";
    spec.seed = node["seed"] ? node["seed"].as<int>() : 0;
    spec.parameters = yaml_double_vector(node["parameters"], "value transform parameters");
    return spec;
}

CompositionSpec composition_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("composition_spec must be a map");
    }
    CompositionSpec spec;
    spec.kind = node["kind"] ? node["kind"].as<std::string>() : "";
    spec.seed = node["seed"] ? node["seed"].as<int>() : 0;
    spec.parameters = yaml_double_vector(node["parameters"], "composition parameters");
    spec.centers = yaml_double_matrix(node["centers"], "centers");
    spec.offsets = yaml_double_vector(node["offsets"], "offsets");
    spec.weights = yaml_double_vector(node["weights"], "weights");
    return spec;
}

ComponentSpec component_spec_from_yaml(const YAML::Node& node) {
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("component spec must be a map");
    }
    ComponentSpec spec;
    spec.base_function = node["base_function"] ? node["base_function"].as<std::string>() : "";
    spec.component_dimension = node["component_dimension"] ? node["component_dimension"].as<int>() : 0;
    spec.coordinate_transform = transform_spec_from_yaml(node["coordinate_transform"]);
    spec.value_transform = value_transform_spec_from_yaml(node["value_transform"]);
    spec.seed = node["seed"] ? node["seed"].as<int>() : 0;
    return spec;
}

YAML::Node function_spec_to_yaml(const FunctionSpec& spec) {
    auto double_vector = [](const std::vector<double>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (double value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto int_vector = [](const std::vector<int>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (int value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto string_vector = [](const std::vector<std::string>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
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
    auto transform_spec = [&](const TransformSpec& transform) {
        YAML::Node node;
        node["kind"] = transform.kind;
        node["dimension"] = transform.dimension;
        node["seed"] = transform.seed;
        node["selected_indices"] = int_vector(transform.selected_indices);
        node["source_point"] = double_vector(transform.source_point);
        node["target_point"] = double_vector(transform.target_point);
        node["parameters"] = double_vector(transform.parameters);
        node["matrix"] = double_matrix(transform.matrix);
        return node;
    };
    auto value_transform_spec = [&double_vector](const ValueTransformSpec& transform) {
        YAML::Node node;
        node["kind"] = transform.kind;
        node["seed"] = transform.seed;
        node["parameters"] = double_vector(transform.parameters);
        return node;
    };
    auto composition_spec = [&](const CompositionSpec& composition) {
        YAML::Node node;
        node["kind"] = composition.kind;
        node["seed"] = composition.seed;
        node["parameters"] = double_vector(composition.parameters);
        node["centers"] = double_matrix(composition.centers);
        node["offsets"] = double_vector(composition.offsets);
        node["weights"] = double_vector(composition.weights);
        return node;
    };

    YAML::Node components(YAML::NodeType::Sequence);
    for (const ComponentSpec& component : spec.component_specs) {
        YAML::Node node;
        node["base_function"] = component.base_function;
        node["component_dimension"] = component.component_dimension;
        node["coordinate_transform"] = transform_spec(component.coordinate_transform);
        node["value_transform"] = value_transform_spec(component.value_transform);
        node["seed"] = component.seed;
        components.push_back(node);
    }

    YAML::Node node;
    node["dimension"] = spec.dimension;
    node["lower_bound"] = double_vector(spec.lower_bound);
    node["upper_bound"] = double_vector(spec.upper_bound);
    node["component_specs"] = components;
    node["composition_spec"] = composition_spec(spec.composition_spec);
    node["seed"] = spec.seed;
    node["function_class_label"] = spec.function_class_label;
    node["known_global_minimizer"] = double_vector(spec.known_global_minimizer);
    node["known_global_value"] = spec.known_global_value;
    node["parameters"] = string_vector(spec.parameters);
    return node;
}

FunctionSpec function_spec_from_yaml(const YAML::Node& root) {
    const YAML::Node node = root["function_spec"] ? root["function_spec"] : root;
    if (!node || !node.IsMap()) {
        throw std::invalid_argument("function spec YAML must be a map");
    }

    FunctionSpec spec;
    spec.dimension = node["dimension"] ? node["dimension"].as<int>() : 0;
    spec.lower_bound = yaml_double_vector(node["lower_bound"], "lower_bound");
    spec.upper_bound = yaml_double_vector(node["upper_bound"], "upper_bound");
    if (node["component_specs"]) {
        if (!node["component_specs"].IsSequence()) {
            throw std::invalid_argument("component_specs must be a sequence");
        }
        spec.component_specs.reserve(node["component_specs"].size());
        for (const YAML::Node& component : node["component_specs"]) {
            spec.component_specs.push_back(component_spec_from_yaml(component));
        }
    }
    spec.composition_spec = composition_spec_from_yaml(node["composition_spec"]);
    spec.seed = node["seed"] ? node["seed"].as<int>() : 0;
    spec.function_class_label = node["function_class_label"] ? node["function_class_label"].as<std::string>() : "";
    spec.known_global_minimizer = yaml_double_vector(node["known_global_minimizer"], "known_global_minimizer");
    spec.known_global_value = node["known_global_value"] ? node["known_global_value"].as<double>() : 0.0;
    spec.parameters = yaml_string_vector(node["parameters"], "function parameters");
    return spec;
}

YAML::Node suite_spec_to_yaml(const SuiteSpec& spec) {
    auto int_vector = [](const std::vector<int>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (int value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto double_vector = [](const std::vector<double>& values) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (double value : values) {
            node.push_back(value);
        }
        return node;
    };
    auto choice_vector = [&double_vector](const std::vector<ChoiceSpec>& choices) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const ChoiceSpec& choice : choices) {
            YAML::Node item;
            item["kind"] = choice.kind;
            item["probability"] = choice.probability;
            item["parameters"] = double_vector(choice.parameters);
            node.push_back(item);
        }
        return node;
    };

    YAML::Node node;
    node["supported_dimensions"] = spec.supported_dimensions;
    node["base_functions"] = int_vector(spec.base_functions);
    node["base_function_coord_transforms"] = choice_vector(spec.base_function_coord_transforms);
    node["coord_transforms"] = choice_vector(spec.coord_transforms);
    node["value_transforms"] = choice_vector(spec.value_transforms);
    node["composition_functions"] = choice_vector(spec.composition_functions);
    node["base_functions_for_compositions"] = int_vector(spec.base_functions_for_compositions);
    node["max_components"] = spec.max_components;
    node["requested_number_of_functions"] = spec.requested_number_of_functions;
    node["max_number_of_functions"] = spec.max_number_of_functions;
    node["master_seed"] = spec.master_seed;
    node["lower_bound"] = spec.lower_bound;
    node["upper_bound"] = spec.upper_bound;
    node["f_opt"] = spec.f_opt;
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
