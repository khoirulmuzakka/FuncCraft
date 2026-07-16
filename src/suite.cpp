#include "suite.h"
#include "builder.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;
namespace {

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

std::vector<double> select_center(const std::vector<double>& center, const std::vector<int>& indices) {
    std::vector<double> selected;
    selected.reserve(indices.size());
    for (int idx : indices) {
        selected.push_back(center[static_cast<std::size_t>(idx)]);
    }
    return selected;
}

std::shared_ptr<CoordinateTransform> make_suite_coordinate_transform(
    CoordinateTransformClass cls,
    int dimension,
    const std::vector<double>& x_star,
    std::mt19937_64& rng) {
    switch (cls) {
    case CoordinateTransformClass::None:
        return std::make_shared<IdentityTransform>(dimension);
    case CoordinateTransformClass::Rotation:
        return std::make_shared<RotationTransform>(random_rotation_matrix(rng, dimension), x_star);
    case CoordinateTransformClass::Affine:
        return std::make_shared<AffineTransform>(
            random_affine_matrix(rng, dimension),
            x_star,
            std::vector<double>(static_cast<std::size_t>(dimension), 0.0));
    case CoordinateTransformClass::NonlinearFold:
        return std::make_shared<FoldTransform>(dimension, 0);
    default:
        throw std::invalid_argument("unsupported suite coordinate transform");
    }
}

std::vector<std::vector<int>> random_disjoint_blocks(std::mt19937_64& rng, int dimension, int block_count) {
    require(block_count > 0, "block count must be positive");
    require(block_count <= dimension, "block count must not exceed dimension");

    std::vector<int> indices(static_cast<std::size_t>(dimension), 0);
    std::iota(indices.begin(), indices.end(), 0);
    stable_shuffle(indices, rng);

    std::vector<std::vector<int>> blocks(static_cast<std::size_t>(block_count));
    for (int i = 0; i < dimension; ++i) {
        blocks[static_cast<std::size_t>(i % block_count)].push_back(indices[static_cast<std::size_t>(i)]);
    }
    return blocks;
}

std::shared_ptr<ValueTransform> make_suite_value_transform(ValueTransformClass cls, std::mt19937_64& rng) {
    switch (cls) {
    case ValueTransformClass::None:
        return std::make_shared<IdentityValueTransform>();
    case ValueTransformClass::CosineZero:
        return std::make_shared<CosineZeroValueTransform>(0.05 + 0.15 * uniform01(rng));
    case ValueTransformClass::Oscillatory:
        return std::make_shared<OscillatoryValueTransform>(0.2 + 0.5 * uniform01(rng), 0.01 + 0.05 * uniform01(rng));
    case ValueTransformClass::Power:
        return std::make_shared<PowerValueTransform>(1.0, 0.35 + 1.65 * uniform01(rng));
    default:
        throw std::invalid_argument("unsupported suite value transform");
    }
}

std::vector<double> equal_weights(int count) {
    return std::vector<double>(static_cast<std::size_t>(count), 1.0);
}

std::shared_ptr<CompositionFunction> make_suite_composition(
    CompositionClass cls,
    int components,
    int dimension,
    const std::vector<std::vector<double>>& prescribed_centers,
    const std::vector<double>& prescribed_offsets,
    std::mt19937_64& rng) {
    switch (cls) {
    case CompositionClass::None:
        return std::make_shared<SingleComponentComposition>();
    case CompositionClass::CommonPointWeightedSum:
        return std::make_shared<WeightedSumComposition>(equal_weights(components));
    case CompositionClass::CommonPointPowerMean:
        return std::make_shared<PowerMeanComposition>(equal_weights(components), 1.5 + 2.5 * uniform01(rng));
    case CompositionClass::CommonPointLevelWell:
        return std::make_shared<LevelWellComposition>(equal_weights(components), 0.2 + 0.55 * uniform01(rng), 0.01 + 0.06 * uniform01(rng));
    case CompositionClass::DeceptivePointSoftmax: {
        require(static_cast<int>(prescribed_centers.size()) == components, "DPM centers must match component count");
        require(static_cast<int>(prescribed_offsets.size()) == components, "DPM offsets must match component count");
        (void)dimension;
        return std::make_shared<DeceptiveSoftmaxComposition>(
            prescribed_centers,
            prescribed_offsets,
            0.01 + 0.05 * uniform01(rng),
            1.0);
    }
    default:
        throw std::invalid_argument("unsupported suite composition");
    }
}

std::string suite_key(
    CompositionClass c,
    ValueTransformClass p,
    CoordinateTransformClass t,
    const std::vector<BasicFunctionId>& bases) {
    std::vector<BasicFunctionId> sorted_bases = bases;
    std::sort(sorted_bases.begin(), sorted_bases.end(), [](BasicFunctionId a, BasicFunctionId b) {
        return static_cast<int>(a) < static_cast<int>(b);
    });
    std::ostringstream out;
    out << to_string(c) << "|" << to_string(p) << "|" << to_string(t) << "|";
    for (BasicFunctionId id : sorted_bases) {
        out << static_cast<int>(id) << ",";
    }
    return out.str();
}

std::vector<BasicFunctionId> random_base_set(std::mt19937_64& rng, int count) {
    auto all = list_basic_functions();
    stable_shuffle(all, rng);

    std::vector<BasicFunctionId> chosen;
    chosen.reserve(static_cast<std::size_t>(count));
    bool has_multimodal = false;
    for (BasicFunctionId id : all) {
        if (static_cast<int>(chosen.size()) >= count) {
            break;
        }
        chosen.push_back(id);
        has_multimodal = has_multimodal || is_multimodal(id);
    }

    if (!has_multimodal) {
        const auto multimodal = multimodalF_list();
        chosen.back() = multimodal[static_cast<std::size_t>(uniform_int(rng, 0, static_cast<int>(multimodal.size()) - 1))];
    }
    return chosen;
}

CompositionClass random_general_composition_class(std::mt19937_64& rng) {
    const double u = uniform01(rng);
    if (u < 0.5) {
        return CompositionClass::DeceptivePointSoftmax;
    }
    if (u < 0.5 + 1.0 / 6.0) {
        return CompositionClass::CommonPointWeightedSum;
    }
    if (u < 0.5 + 2.0 / 6.0) {
        return CompositionClass::CommonPointPowerMean;
    }
    return CompositionClass::CommonPointLevelWell;
}

ComposedFunction make_single_rotated_basic(
    BasicFunctionId id,
    const Domain& domain,
    const std::vector<double>& x_star,
    double assigned_fmin,
    unsigned long long seed,
    const std::string& family) {
    const int dimension = domain.dimension();
    std::mt19937_64 rng(mix_seed(seed));
    FunctionBuilder builder(dimension);
    builder.domain(domain)
        .seed(seed)
        .known_global_minimizer(x_star)
        .known_global_value(assigned_fmin)
        .parameter("assigned_fmin", std::to_string(assigned_fmin))
        .parameter("suite_family", family)
        .add_component(
            id,
            dimension,
            std::make_shared<RotationTransform>(random_rotation_matrix(rng, dimension), x_star),
            std::make_shared<IdentityValueTransform>())
        .composition(std::make_shared<SingleComponentComposition>());
    return builder.build();
}

ComposedFunction make_single_unimodal_transform(
    BasicFunctionId id,
    ValueTransformClass p,
    const Domain& domain,
    const std::vector<double>& x_star,
    double assigned_fmin,
    unsigned long long seed) {
    const int dimension = domain.dimension();
    std::mt19937_64 rng(mix_seed(seed));
    FunctionBuilder builder(dimension);
    builder.domain(domain)
        .seed(seed)
        .known_global_minimizer(x_star)
        .known_global_value(assigned_fmin)
        .parameter("assigned_fmin", std::to_string(assigned_fmin))
        .parameter("suite_family", "unimodal-value-transform")
        .add_component(
            id,
            dimension,
            std::make_shared<RotationTransform>(random_rotation_matrix(rng, dimension), x_star),
            make_suite_value_transform(p, rng))
        .composition(std::make_shared<SingleComponentComposition>());
    return builder.build();
}

ComposedFunction make_general_suite_function(
    const std::vector<BasicFunctionId>& bases,
    CompositionClass c,
    ValueTransformClass p,
    CoordinateTransformClass t,
    const Domain& domain,
    const std::vector<double>& x_star,
    double assigned_fmin,
    unsigned long long seed) {
    const int dimension = domain.dimension();
    std::mt19937_64 rng(mix_seed(seed));
    FunctionBuilder builder(dimension);
    builder.domain(domain)
        .seed(seed)
        .known_global_minimizer(x_star)
        .known_global_value(assigned_fmin)
        .parameter("assigned_fmin", std::to_string(assigned_fmin))
        .parameter("suite_family", "general-composition");

    std::vector<std::vector<double>> prescribed_centers(
        bases.size(),
        x_star);
    std::vector<double> prescribed_offsets(bases.size(), 0.0);
    if (c == CompositionClass::DeceptivePointSoftmax) {
        const double deceptive_step = 10.0 + 0.1 * assigned_fmin;
        require(deceptive_step > 0.0, "DPM deceptive bias increment must be positive");
        prescribed_centers[0] = x_star;
        prescribed_offsets[0] = 0.0;
        if (bases.size() >= 2) {
            prescribed_centers[1].assign(static_cast<std::size_t>(dimension), 0.0);
            prescribed_offsets[1] = deceptive_step;
        }
        for (std::size_t i = 2; i < bases.size(); ++i) {
            prescribed_centers[i] = random_point_in_domain(rng, domain);
            prescribed_offsets[i] = deceptive_step * static_cast<double>(i);
        }
        builder.parameter("dpm_origin_deceptive_index", "1");
        builder.parameter("dpm_bias_increment", std::to_string(deceptive_step));
    }

    std::vector<std::vector<int>> blocks;
    if (t == CoordinateTransformClass::Block) {
        blocks = random_disjoint_blocks(rng, dimension, static_cast<int>(bases.size()));
    }

    for (std::size_t i = 0; i < bases.size(); ++i) {
        const BasicFunctionId id = bases[i];
        std::shared_ptr<CoordinateTransform> coordinate_transform;
        int component_dimension = dimension;
        if (t == CoordinateTransformClass::Block) {
            coordinate_transform = std::make_shared<SubspaceTransform>(dimension, blocks[i], select_center(prescribed_centers[i], blocks[i]));
            component_dimension = static_cast<int>(blocks[i].size());
        } else {
            coordinate_transform = make_suite_coordinate_transform(t, dimension, prescribed_centers[i], rng);
        }

        builder.add_component(
            id,
            component_dimension,
            coordinate_transform,
            make_suite_value_transform(p, rng));
    }
    builder.composition(make_suite_composition(
        c,
        static_cast<int>(bases.size()),
        dimension,
        prescribed_centers,
        prescribed_offsets,
        rng));
    return builder.build();
}

} // namespace

