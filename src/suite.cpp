#include "suite.h"
#include "basicf.h"
#include "support.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace FuncCraft {
using namespace detail;

namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::vector<int> unique_sorted(std::vector<int> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::vector<double> yaml_number_sequence_to_double_vector(const YAML::Node& node, const std::string& field) {
    require(node && node.IsSequence(), field + " must be a YAML sequence");
    std::vector<double> values;
    values.reserve(node.size());
    for (const auto& item : node) {
        values.push_back(item.as<double>());
    }
    return values;
}

std::vector<int> yaml_number_sequence_to_int_vector(const YAML::Node& node, const std::string& field) {
    require(node && node.IsSequence(), field + " must be a YAML sequence");
    std::vector<int> values;
    values.reserve(node.size());
    for (const auto& item : node) {
        values.push_back(item.as<int>());
    }
    return values;
}

std::string yaml_supported_dimensions(const YAML::Node& node) {
    if (!node || node.IsNull()) {
        return "any";
    }
    if (node.IsScalar()) {
        return node.as<std::string>();
    }
    require(node.IsSequence(), "supported_dimensions must be a scalar or sequence");
    std::ostringstream out;
    for (std::size_t i = 0; i < node.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << node[i].as<int>();
    }
    return out.str();
}

std::vector<int> yaml_int_list(const YAML::Node& node, const std::string& field) {
    if (!node || node.IsNull()) {
        return {};
    }
    if (node.IsSequence()) {
        return yaml_number_sequence_to_int_vector(node, field);
    }
    if (node.IsScalar()) {
        const std::string value = trim(node.as<std::string>());
        if (value.empty()) {
            return {};
        }
        if (value.find(',') != std::string::npos) {
            std::vector<int> values;
            std::istringstream in(value);
            std::string token;
            while (std::getline(in, token, ',')) {
                token = trim(token);
                if (!token.empty()) {
                    values.push_back(std::stoi(token));
                }
            }
            return values;
        }
        return {std::stoi(value)};
    }
    throw std::invalid_argument(field + " must be a scalar or sequence");
}

BasicFunctionId parse_basic_function_yaml_item(const YAML::Node& node, const std::string& field) {
    require(node && node.IsScalar(), field + " entries must be scalar");
    const std::string text = trim(node.as<std::string>());
    const std::string normalized = normalize_spec_name(text);
    for (BasicFunctionId id : list_basic_functions()) {
        if (normalize_spec_name(to_string(id)) == normalized) {
            return id;
        }
    }
    try {
        return static_cast<BasicFunctionId>(std::stoi(text));
    } catch (const std::exception&) {
        throw std::invalid_argument("unknown base function in " + field + ": " + text);
    }
}

std::vector<BasicFunctionId> yaml_basic_function_list(const YAML::Node& node, const std::string& field) {
    if (!node || node.IsNull()) {
        return {};
    }
    std::vector<BasicFunctionId> values;
    if (node.IsSequence()) {
        values.reserve(node.size());
        for (const auto& item : node) {
            values.push_back(parse_basic_function_yaml_item(item, field));
        }
        return values;
    }
    if (node.IsScalar()) {
        const std::string text = trim(node.as<std::string>());
        if (text.empty()) {
            return {};
        }
        if (text.find(',') != std::string::npos) {
            std::istringstream in(text);
            std::string token;
            while (std::getline(in, token, ',')) {
                token = trim(token);
                if (!token.empty()) {
                    YAML::Node scalar(token);
                    values.push_back(parse_basic_function_yaml_item(scalar, field));
                }
            }
            return values;
        }
        values.push_back(parse_basic_function_yaml_item(node, field));
        return values;
    }
    throw std::invalid_argument(field + " must be a scalar or sequence");
}

std::vector<double> yaml_double_list(const YAML::Node& node, const std::string& field) {
    if (!node || node.IsNull()) {
        return {};
    }
    if (node.IsSequence()) {
        return yaml_number_sequence_to_double_vector(node, field);
    }
    if (node.IsScalar()) {
        const std::string value = trim(node.as<std::string>());
        if (value.empty()) {
            return {};
        }
        return {std::stod(value)};
    }
    throw std::invalid_argument(field + " must be a scalar or sequence");
}

template <typename Choice, typename ParseKind>
Choice choice_from_yaml(const YAML::Node& node, ParseKind parse_kind) {
    require(node && node.IsMap(), "choice entry must be a YAML mapping");
    Choice choice;
    if (node["kind"]) {
        choice.kind = parse_kind(node["kind"].as<std::string>());
    }
    if (node["probability"]) {
        choice.probability = node["probability"].as<double>();
    }
    if (node["parameters"]) {
        choice.parameters = yaml_double_list(node["parameters"], "choice parameters");
    }
    return choice;
}

template <typename Choice, typename ParseKind>
std::vector<Choice> yaml_choice_list(const YAML::Node& node, const std::string& field, ParseKind parse_kind) {
    if (!node || node.IsNull()) {
        return {};
    }
    require(node.IsSequence(), field + " must be a YAML sequence");
    std::vector<Choice> values;
    values.reserve(node.size());
    for (const auto& item : node) {
        values.push_back(choice_from_yaml<Choice>(item, parse_kind));
    }
    return values;
}

SuiteSpec suite_spec_from_yaml_node(const YAML::Node& node) {
    require(node && node.IsMap(), "suite specification root must be a YAML mapping");
    SuiteSpec spec;
    if (node["supported_dimensions"]) {
        spec.supported_dimensions = yaml_supported_dimensions(node["supported_dimensions"]);
    }
    if (node["base_functions"]) {
        spec.base_functions = yaml_basic_function_list(node["base_functions"], "base_functions");
    }
    if (node["coordinate_transforms"]) {
        spec.coordinate_transforms = yaml_choice_list<CoordinateTransformChoice>(
            node["coordinate_transforms"],
            "coordinate_transforms",
            parse_coordinate_transform_kind);
    }
    if (node["value_transforms"]) {
        spec.value_transforms = yaml_choice_list<ValueTransformChoice>(
            node["value_transforms"],
            "value_transforms",
            parse_value_transform_kind);
    }
    if (node["compositions"]) {
        spec.compositions = yaml_choice_list<CompositionChoice>(
            node["compositions"],
            "compositions",
            parse_composition_kind);
    }
    if (node["composition_base_functions"]) {
        spec.composition_base_functions = yaml_basic_function_list(
            node["composition_base_functions"],
            "composition_base_functions");
    }
    if (node["max_components"]) {
        spec.max_components = node["max_components"].as<int>();
    }
    if (node["requested_number_of_functions"]) {
        spec.requested_number_of_functions = node["requested_number_of_functions"].as<int>();
    }
    if (node["max_number_of_functions"]) {
        spec.max_number_of_functions = node["max_number_of_functions"].as<int>();
    }
    if (node["master_seed"]) {
        spec.master_seed = node["master_seed"].as<unsigned long long>();
    }
    if (node["lower_bound"]) {
        spec.lower_bound = node["lower_bound"].as<double>();
    }
    if (node["upper_bound"]) {
        spec.upper_bound = node["upper_bound"].as<double>();
    }
    if (node["assigned_fopt"]) {
        spec.assigned_fopt = node["assigned_fopt"].as<double>();
    }
    if (node["xopt_domain_shrink_factor"]) {
        spec.xopt_domain_shrink_factor = node["xopt_domain_shrink_factor"].as<double>();
    }
    if (node["suite_label"]) {
        spec.suite_label = node["suite_label"].as<std::string>();
    }
    return spec;
}

std::vector<int> parse_dimension_values(const std::string& expr, int ambient_dimension) {
    std::string cleaned = trim(expr);
    while (!cleaned.empty() && (cleaned.front() == '[' || cleaned.front() == '(' || cleaned.front() == '{')) {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && (cleaned.back() == ']' || cleaned.back() == ')' || cleaned.back() == '}')) {
        cleaned.pop_back();
    }
    cleaned = trim(cleaned);
    const std::string normalized = normalize_spec_name(cleaned);
    require(ambient_dimension > 0, "ambient dimension must be positive");

    if (normalized.empty() || normalized == "any" || normalized == "all") {
        std::vector<int> values;
        values.reserve(static_cast<std::size_t>(ambient_dimension));
        for (int d = 1; d <= ambient_dimension; ++d) {
            values.push_back(d);
        }
        return values;
    }

    if (cleaned.find(',') != std::string::npos) {
        std::vector<int> values;
        std::istringstream in(cleaned);
        std::string token;
        while (std::getline(in, token, ',')) {
            token = trim(token);
            if (!token.empty()) {
                values.push_back(std::stoi(token));
            }
        }
        values = unique_sorted(std::move(values));
        require(!values.empty(), "supported_dimensions list must not be empty");
        for (int value : values) {
            require(value > 0, "supported dimension values must be positive");
        }
        return values;
    }

    const std::size_t gt_pos = cleaned.find('>');
    if (gt_pos != std::string::npos) {
        const bool inclusive = gt_pos + 1 < cleaned.size() && cleaned[gt_pos + 1] == '=';
        const int lower = std::stoi(trim(cleaned.substr(gt_pos + (inclusive ? 2 : 1))));
        const int start = inclusive ? lower : lower + 1;
        require(start <= ambient_dimension, "supported dimension lower bound exceeds ambient dimension");
        std::vector<int> values;
        for (int d = start; d <= ambient_dimension; ++d) {
            values.push_back(d);
        }
        return values;
    }

    const std::size_t dash_pos = cleaned.find('-');
    if (dash_pos != std::string::npos) {
        const int lower = std::stoi(trim(cleaned.substr(0, dash_pos)));
        const int upper = std::stoi(trim(cleaned.substr(dash_pos + 1)));
        require(lower > 0, "supported dimension lower bound must be positive");
        require(upper >= lower, "supported dimension range must be increasing");
        require(upper <= ambient_dimension, "supported dimension upper bound exceeds ambient dimension");
        std::vector<int> values;
        for (int d = lower; d <= upper; ++d) {
            values.push_back(d);
        }
        return values;
    }

    const int single = std::stoi(cleaned);
    require(single > 0, "supported dimension must be positive");
    return {single};
}

std::vector<int> normalize_dimensions(const SuiteSpec& spec, int ambient_dimension) {
    std::vector<int> values = parse_dimension_values(spec.supported_dimensions, ambient_dimension);
    values = unique_sorted(std::move(values));
    require(!values.empty(), "suite must support at least one dimension");
    return values;
}

std::vector<BasicFunctionId> normalize_base_functions(const std::vector<BasicFunctionId>& ids) {
    const auto all = list_basic_functions();
    std::set<BasicFunctionId> allowed;
    for (BasicFunctionId id : all) {
        allowed.insert(id);
    }

    std::vector<BasicFunctionId> result;
    std::set<BasicFunctionId> seen;
    if (ids.empty()) {
        result = all;
    } else {
        for (BasicFunctionId id : ids) {
            require(allowed.count(id) != 0, "unknown base function id in suite spec");
            if (seen.insert(id).second) {
                result.push_back(id);
            }
        }
    }
    require(!result.empty(), "suite needs at least one base function");
    return result;
}

std::vector<BasicFunctionId> normalize_composition_base_functions(
    const std::vector<BasicFunctionId>& ids,
    const std::vector<BasicFunctionId>& fallback) {
    if (ids.empty()) {
        return fallback;
    }

    const auto all = list_basic_functions();
    std::set<BasicFunctionId> allowed;
    for (BasicFunctionId id : all) {
        allowed.insert(id);
    }

    std::vector<BasicFunctionId> result;
    std::set<BasicFunctionId> seen;
    for (BasicFunctionId id : ids) {
        require(allowed.count(id) != 0, "unknown composition base-function id in suite spec");
        if (seen.insert(id).second) {
            result.push_back(id);
        }
    }
    require(!result.empty(), "composition base-function pool must contain at least one base function");
    return result;
}

template <typename Choice>
std::vector<Choice> normalize_choices(const std::vector<Choice>& choices, const std::vector<Choice>& fallback) {
    if (choices.empty()) {
        return fallback;
    }

    std::vector<Choice> result;
    result.reserve(choices.size());
    for (const Choice& choice : choices) {
        Choice normalized = choice;
        require(normalized.probability >= 0.0, "choice probability must be nonnegative");
        result.push_back(std::move(normalized));
    }
    double probability_sum = 0.0;
    for (const Choice& choice : result) {
        probability_sum += choice.probability;
    }
    require(std::fabs(probability_sum - 1.0) <= 1.0e-12, "choice probabilities must sum to one");
    return result;
}

template <typename Choice>
double total_probability(const std::vector<Choice>& choices) {
    double sum = 0.0;
    for (const Choice& choice : choices) {
        sum += choice.probability;
    }
    return sum;
}

template <typename Choice>
const Choice& choose_weighted(const std::vector<Choice>& choices, std::mt19937_64& rng) {
    require(!choices.empty(), "weighted choice list must not be empty");
    const double total = total_probability(choices);
    require(total > 0.0, "weighted choice list must have positive total probability");
    const double target = uniform01(rng) * total;
    double accum = 0.0;
    for (const Choice& choice : choices) {
        accum += choice.probability;
        if (target < accum) {
            return choice;
        }
    }
    return choices.back();
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

std::vector<std::vector<double>> latin_hypercube_points_in_domain(
    std::mt19937_64& rng,
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

    std::vector<int> strata(static_cast<std::size_t>(count), 0);
    for (int i = 0; i < count; ++i) {
        strata[static_cast<std::size_t>(i)] = i;
    }

    for (int d = 0; d < domain.dimension(); ++d) {
        stable_shuffle(strata, rng);
        const auto dim = static_cast<std::size_t>(d);
        const double lo = domain.lower[dim];
        const double hi = domain.upper[dim];
        for (int i = 0; i < count; ++i) {
            const double t = (static_cast<double>(strata[static_cast<std::size_t>(i)]) + uniform01(rng))
                / static_cast<double>(count);
            points[static_cast<std::size_t>(i)][dim] = lo + (hi - lo) * t;
        }
    }

    return points;
}

std::vector<std::vector<double>> latin_hypercube_centers(
    std::mt19937_64& rng,
    const Domain& domain,
    int count,
    double shrink_factor) {
    return latin_hypercube_points_in_domain(rng, centered_scaled_domain(domain, shrink_factor), count);
}

CoordinateTransformSpec make_coordinate_transform_spec(
    const CoordinateTransformChoice& choice,
    int dimension,
    const std::vector<double>& assigned_xopt,
    std::uint64_t seed,
    std::mt19937_64& rng,
    const std::vector<int>* selected_indices) {
    CoordinateTransformSpec spec;
    spec.kind = choice.kind;
    spec.dimension = dimension;
    spec.seed = seed;
    spec.assigned_xopt = assigned_xopt;

    if (choice.kind == CoordinateTransformKind::None) {
        return spec;
    }
    if (choice.kind == CoordinateTransformKind::Rotation) {
        return spec;
    }
    if (choice.kind == CoordinateTransformKind::Affine) {
        return spec;
    }
    if (choice.kind == CoordinateTransformKind::BlockRotation) {
        require(dimension > 1, "block rotation requires dimension greater than one");
        if (selected_indices != nullptr) {
            require(!selected_indices->empty(), "block rotation selected indices must not be empty");
            spec.selected_indices = *selected_indices;
            for (int idx : spec.selected_indices) {
                require(idx >= 0 && idx < dimension, "block rotation selected index out of range");
            }
            return spec;
        }
        const int selected_size = uniform_int(rng, 1, dimension);
        std::set<int> indices;
        while (static_cast<int>(indices.size()) < selected_size) {
            indices.insert(uniform_int(rng, 0, dimension - 1));
        }
        spec.selected_indices.assign(indices.begin(), indices.end());
        return spec;
    }
    throw std::logic_error("unhandled coordinate transform kind in suite spec");
}

ValueTransformSpec make_value_transform_spec(const ValueTransformChoice& choice) {
    ValueTransformSpec spec;
    spec.kind = choice.kind;
    if (choice.kind == ValueTransformKind::None) {
        return spec;
    }
    if (choice.kind == ValueTransformKind::CosineZero) {
        spec.parameters = choice.parameters.empty() ? std::vector<double>{1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == ValueTransformKind::Oscillatory) {
        spec.parameters = choice.parameters.empty() ? std::vector<double>{0.1, 1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == ValueTransformKind::Power) {
        spec.parameters = choice.parameters.empty() ? std::vector<double>{1.0, 1.0} : choice.parameters;
        return spec;
    }
    throw std::logic_error("unhandled value transform kind in suite spec");
}

CompositionSpec make_composition_spec(
    const CompositionChoice& choice) {
    CompositionSpec spec;
    spec.kind = choice.kind;
    if (choice.kind == CompositionKind::CpmWeightedSum) {
        return spec;
    }
    if (choice.kind == CompositionKind::CpmLevelWell) {
        spec.parameters = choice.parameters.empty() ? std::vector<double>{0.1, 1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == CompositionKind::CpmPowerMean) {
        spec.parameters = choice.parameters.empty() ? std::vector<double>{2.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == CompositionKind::DpmSoftmax) {
        spec.parameters = choice.parameters.empty()
            ? std::vector<double>{0.01}
            : choice.parameters;
        return spec;
    }
    if (choice.kind == CompositionKind::DpmBgSoftmax) {
        spec.parameters = choice.parameters.empty()
            ? std::vector<double>{0.01, 1.0, 0.01}
            : choice.parameters;
        return spec;
    }
    throw std::logic_error("unhandled composition kind in suite spec");
}

std::vector<BasicFunctionId> sample_base_functions(
    const std::vector<BasicFunctionId>& pool,
    int count,
    std::mt19937_64& rng) {
    require(count > 0, "sample count must be positive");
    require(!pool.empty(), "sample pool must not be empty");
    std::vector<BasicFunctionId> selected;
    selected.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int choice = uniform_int(rng, 0, static_cast<int>(pool.size()) - 1);
        selected.push_back(pool[static_cast<std::size_t>(choice)]);
    }
    return selected;
}

bool is_block_rotation_choice(const CoordinateTransformChoice& choice) {
    return choice.kind == CoordinateTransformKind::BlockRotation;
}

bool is_deceptive_composition_choice(const CompositionChoice& choice) {
    return choice.kind == CompositionKind::DpmSoftmax || choice.kind == CompositionKind::DpmBgSoftmax;
}

std::vector<int> full_dimension_indices(int dimension) {
    require(dimension > 0, "dimension must be positive");
    std::vector<int> indices(static_cast<std::size_t>(dimension), 0);
    for (int i = 0; i < dimension; ++i) {
        indices[static_cast<std::size_t>(i)] = i;
    }
    return indices;
}

std::vector<int> random_nonempty_subspace(int dimension, std::mt19937_64& rng) {
    require(dimension > 0, "subspace dimension must be positive");

    const int selected_size = dimension == 1 ? 1 : uniform_int(rng, 1, dimension);
    std::set<int> indices;
    while (static_cast<int>(indices.size()) < selected_size) {
        indices.insert(uniform_int(rng, 0, dimension - 1));
    }
    return {indices.begin(), indices.end()};
}

std::vector<std::vector<int>> covering_block_subspaces(int dimension, int count, std::mt19937_64& rng) {
    require(dimension > 0, "subspace dimension must be positive");
    require(count > 0, "subspace count must be positive");

    std::vector<std::vector<int>> subspaces(static_cast<std::size_t>(count));
    for (int d = 0; d < dimension; ++d) {
        const int subspace = uniform_int(rng, 0, count - 1);
        subspaces[static_cast<std::size_t>(subspace)].push_back(d);
    }

    for (auto& indices : subspaces) {
        if (indices.empty()) {
            indices = random_nonempty_subspace(dimension, rng);
        }
        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    }

    return subspaces;
}

std::vector<std::vector<int>> block_rotation_subspaces(
    const std::vector<CoordinateTransformChoice>& coord_choices,
    int dimension,
    std::mt19937_64& rng) {
    const bool all_block_rotation = std::all_of(coord_choices.begin(), coord_choices.end(), is_block_rotation_choice);
    const int block_count = static_cast<int>(std::count_if(coord_choices.begin(), coord_choices.end(), is_block_rotation_choice));
    std::vector<std::vector<int>> generated = all_block_rotation
        ? covering_block_subspaces(dimension, block_count, rng)
        : std::vector<std::vector<int>>{};

    std::vector<std::vector<int>> per_component(coord_choices.size());
    int block_index = 0;
    for (std::size_t i = 0; i < coord_choices.size(); ++i) {
        if (!is_block_rotation_choice(coord_choices[i])) {
            continue;
        }
        per_component[i] = all_block_rotation
            ? generated[static_cast<std::size_t>(block_index)]
            : random_nonempty_subspace(dimension, rng);
        ++block_index;
    }
    return per_component;
}

std::uint64_t saturating_add(std::uint64_t a, std::uint64_t b) {
    const std::uint64_t limit = std::numeric_limits<std::uint64_t>::max();
    if (limit - a < b) {
        return limit;
    }
    return a + b;
}

std::uint64_t saturating_mul(std::uint64_t a, std::uint64_t b) {
    const std::uint64_t limit = std::numeric_limits<std::uint64_t>::max();
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > limit / b) {
        return limit;
    }
    return a * b;
}

std::uint64_t saturating_pow(std::uint64_t base, int exp) {
    if (exp < 0) {
        return 0;
    }
    std::uint64_t result = 1;
    for (int i = 0; i < exp; ++i) {
        result = saturating_mul(result, base);
        if (result == std::numeric_limits<std::uint64_t>::max()) {
            break;
        }
    }
    return result;
}

} // namespace

BenchmarkSuite::BenchmarkSuite(SuiteSpec spec, int dimension)
    : spec_(std::move(spec)),
      dimension_(dimension),
      supported_dimensions_(normalize_dimensions(spec_, dimension_)) {
    require(dimension_ > 0, "suite dimension must be positive");
    require(supports_dimension(dimension_), "suite dimension is not supported by the suite spec");

    const std::vector<BasicFunctionId> mandatory_base_functions = normalize_base_functions(spec_.base_functions);
    const std::vector<BasicFunctionId> composition_pool = normalize_composition_base_functions(
        spec_.composition_base_functions,
        mandatory_base_functions);

    const int coord_family_count = static_cast<int>(normalize_choices(spec_.coordinate_transforms, all_coordinate_transform_choices()).size());
    const int value_family_count = static_cast<int>(normalize_choices(spec_.value_transforms, all_value_transform_choices()).size());
    const auto composition_choices = normalize_choices(spec_.compositions, all_composition_choices());
    const int max_components = spec_.max_components;
    require(max_components >= 2, "suite spec max_components must be at least 2");
    std::uint64_t theoretical_composed = 0;
    for (int component_count = 2; component_count <= max_components; ++component_count) {
        const std::uint64_t component_orders = saturating_pow(static_cast<std::uint64_t>(composition_pool.size()), component_count);
        const std::uint64_t coord_choices = saturating_pow(static_cast<std::uint64_t>(coord_family_count), component_count);
        const std::uint64_t value_choices = saturating_pow(static_cast<std::uint64_t>(value_family_count), component_count);
        const std::uint64_t choices = saturating_mul(component_orders, saturating_mul(coord_choices, value_choices));
        theoretical_composed = saturating_add(theoretical_composed, saturating_mul(choices, static_cast<std::uint64_t>(composition_choices.size())));
    }
    theoretical_max_number_of_functions_ = saturating_add(
        static_cast<std::uint64_t>(mandatory_base_functions.size()),
        theoretical_composed);

    const int mandatory_count = static_cast<int>(mandatory_base_functions.size());
    const int requested_count = spec_.requested_number_of_functions > 0
        ? spec_.requested_number_of_functions
        : mandatory_count;
    require(requested_count >= mandatory_count, "requested_number_of_functions is smaller than the mandatory base-function count");

    const auto coord_choices = normalize_choices(spec_.coordinate_transforms, all_coordinate_transform_choices());
    const auto value_choices = normalize_choices(spec_.value_transforms, all_value_transform_choices());
    if (requested_count > mandatory_count) {
        require(!composition_pool.empty(), "composition base-function pool must not be empty");
    }

    blueprints_.reserve(static_cast<std::size_t>(requested_count));
    std::uint64_t sequence = 0;

    for (BasicFunctionId id : mandatory_base_functions) {
        std::mt19937_64 rng(mix_seed(spec_.master_seed + sequence + 0x1000ull));
        FunctionBlueprint blueprint;
        blueprint.composed = false;
        blueprint.base_function = id;
        blueprint.coordinate_transform_choice = choose_weighted(coord_choices, rng);
        blueprint.seed = mix_seed(spec_.master_seed + sequence + 0x2000ull);
        blueprints_.push_back(std::move(blueprint));
        ++sequence;
    }

    while (static_cast<int>(blueprints_.size()) < requested_count) {
        std::mt19937_64 rng(mix_seed(spec_.master_seed + sequence + 0x3000ull));
        const CompositionChoice& comp_choice = choose_weighted(composition_choices, rng);
        const int max_components = spec_.max_components;
        const int component_count = uniform_int(rng, 2, max_components);
        FunctionBlueprint blueprint;
        blueprint.composed = true;
        blueprint.component_count = component_count;
        blueprint.component_bases = sample_base_functions(composition_pool, component_count, rng);
        blueprint.coordinate_transform_choices.reserve(static_cast<std::size_t>(component_count));
        blueprint.value_transform_choices.reserve(static_cast<std::size_t>(component_count));
        for (int i = 0; i < component_count; ++i) {
            blueprint.coordinate_transform_choices.push_back(choose_weighted(coord_choices, rng));
            blueprint.value_transform_choices.push_back(choose_weighted(value_choices, rng));
        }
        blueprint.composition_choice = comp_choice;
        blueprint.seed = mix_seed(spec_.master_seed + sequence + 0x4000ull);

        blueprints_.push_back(std::move(blueprint));
        ++sequence;
    }

    spec_.max_number_of_functions = static_cast<int>(blueprints_.size());
    function_cache_.resize(blueprints_.size());
    std::cout << "BenchmarkSuite generated. Requested functions: " << requested_count
              << ", theoretical max functions: " << std::scientific << static_cast<long double>(theoretical_max_number_of_functions_) << std::defaultfloat
              << ", generated functions: " << max_number_of_functions()
              << ", dimension: " << dimension_ << '\n';
}

BenchmarkSuite::BenchmarkSuite(const std::string& yaml_path, int dimension)
    : BenchmarkSuite(load_suite_spec_yaml(yaml_path), dimension) {
    std::cout << "A benchmark suite configuration has been read successfully from YAML file: "
              << yaml_path
              << " (dimension: " << dimension_
              << ", requested_functions: " << spec_.requested_number_of_functions
              << ")\n";
}

bool BenchmarkSuite::supports_dimension(int dimension) const {
    return std::find(supported_dimensions_.begin(), supported_dimensions_.end(), dimension) != supported_dimensions_.end();
}

BenchmarkFunction BenchmarkSuite::build_function(const FunctionBlueprint& blueprint) const {
    require(dimension_ > 0, "dimension must be positive");

    const Domain domain(dimension_, spec_.lower_bound, spec_.upper_bound);
    std::mt19937_64 rng(mix_seed(blueprint.seed ^ static_cast<std::uint64_t>(dimension_)));

    FunctionSpec function_spec;
    function_spec.dimension = dimension_;
    function_spec.domain.dimension = dimension_;
    function_spec.domain.lower_bound = domain.lower;
    function_spec.domain.upper_bound = domain.upper;
    function_spec.seed = blueprint.seed;
    function_spec.assigned_fopt = spec_.assigned_fopt;
    function_spec.metadata.push_back("suite_label=" + spec_.suite_label);

    if (!blueprint.composed) {
        const auto x_star = latin_hypercube_centers(rng, domain, 1, spec_.xopt_domain_shrink_factor).front();
        function_spec.assigned_xopt = x_star;
        function_spec.metadata.push_back("suite_role=base");
        function_spec.composition.kind = CompositionKind::None;

        const std::vector<int> full_subspace = is_block_rotation_choice(blueprint.coordinate_transform_choice)
            ? full_dimension_indices(dimension_)
            : std::vector<int>{};
        ComponentSpec component;
        component.base_function = blueprint.base_function;
        component.component_dimension = dimension_;
        component.coordinate_transform = make_coordinate_transform_spec(
            blueprint.coordinate_transform_choice,
            dimension_,
            x_star,
            blueprint.seed,
            rng,
            is_block_rotation_choice(blueprint.coordinate_transform_choice) ? &full_subspace : nullptr);
        component.value_transform.kind = ValueTransformKind::None;
        component.seed = blueprint.seed;
        function_spec.components.push_back(std::move(component));

        return BenchmarkFunction(std::move(function_spec));
    }

    const bool dpm_mode = is_deceptive_composition_choice(blueprint.composition_choice);
    const int random_center_count = dpm_mode
        ? 1 + std::max(0, blueprint.component_count - 2)
        : 1;
    const std::vector<std::vector<double>> random_centers = latin_hypercube_centers(
        rng,
        domain,
        random_center_count,
        spec_.xopt_domain_shrink_factor);
    const auto x_star = random_centers.front();
    std::vector<std::vector<double>> centers(static_cast<std::size_t>(blueprint.component_count), x_star);
    std::vector<double> offsets(static_cast<std::size_t>(blueprint.component_count), 0.0);
    const bool uses_block_rotation = std::any_of(
        blueprint.coordinate_transform_choices.begin(),
        blueprint.coordinate_transform_choices.end(),
        is_block_rotation_choice);
    std::vector<std::vector<int>> block_subspaces = uses_block_rotation
        ? block_rotation_subspaces(blueprint.coordinate_transform_choices, dimension_, rng)
        : std::vector<std::vector<int>>(static_cast<std::size_t>(blueprint.component_count));
    if (dpm_mode && is_block_rotation_choice(blueprint.coordinate_transform_choices.front())) {
        block_subspaces.front() = full_dimension_indices(dimension_);
    }
    if (dpm_mode) {
        const double deceptive_step = 10.0 + 0.1 * spec_.assigned_fopt;
        int random_center_index = 1;
        for (int i = 1; i < blueprint.component_count; ++i) {
            if (i == 1) {
                centers[static_cast<std::size_t>(i)] = std::vector<double>(static_cast<std::size_t>(dimension_), 0.0);
            } else {
                centers[static_cast<std::size_t>(i)] = random_centers[static_cast<std::size_t>(random_center_index)];
                ++random_center_index;
            }
            offsets[static_cast<std::size_t>(i)] = deceptive_step * static_cast<double>(i);
        }
    }

    function_spec.assigned_xopt = x_star;
    function_spec.metadata.push_back("suite_role=composed");
    function_spec.composition = make_composition_spec(blueprint.composition_choice);

    for (int i = 0; i < blueprint.component_count; ++i) {
        const auto pos = static_cast<std::size_t>(i);
        const std::uint64_t component_seed = mix_seed(blueprint.seed + static_cast<std::uint64_t>(i) + 1ULL);
        ComponentSpec component;
        component.base_function = blueprint.component_bases[pos];
        component.component_dimension = dimension_;
        component.coordinate_transform = make_coordinate_transform_spec(
            blueprint.coordinate_transform_choices[pos],
            dimension_,
            centers[pos],
            component_seed,
            rng,
            is_block_rotation_choice(blueprint.coordinate_transform_choices[pos])
                ? &block_subspaces[pos]
                : nullptr);
        component.value_transform = make_value_transform_spec(blueprint.value_transform_choices[pos]);
        component.f_bias = offsets[pos];
        component.seed = component_seed;
        function_spec.components.push_back(std::move(component));
    }

    return BenchmarkFunction(std::move(function_spec));
}

int BenchmarkSuite::size() const {
    return static_cast<int>(blueprints_.size());
}

int BenchmarkSuite::max_number_of_functions() const {
    return size();
}

std::uint64_t BenchmarkSuite::theoretical_max_number_of_functions() const {
    return theoretical_max_number_of_functions_;
}

int BenchmarkSuite::dimension() const {
    return dimension_;
}

const BenchmarkFunction& BenchmarkSuite::function(int index) const {
    require(index >= 0 && index < size(), "benchmark function index out of range");
    const std::size_t pos = static_cast<std::size_t>(index);
    if (!function_cache_[pos].has_value()) {
        function_cache_[pos].emplace(build_function(blueprints_[pos]));
    }
    return *function_cache_[pos];
}

std::vector<double> BenchmarkSuite::operator()(int index, const std::vector<std::vector<double>>& X) const {
    return function(index)(X);
}

const SuiteSpec& BenchmarkSuite::spec() const {
    return spec_;
}

YAML::Node BenchmarkSuite::export_manifest() const {
    YAML::Node node;
    node["format"] = "funccraft.benchmark_suite_manifest";
    node["format_version"] = 1;
    node["suite_spec"] = detail::suite_spec_to_yaml(spec_);
    node["dimension"] = dimension_;
    node["size"] = size();
    node["max_number_of_functions"] = max_number_of_functions();
    node["theoretical_max_number_of_functions"] = theoretical_max_number_of_functions_;

    YAML::Node functions(YAML::NodeType::Sequence);
    for (int i = 0; i < size(); ++i) {
        YAML::Node entry = function(i).export_spec();
        entry["index"] = i;
        functions.push_back(entry);
    }
    node["functions"] = functions;
    return node;
}

void BenchmarkSuite::export_manifest(const std::string& path) const {
    detail::write_yaml_file(path, export_manifest());
}

YAML::Node BenchmarkSuite::export_spec() const {
    return export_manifest();
}

void BenchmarkSuite::export_spec(const std::string& path) const {
    export_manifest(path);
}

BenchmarkSuite make_benchmark_suite(SuiteSpec spec, int dimension) {
    return BenchmarkSuite(std::move(spec), dimension);
}

SuiteSpec load_suite_spec_yaml(const std::string& path) {
    try {
        const YAML::Node root = YAML::LoadFile(path);
        if (root["suite_spec"]) {
            return suite_spec_from_yaml_node(root["suite_spec"]);
        }
        return suite_spec_from_yaml_node(root);
    } catch (const YAML::Exception& e) {
        throw std::invalid_argument(std::string("failed to load suite spec from YAML: ") + e.what());
    }
}

BenchmarkSuite make_benchmark_suite_from_yaml(const std::string& path, int dimension) {
    return BenchmarkSuite(load_suite_spec_yaml(path), dimension);
}

} // namespace FuncCraft
