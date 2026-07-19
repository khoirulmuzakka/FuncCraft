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

namespace FuncCraft {
using namespace detail;

namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string normalize_token(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (ch == '_' || ch == '-' || ch == '/' || ch == '[' || ch == ']' || ch == '{' || ch == '}' || std::isspace(ch)) {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized;
}

std::vector<int> unique_sorted(std::vector<int> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::vector<int> parse_dimension_values(const std::string& expr, int max_dimension) {
    std::string cleaned = trim(expr);
    while (!cleaned.empty() && (cleaned.front() == '[' || cleaned.front() == '(' || cleaned.front() == '{')) {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && (cleaned.back() == ']' || cleaned.back() == ')' || cleaned.back() == '}')) {
        cleaned.pop_back();
    }
    cleaned = trim(cleaned);
    const std::string normalized = normalize_token(cleaned);
    require(max_dimension > 0, "max_dimension must be positive");

    if (normalized.empty() || normalized == "any" || normalized == "all") {
        std::vector<int> values;
        values.reserve(static_cast<std::size_t>(max_dimension));
        for (int d = 1; d <= max_dimension; ++d) {
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
        require(start <= max_dimension, "supported dimension lower bound exceeds max_dimension");
        std::vector<int> values;
        for (int d = start; d <= max_dimension; ++d) {
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
        require(upper <= max_dimension, "supported dimension upper bound exceeds max_dimension");
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

std::vector<int> normalize_dimensions(const SuiteSpec& spec) {
    std::vector<int> values = parse_dimension_values(spec.supported_dimensions, spec.max_dimension);
    values = unique_sorted(std::move(values));
    require(!values.empty(), "suite must support at least one dimension");
    return values;
}

std::vector<BasicFunctionId> normalize_base_functions(const std::vector<int>& ids) {
    const auto all = list_basic_functions();
    std::set<int> allowed;
    for (BasicFunctionId id : all) {
        allowed.insert(static_cast<int>(id));
    }

    std::vector<BasicFunctionId> result;
    std::set<int> seen;
    if (ids.empty()) {
        result = all;
    } else {
        for (int raw_id : ids) {
            require(allowed.count(raw_id) != 0, "unknown base function id in suite spec");
            if (seen.insert(raw_id).second) {
                result.push_back(static_cast<BasicFunctionId>(raw_id));
            }
        }
    }
    require(!result.empty(), "suite needs at least one base function");
    return result;
}

std::vector<BasicFunctionId> normalize_composition_base_functions(
    const std::vector<int>& ids,
    const std::vector<BasicFunctionId>& fallback) {
    if (ids.empty()) {
        return fallback;
    }

    const auto all = list_basic_functions();
    std::set<int> allowed;
    for (BasicFunctionId id : all) {
        allowed.insert(static_cast<int>(id));
    }

    std::vector<BasicFunctionId> result;
    std::set<int> seen;
    for (int raw_id : ids) {
        require(allowed.count(raw_id) != 0, "unknown composition base-function id in suite spec");
        if (seen.insert(raw_id).second) {
            result.push_back(static_cast<BasicFunctionId>(raw_id));
        }
    }
    require(result.size() >= 2, "composition base-function pool must contain at least two base functions");
    return result;
}

std::vector<ChoiceSpec> normalize_choices(const std::vector<ChoiceSpec>& choices, const std::vector<ChoiceSpec>& fallback) {
    if (choices.empty()) {
        return fallback;
    }

    std::vector<ChoiceSpec> result;
    result.reserve(choices.size());
    for (const ChoiceSpec& choice : choices) {
        ChoiceSpec normalized = choice;
        normalized.kind = normalize_token(choice.kind);
        require(!normalized.kind.empty(), "choice kind must not be empty");
        require(normalized.probability >= 0.0, "choice probability must be nonnegative");
        result.push_back(std::move(normalized));
    }
    return result;
}

double total_probability(const std::vector<ChoiceSpec>& choices) {
    double sum = 0.0;
    for (const ChoiceSpec& choice : choices) {
        sum += choice.probability;
    }
    return sum;
}

const ChoiceSpec& choose_weighted(const std::vector<ChoiceSpec>& choices, std::mt19937_64& rng) {
    require(!choices.empty(), "weighted choice list must not be empty");
    const double total = total_probability(choices);
    require(total > 0.0, "weighted choice list must have positive total probability");
    const double target = uniform01(rng) * total;
    double accum = 0.0;
    for (const ChoiceSpec& choice : choices) {
        accum += choice.probability;
        if (target <= accum) {
            return choice;
        }
    }
    return choices.back();
}

std::vector<double> equal_weights(int count) {
    return std::vector<double>(static_cast<std::size_t>(count), 1.0);
}

std::vector<double> random_point_in_domain(std::mt19937_64& rng, const Domain& domain) {
    std::vector<double> x(static_cast<std::size_t>(domain.dimension()), 0.0);
    for (int i = 0; i < domain.dimension(); ++i) {
        const auto idx = static_cast<std::size_t>(i);
        x[idx] = domain.lower[idx] + (domain.upper[idx] - domain.lower[idx]) * uniform01(rng);
    }
    return x;
}

std::vector<double> random_point_in_domain_away_from_origin(
    std::mt19937_64& rng,
    const Domain& domain,
    double min_norm) {
    for (int attempt = 0; attempt < 1000; ++attempt) {
        auto x = random_point_in_domain(rng, domain);
        double norm_sq = 0.0;
        for (double value : x) {
            norm_sq += value * value;
        }
        if (std::sqrt(norm_sq) >= min_norm) {
            return x;
        }
    }
    auto x = random_point_in_domain(rng, domain);
    if (!x.empty() && std::fabs(x.front()) < min_norm) {
        x.front() = domain.upper.front() >= min_norm ? min_norm : domain.upper.front();
    }
    return x;
}

TransformSpec make_coordinate_transform_spec(
    const ChoiceSpec& choice,
    int dimension,
    const std::vector<double>& source_point,
    const std::vector<double>& target_point,
    std::uint64_t seed,
    std::mt19937_64& rng,
    bool allow_block_rotation,
    const std::vector<int>* selected_indices) {
    TransformSpec spec;
    spec.dimension = dimension;
    spec.seed = static_cast<int>(seed);
    spec.source_point = source_point;
    spec.target_point = target_point;

    if (choice.kind == "identity" || choice.kind == "none" || choice.kind.empty()) {
        spec.kind = "identity";
        return spec;
    }
    if (choice.kind == "rot" || choice.kind == "rotation") {
        spec.kind = "rotation";
        return spec;
    }
    if (choice.kind == "aff" || choice.kind == "affine") {
        spec.kind = "affine";
        return spec;
    }
    if ((choice.kind == "brot" || choice.kind == "blockrot" || choice.kind == "blockrotation") && allow_block_rotation) {
        require(dimension > 1, "block rotation requires dimension greater than one");
        if (selected_indices != nullptr) {
            require(!selected_indices->empty(), "block rotation selected indices must not be empty");
            spec.kind = "block_rotation";
            spec.selected_indices = *selected_indices;
            for (int idx : spec.selected_indices) {
                require(idx >= 0 && idx < dimension, "block rotation selected index out of range");
            }
            return spec;
        }
        const int selected_size = dimension == 2 ? 1 : uniform_int(rng, 2, dimension - 1);
        std::set<int> indices;
        while (static_cast<int>(indices.size()) < selected_size) {
            indices.insert(uniform_int(rng, 0, dimension - 1));
        }
        spec.kind = "block_rotation";
        spec.selected_indices.assign(indices.begin(), indices.end());
        return spec;
    }
    throw std::invalid_argument("unknown coordinate transform kind in suite spec: " + choice.kind);
}

ValueTransformSpec make_value_transform_spec(const ChoiceSpec& choice) {
    ValueTransformSpec spec;
    if (choice.kind == "identity" || choice.kind == "none" || choice.kind.empty()) {
        spec.kind = "identity";
        return spec;
    }
    if (choice.kind == "coszero" || choice.kind == "cosinezero") {
        spec.kind = "cosine_zero";
        spec.parameters = choice.parameters.empty() ? std::vector<double>{1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == "osc" || choice.kind == "oscillatory") {
        spec.kind = "oscillatory";
        spec.parameters = choice.parameters.empty() ? std::vector<double>{0.1, 1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == "power") {
        spec.kind = "power";
        spec.parameters = choice.parameters.empty() ? std::vector<double>{1.0, 1.0} : choice.parameters;
        return spec;
    }
    throw std::invalid_argument("unknown value transform kind in suite spec: " + choice.kind);
}

CompositionSpec make_composition_spec(
    const ChoiceSpec& choice,
    int component_count,
    const std::vector<std::vector<double>>& centers,
    const std::vector<double>& offsets) {
    CompositionSpec spec;
    if (choice.kind == "cpm" || choice.kind == "cpmsum" || choice.kind == "sum" || choice.kind == "weightedsum") {
        spec.kind = "weighted_sum";
        spec.weights = equal_weights(component_count);
        return spec;
    }
    if (choice.kind == "cpmlwell" || choice.kind == "levelwell" || choice.kind == "lwell") {
        spec.kind = "level_well";
        spec.weights = equal_weights(component_count);
        spec.parameters = choice.parameters.empty() ? std::vector<double>{0.1, 1.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == "cpmpmean" || choice.kind == "powermean") {
        spec.kind = "power_mean";
        spec.weights = equal_weights(component_count);
        spec.parameters = choice.parameters.empty() ? std::vector<double>{2.0} : choice.parameters;
        return spec;
    }
    if (choice.kind == "dpmsoftmax" || choice.kind == "dpm" || choice.kind == "softmax") {
        spec.kind = "deceptive_softmax";
        spec.centers = centers;
        spec.offsets = offsets;
        spec.parameters = choice.parameters.empty() ? std::vector<double>{1.0, 0.0} : choice.parameters;
        return spec;
    }
    throw std::invalid_argument("unknown composition kind in suite spec: " + choice.kind);
}

std::vector<ChoiceSpec> default_base_transform_choices() {
    return {
        make_choice_spec("rotation", 0.5),
        make_choice_spec("affine", 0.5),
    };
}

std::vector<ChoiceSpec> default_coord_transform_choices() {
    return {
        make_choice_spec("rotation", 0.35),
        make_choice_spec("affine", 0.35),
        make_choice_spec("blockrotation", 0.30),
    };
}

std::vector<ChoiceSpec> default_value_transform_choices() {
    return {
        make_choice_spec("coszero", 0.34),
        make_choice_spec("osc", 0.33),
        make_choice_spec("power", 0.33),
    };
}

std::vector<ChoiceSpec> default_composition_choices() {
    return {
        make_choice_spec("cpmsum", 0.34),
        make_choice_spec("cpmlwell", 0.33),
        make_choice_spec("dpmsoftmax", 0.33),
    };
}

std::vector<BasicFunctionId> sample_base_functions(
    const std::vector<BasicFunctionId>& pool,
    int count,
    std::mt19937_64& rng) {
    require(count > 0, "sample count must be positive");
    require(static_cast<std::size_t>(count) <= pool.size(), "sample count exceeds available base functions");
    std::vector<BasicFunctionId> selected = pool;
    stable_shuffle(selected, rng);
    selected.resize(static_cast<std::size_t>(count));
    return selected;
}

std::vector<std::vector<int>> partition_indices(int dimension, int parts, std::mt19937_64& rng) {
    require(dimension > 0, "partition dimension must be positive");
    require(parts > 0, "partition parts must be positive");
    require(parts <= dimension, "partition parts must not exceed dimension");

    std::vector<int> indices(static_cast<std::size_t>(dimension));
    for (int i = 0; i < dimension; ++i) {
        indices[static_cast<std::size_t>(i)] = i;
    }
    stable_shuffle(indices, rng);

    std::vector<std::vector<int>> blocks;
    blocks.reserve(static_cast<std::size_t>(parts));

    const int base_size = dimension / parts;
    const int remainder = dimension % parts;
    std::size_t offset = 0;
    for (int i = 0; i < parts; ++i) {
        const int block_size = base_size + (i < remainder ? 1 : 0);
        require(block_size > 0, "partition block size must be positive");
        std::vector<int> block;
        block.reserve(static_cast<std::size_t>(block_size));
        for (int j = 0; j < block_size; ++j) {
            block.push_back(indices[offset + static_cast<std::size_t>(j)]);
        }
        offset += static_cast<std::size_t>(block_size);
        std::sort(block.begin(), block.end());
        blocks.push_back(std::move(block));
    }
    return blocks;
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

std::uint64_t permutation_count(int n, int k) {
    if (k < 0 || k > n) {
        return 0;
    }
    std::uint64_t result = 1;
    for (int i = 0; i < k; ++i) {
        result = saturating_mul(result, static_cast<std::uint64_t>(n - i));
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
      supported_dimensions_(normalize_dimensions(spec_)) {
    require(dimension_ > 0, "suite dimension must be positive");
    require(supports_dimension(dimension_), "suite dimension is not supported by the suite spec");

    const std::vector<BasicFunctionId> mandatory_base_functions = normalize_base_functions(spec_.base_functions);
    const std::vector<BasicFunctionId> composition_pool = normalize_composition_base_functions(
        spec_.base_functions_for_compositions,
        mandatory_base_functions);

    const int coord_family_count = static_cast<int>(normalize_choices(spec_.coord_transforms, default_coord_transform_choices()).size());
    const int value_family_count = static_cast<int>(normalize_choices(spec_.value_transforms, default_value_transform_choices()).size());
    const int composition_family_count = static_cast<int>(normalize_choices(spec_.composition_functions, default_composition_choices()).size());
    const int max_components = std::min<int>(6, static_cast<int>(composition_pool.size()));
    std::uint64_t theoretical_composed = 0;
    for (int component_count = 2; component_count <= max_components; ++component_count) {
        const std::uint64_t component_orders = permutation_count(static_cast<int>(composition_pool.size()), component_count);
        const std::uint64_t coord_choices = saturating_pow(static_cast<std::uint64_t>(coord_family_count), component_count);
        const std::uint64_t value_choices = saturating_pow(static_cast<std::uint64_t>(value_family_count), component_count);
        const std::uint64_t choices = saturating_mul(component_orders, saturating_mul(coord_choices, value_choices));
        theoretical_composed = saturating_add(theoretical_composed, saturating_mul(choices, static_cast<std::uint64_t>(composition_family_count)));
    }
    theoretical_max_number_of_functions_ = saturating_add(
        static_cast<std::uint64_t>(mandatory_base_functions.size()),
        theoretical_composed);

    const int mandatory_count = static_cast<int>(mandatory_base_functions.size());
    const int requested_count = spec_.requested_number_of_functions > 0
        ? spec_.requested_number_of_functions
        : mandatory_count;
    require(requested_count >= mandatory_count, "requested_number_of_functions is smaller than the mandatory base-function count");

    const auto base_choices = normalize_choices(spec_.base_function_coord_transforms, default_base_transform_choices());
    const auto coord_choices = normalize_choices(spec_.coord_transforms, default_coord_transform_choices());
    const auto value_choices = normalize_choices(spec_.value_transforms, default_value_transform_choices());
    const auto composition_choices = normalize_choices(spec_.composition_functions, default_composition_choices());
    if (requested_count > mandatory_count) {
        require(composition_pool.size() >= 2, "composition base-function pool must contain at least two functions");
    }

    blueprints_.reserve(static_cast<std::size_t>(requested_count));
    std::uint64_t sequence = 0;

    for (BasicFunctionId id : mandatory_base_functions) {
        std::mt19937_64 rng(mix_seed(spec_.master_seed + sequence + 0x1000ull));
        FunctionBlueprint blueprint;
        blueprint.composed = false;
        blueprint.base_function = id;
        blueprint.base_transform_choice = choose_weighted(base_choices, rng);
        blueprint.seed = mix_seed(spec_.master_seed + sequence + 0x2000ull);
        blueprints_.push_back(std::move(blueprint));
        ++sequence;
    }

    int attempts = 0;
    const int max_attempts = std::max(1000, requested_count * 500);
    while (static_cast<int>(blueprints_.size()) < requested_count) {
        if (++attempts > max_attempts) {
            throw std::runtime_error("could not generate enough unique benchmark-suite blueprints");
        }

        std::mt19937_64 rng(mix_seed(spec_.master_seed + sequence + 0x3000ull));
        const ChoiceSpec& comp_choice = choose_weighted(composition_choices, rng);
        const int max_components = std::min<int>(6, static_cast<int>(composition_pool.size()));
        const int component_count = max_components == 2 ? 2 : uniform_int(rng, 2, max_components);

        FunctionBlueprint blueprint;
        blueprint.composed = true;
        blueprint.component_count = component_count;
        blueprint.component_bases = sample_base_functions(composition_pool, component_count, rng);
        blueprint.coord_transform_choices.reserve(static_cast<std::size_t>(component_count));
        blueprint.value_transform_choices.reserve(static_cast<std::size_t>(component_count));
        for (int i = 0; i < component_count; ++i) {
            blueprint.coord_transform_choices.push_back(choose_weighted(coord_choices, rng));
            blueprint.value_transform_choices.push_back(choose_weighted(value_choices, rng));
        }
        blueprint.composition_choice = comp_choice;
        blueprint.seed = mix_seed(spec_.master_seed + sequence + 0x4000ull);

        blueprints_.push_back(std::move(blueprint));
        ++sequence;
    }

    spec_.max_number_of_functions = static_cast<int>(blueprints_.size());
    std::cout << "BenchmarkSuite generated. Requested functions: " << requested_count
              << ", theoretical max functions: " << theoretical_max_number_of_functions_
              << ", generated functions: " << max_number_of_functions()
              << ", dimension: " << dimension_ << '\n';
}

bool BenchmarkSuite::supports_dimension(int dimension) const {
    return std::find(supported_dimensions_.begin(), supported_dimensions_.end(), dimension) != supported_dimensions_.end();
}

BenchmarkFunction BenchmarkSuite::build_function(const FunctionBlueprint& blueprint) const {
    require(dimension_ > 0, "dimension must be positive");

    FunctionBuilder builder(dimension_);
    builder.domain(Domain(dimension_, spec_.lower_bound, spec_.upper_bound));
    builder.seed(blueprint.seed);
    builder.known_global_value(spec_.f_opt);
    builder.parameter("suite_label", spec_.suite_label);

    const Domain domain(dimension_, spec_.lower_bound, spec_.upper_bound);
    std::mt19937_64 rng(mix_seed(blueprint.seed ^ static_cast<std::uint64_t>(dimension_)));

    if (!blueprint.composed) {
        const auto x_star = random_point_in_domain(rng, domain);
        builder.known_global_minimizer(x_star);
        builder.parameter("suite_role", "base");

        const std::vector<double> zero(static_cast<std::size_t>(dimension_), 0.0);
        builder.add_component(
            blueprint.base_function,
            dimension_,
            make_coordinate_transform(
                make_coordinate_transform_spec(
                    blueprint.base_transform_choice,
                    dimension_,
                    x_star,
                    zero,
                    blueprint.seed,
                    rng,
                    false,
                    nullptr)),
            std::make_shared<IdentityValueTransform>())
            .composition(std::make_shared<SingleComponentComposition>());

        return BenchmarkFunction(builder.build_spec());
    }

    const auto x_star = (normalize_token(blueprint.composition_choice.kind) == "dpmsoftmax"
        || normalize_token(blueprint.composition_choice.kind) == "dpm"
        || normalize_token(blueprint.composition_choice.kind) == "softmax")
        ? random_point_in_domain_away_from_origin(rng, domain, 5.0)
        : random_point_in_domain(rng, domain);

    std::vector<std::vector<double>> centers(static_cast<std::size_t>(blueprint.component_count), x_star);
    std::vector<double> offsets(static_cast<std::size_t>(blueprint.component_count), 0.0);
    const bool uses_block_rotation = std::any_of(
        blueprint.coord_transform_choices.begin(),
        blueprint.coord_transform_choices.end(),
        [](const ChoiceSpec& choice) {
            return choice.kind == "brot" || choice.kind == "blockrot" || choice.kind == "blockrotation";
        });
    std::vector<std::vector<int>> block_partitions;
    if (uses_block_rotation) {
        require(blueprint.component_count <= dimension_, "block rotation composition requires dimension at least the number of components");
        block_partitions = partition_indices(dimension_, blueprint.component_count, rng);
    }
    if (normalize_token(blueprint.composition_choice.kind) == "dpmsoftmax"
        || normalize_token(blueprint.composition_choice.kind) == "dpm"
        || normalize_token(blueprint.composition_choice.kind) == "softmax") {
        const double deceptive_step = 10.0 + 0.1 * spec_.f_opt;
        for (int i = 1; i < blueprint.component_count; ++i) {
            centers[static_cast<std::size_t>(i)] = random_point_in_domain(rng, domain);
            offsets[static_cast<std::size_t>(i)] = deceptive_step * static_cast<double>(i);
        }
    }

    builder.known_global_minimizer(x_star);
    builder.parameter("suite_role", "composed");

    const std::vector<double> zero(static_cast<std::size_t>(dimension_), 0.0);
    for (int i = 0; i < blueprint.component_count; ++i) {
        const std::vector<double>& source = centers[static_cast<std::size_t>(i)];
        const std::uint64_t component_seed = mix_seed(blueprint.seed + static_cast<std::uint64_t>(i) + 1ULL);
        builder.add_component(
            blueprint.component_bases[static_cast<std::size_t>(i)],
            dimension_,
            make_coordinate_transform(
                make_coordinate_transform_spec(
                    blueprint.coord_transform_choices[static_cast<std::size_t>(i)],
                    dimension_,
                    source,
                    zero,
                    component_seed,
                    rng,
                    true,
                    uses_block_rotation ? &block_partitions[static_cast<std::size_t>(i)] : nullptr)),
            make_value_transform(
                make_value_transform_spec(
                    blueprint.value_transform_choices[static_cast<std::size_t>(i)])));
    }

    builder.composition(make_composition(
        make_composition_spec(
            blueprint.composition_choice,
            blueprint.component_count,
            centers,
            offsets),
        static_cast<std::size_t>(blueprint.component_count)));

    return BenchmarkFunction(builder.build_spec());
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

BenchmarkFunction BenchmarkSuite::function(int index) const {
    require(index >= 0 && index < size(), "benchmark function index out of range");
    return build_function(blueprints_[static_cast<std::size_t>(index)]);
}

std::vector<double> BenchmarkSuite::operator()(int index, const std::vector<std::vector<double>>& X) const {
    return function(index)(X);
}

const SuiteSpec& BenchmarkSuite::spec() const {
    return spec_;
}

BenchmarkSuite make_benchmark_suite(SuiteSpec spec, int dimension) {
    return BenchmarkSuite(std::move(spec), dimension);
}

} // namespace FuncCraft