int BenchmarkSuite::mandatory_function_count() {
    return static_cast<int>(list_basic_functions().size())
        + 2 * static_cast<int>(unimodalF_list().size());
}

BenchmarkSuite::BenchmarkSuite(BenchmarkSuiteOptions options)
    : options_(std::move(options)) {
    require(options_.function_count >= mandatory_function_count(), "function_count is smaller than the mandatory FuncCraft suite prefix");
    require(options_.dimension >= 2, "suite dimension must be at least two");
    require(options_.lower_bound <= options_.upper_bound, "suite lower bound must not exceed upper bound");
    require(std::isfinite(options_.assigned_fmin), "assigned_fmin must be finite");

    functions_.reserve(static_cast<std::size_t>(options_.function_count));
    const Domain suite_domain(options_.dimension, options_.lower_bound, options_.upper_bound);

    const auto all_bases = list_basic_functions();
    int slot = 0;
    for (BasicFunctionId id : all_bases) {
        std::mt19937_64 point_rng(mix_seed(options_.seed + static_cast<unsigned long long>(slot + 1) + 0x1234ull));
        const auto x_star = random_point_in_domain(point_rng, suite_domain);
        functions_.push_back(make_single_rotated_basic(
            id,
            suite_domain,
            x_star,
            options_.assigned_fmin,
            options_.seed + static_cast<unsigned long long>(slot + 1),
            "rotated-basic"));
        ++slot;
    }

    const auto unimodal = unimodalF_list();
    for (BasicFunctionId id : all_bases) {
        std::mt19937_64 cos_point_rng(mix_seed(options_.seed + static_cast<unsigned long long>(slot + 1) + 0x2345ull));
        const auto cos_x_star = random_point_in_domain(cos_point_rng, suite_domain);
        functions_.push_back(make_single_unimodal_transform(
            id,
            ValueTransformClass::CosineZero,
            suite_domain,
            cos_x_star,
            options_.assigned_fmin,
            options_.seed + static_cast<unsigned long long>(slot + 1)));
        ++slot;

        std::mt19937_64 osc_point_rng(mix_seed(options_.seed + static_cast<unsigned long long>(slot + 1) + 0x3456ull));
        const auto osc_x_star = random_point_in_domain(osc_point_rng, suite_domain);
        functions_.push_back(make_single_unimodal_transform(
            id,
            ValueTransformClass::Oscillatory,
            suite_domain,
            osc_x_star,
            options_.assigned_fmin,
            options_.seed + static_cast<unsigned long long>(slot + 1)));
        ++slot;
    }

    const std::vector<ValueTransformClass> value_transforms = {
        ValueTransformClass::CosineZero,
        ValueTransformClass::Oscillatory,
        ValueTransformClass::Power,
    };
    const std::vector<CoordinateTransformClass> coordinate_transforms = {
        CoordinateTransformClass::Rotation,
        CoordinateTransformClass::Block,
        CoordinateTransformClass::Affine,
    };

    std::set<std::string> used;
    for (const auto& f : functions_) {
        used.insert(class_label(f.metadata().function_class) + "|mandatory|" + std::to_string(slot));
    }

    std::mt19937_64 rng(mix_seed(options_.seed ^ 0x4f1bbcdd3a2d1f7bull));
    int attempts = 0;
    const int max_attempts = std::max(1000, options_.function_count * 500);
    while (static_cast<int>(functions_.size()) < options_.function_count) {
        if (++attempts > max_attempts) {
            throw std::runtime_error("could not generate enough unique FuncCraft benchmark combinations");
        }

        const CompositionClass c = random_general_composition_class(rng);
        const ValueTransformClass p = value_transforms[static_cast<std::size_t>(uniform_int(rng, 0, static_cast<int>(value_transforms.size()) - 1))];
        const CoordinateTransformClass t = coordinate_transforms[static_cast<std::size_t>(uniform_int(rng, 0, static_cast<int>(coordinate_transforms.size()) - 1))];
        const int component_count = uniform_int(rng, 2, std::min(6, options_.dimension));
        auto bases = random_base_set(rng, component_count);
        const std::string key = suite_key(c, p, t, bases);
        if (!used.insert(key).second) {
            continue;
        }

        std::mt19937_64 point_rng(mix_seed(options_.seed + static_cast<unsigned long long>(functions_.size() + 1) + 0x4567ull));
        const auto x_star = c == CompositionClass::DeceptivePointSoftmax
            ? random_point_in_domain_away_from_origin(point_rng, suite_domain, 5.0)
            : random_point_in_domain(point_rng, suite_domain);
        functions_.push_back(make_general_suite_function(
            bases,
            c,
            p,
            t,
            suite_domain,
            x_star,
            options_.assigned_fmin,
            options_.seed + static_cast<unsigned long long>(functions_.size() + 1)));
    }
}

int BenchmarkSuite::size() const {
    return static_cast<int>(functions_.size());
}

const ComposedFunction& BenchmarkSuite::function(int index) const {
    require(index >= 0 && index < size(), "benchmark function index out of range");
    return functions_[static_cast<std::size_t>(index)];
}

std::vector<double> BenchmarkSuite::operator()(int index, const std::vector<std::vector<double>>& X) const {
    return function(index)(X);
}

const BenchmarkSuiteOptions& BenchmarkSuite::options() const {
    return options_;
}

const std::vector<ComposedFunction>& BenchmarkSuite::functions() const {
    return functions_;
}

BenchmarkSuite make_benchmark_suite(int function_count, int dimension, unsigned long long seed) {
    BenchmarkSuiteOptions options;
    options.function_count = function_count;
    options.dimension = dimension;
    options.seed = seed;
    return BenchmarkSuite(options);
}

} // namespace FuncCraft
