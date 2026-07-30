#include "composition.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {
constexpr double kCompositionTolerance = 1.0e-12;

void require_nonnegative_weights(const std::vector<double>& weights) {
    for (double weight : weights) {
        require(std::isfinite(weight), "weights must be finite");
        require(weight >= 0.0, "weights must be nonnegative");
    }
}

void require_dpm_biases(const std::vector<double>& biases) {
    if (biases.empty()) {
        return;
    }
    require(std::abs(biases.front()) <= kCompositionTolerance, "first DPM bias must be zero");
    for (double bias : biases) {
        require(std::isfinite(bias), "DPM biases must be finite");
        require(bias >= 0.0, "DPM biases must be nonnegative");
    }
}

double weighted_sum_unchecked(const std::vector<double>& weights, const std::vector<double>& z) {
    double result = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        result += weights[i] * z[i];
    }
    return result;
}

double positive_power(double value, double exponent) {
    require(value >= 0.0, "power input must be nonnegative");
    if (value == 0.0) {
        return 0.0;
    }
    if (exponent == 1.0) {
        return value;
    }
    if (exponent == 2.0) {
        return value * value;
    }
    if (exponent == 0.5) {
        return std::sqrt(value);
    }
    return std::pow(value, exponent);
}

double one_minus_exp_neg(double x) {
    if (std::isnan(x)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    require(x >= 0.0, "exponential argument must be nonnegative");
    if (x == 0.0) {
        return 0.0;
    }
    if (!std::isfinite(x) || x > 745.0) {
        return 1.0;
    }
    const double value = -std::expm1(-x);
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

} // namespace

double CompositionFunction::apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(!z.empty(), "composition requires at least one component");
    require(!x.empty(), "composition point must not be empty");
    return raw_apply(x, z);
}

double CommonPointComposition::raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    (void)x;
    require(!z.empty(), "common-point composition requires at least one component");
    return common_raw_apply(z);
}

double DeceptivePointComposition::raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(!z.empty(), "deceptive composition requires at least one component");
    require(!x.empty(), "deceptive composition point must not be empty");
    return deceptive_raw_apply(x, z);
}

double NoneComposition::common_raw_apply(const std::vector<double>& z) const {
    require(z.size() == 1, "no composition requires exactly one component");
    return z.front();
}

CompositionClass NoneComposition::composition_class() const {
    return CompositionClass::None;
}

WeightedSumComposition::WeightedSumComposition(std::vector<double> weights)
    : weights_(std::move(weights)) {
    require(!weights_.empty(), "weighted sum needs at least one weight");
    require_nonnegative_weights(weights_);
}

WeightedSumComposition::WeightedSumComposition(std::size_t components)
    : weights_(components, 1.0) {
    require(components > 0, "weighted sum needs at least one component");
}

double WeightedSumComposition::common_raw_apply(const std::vector<double>& z) const {
    require(weights_.size() == z.size(), "weight/component size mismatch");
    return weighted_sum_unchecked(weights_, z);
}

CompositionClass WeightedSumComposition::composition_class() const {
    return CompositionClass::CommonPointWeightedSum;
}

PowerMeanComposition::PowerMeanComposition(std::vector<double> weights, double p)
    : weights_(std::move(weights)),
      p_(p) {
    require(!weights_.empty(), "power mean needs at least one weight");
    require_nonnegative_weights(weights_);
    require(std::isfinite(p), "power mean exponent must be finite");
    require(p > 0.0, "power mean exponent must be positive");
}

PowerMeanComposition::PowerMeanComposition(std::size_t components, double p)
    : weights_(components, 1.0),
      p_(p) {
    require(components > 0, "power mean needs at least one component");
    require(std::isfinite(p), "power mean exponent must be finite");
    require(p > 0.0, "power mean exponent must be positive");
}

double PowerMeanComposition::common_raw_apply(const std::vector<double>& z) const {
    require(weights_.size() == z.size(), "weight/component size mismatch");
    double sum = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        require(z[i] >= 0.0, "power mean components must be nonnegative");
        sum += weights_[i] * positive_power(z[i], p_);
    }
    return positive_power(sum, 1.0 / p_);
}

CompositionClass PowerMeanComposition::composition_class() const {
    return CompositionClass::CommonPointPowerMean;
}

LevelWellComposition::LevelWellComposition(std::vector<double> weights, double epsilon, double alpha)
    : weights_(std::move(weights)),
      epsilon_(epsilon),
      alpha_(alpha) {
    require(!weights_.empty(), "level well needs at least one weight");
    require_nonnegative_weights(weights_);
    require(std::isfinite(epsilon), "level-well epsilon must be finite");
    require(std::isfinite(alpha), "level-well alpha must be finite");
    require(epsilon >= 0.0 && epsilon < 1.0, "level-well epsilon must be in [0, 1)");
    require(alpha >= 0.0, "level-well alpha must be nonnegative");
}

LevelWellComposition::LevelWellComposition(std::size_t components, double epsilon, double alpha)
    : weights_(components, 1.0),
      epsilon_(epsilon),
      alpha_(alpha) {
    require(components > 0, "level well needs at least one component");
    require(std::isfinite(epsilon), "level-well epsilon must be finite");
    require(std::isfinite(alpha), "level-well alpha must be finite");
    require(epsilon >= 0.0 && epsilon < 1.0, "level-well epsilon must be in [0, 1)");
    require(alpha >= 0.0, "level-well alpha must be nonnegative");
}

double LevelWellComposition::common_raw_apply(const std::vector<double>& z) const {
    require(weights_.size() == z.size(), "weight/component size mismatch");
    const double s = weighted_sum_unchecked(weights_, z);
    return s * (1.0 + epsilon_ * std::sin(alpha_ * s));
}

CompositionClass LevelWellComposition::composition_class() const {
    return CompositionClass::CommonPointLevelWell;
}

double MaxComposition::common_raw_apply(const std::vector<double>& z) const {
    return *std::max_element(z.begin(), z.end());
}

CompositionClass MaxComposition::composition_class() const {
    return CompositionClass::CommonPointMax;
}

SmoothMaxComposition::SmoothMaxComposition(double beta)
    : beta_(beta) {
    require(std::isfinite(beta), "smoothmax beta must be finite");
    require(beta > 0.0, "smoothmax beta must be positive");
}

double SmoothMaxComposition::common_raw_apply(const std::vector<double>& z) const {
    const double max_z = *std::max_element(z.begin(), z.end());
    double sum = 0.0;
    for (double value : z) {
        sum += std::exp(beta_ * (value - max_z));
    }
    return max_z + (std::log(sum) - std::log(static_cast<double>(z.size()))) / beta_;
}

CompositionClass SmoothMaxComposition::composition_class() const {
    return CompositionClass::CommonPointSmoothMax;
}

ConstraintPenaltyComposition::ConstraintPenaltyComposition(double rho, double p)
    : rho_(rho),
      p_(p) {
    require(std::isfinite(rho), "constraint-penalty rho must be finite");
    require(std::isfinite(p), "constraint-penalty exponent must be finite");
    require(rho >= 0.0, "constraint-penalty rho must be nonnegative");
    require(p > 0.0, "constraint-penalty exponent must be positive");
}

double ConstraintPenaltyComposition::common_raw_apply(const std::vector<double>& z) const {
    double result = z.front();
    for (std::size_t i = 1; i < z.size(); ++i) {
        result += rho_ * positive_power(z[i], p_);
    }
    return result;
}

CompositionClass ConstraintPenaltyComposition::composition_class() const {
    return CompositionClass::CommonPointConstraintPenalty;
}

LexicographicComposition::LexicographicComposition(double decay)
    : decay_(decay) {
    require(std::isfinite(decay), "lexicographic decay must be finite");
    require(decay >= 0.0 && decay <= 1.0, "lexicographic decay must be in [0, 1]");
}

double LexicographicComposition::common_raw_apply(const std::vector<double>& z) const {
    double result = 0.0;
    double weight = 1.0;
    for (double value : z) {
        result += weight * value;
        weight *= decay_;
    }
    return result;
}

CompositionClass LexicographicComposition::composition_class() const {
    return CompositionClass::CommonPointLexicographic;
}

ProductComposition::ProductComposition(double alpha)
    : alpha_(alpha) {
    require(std::isfinite(alpha), "product alpha must be finite");
    require(alpha > 0.0, "product alpha must be positive");
}

double ProductComposition::common_raw_apply(const std::vector<double>& z) const {
    double product = 1.0;
    for (double value : z) {
        product *= 1.0 + alpha_ * value;
        if (!std::isfinite(product)) {
            return std::numeric_limits<double>::max();
        }
    }
    return (product - 1.0) / alpha_;
}

CompositionClass ProductComposition::composition_class() const {
    return CompositionClass::CommonPointProduct;
}

MaxPlusMeanComposition::MaxPlusMeanComposition(double lambda)
    : lambda_(lambda) {
    require(std::isfinite(lambda), "max-plus-mean lambda must be finite");
    require(lambda >= 0.0 && lambda <= 1.0, "max-plus-mean lambda must be in [0, 1]");
}

double MaxPlusMeanComposition::common_raw_apply(const std::vector<double>& z) const {
    const double max_value = *std::max_element(z.begin(), z.end());
    double sum = 0.0;
    for (double value : z) {
        sum += value;
    }
    const double mean = sum / static_cast<double>(z.size());
    return lambda_ * max_value + (1.0 - lambda_) * mean;
}

CompositionClass MaxPlusMeanComposition::composition_class() const {
    return CompositionClass::CommonPointMaxPlusMean;
}

CvarComposition::CvarComposition(double quantile)
    : quantile_(quantile) {
    require(std::isfinite(quantile), "CVaR quantile must be finite");
    require(quantile > 0.0 && quantile <= 1.0, "CVaR quantile must be in (0, 1]");
}

double CvarComposition::common_raw_apply(const std::vector<double>& z) const {
    std::vector<double> sorted = z;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());
    const std::size_t count = std::max<std::size_t>(
        1,
        static_cast<std::size_t>(std::ceil(quantile_ * static_cast<double>(sorted.size()))));
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        sum += sorted[i];
    }
    return sum / static_cast<double>(count);
}

CompositionClass CvarComposition::composition_class() const {
    return CompositionClass::CommonPointCvar;
}

SparseActiveComposition::SparseActiveComposition(double frequency)
    : frequency_(frequency) {
    require(std::isfinite(frequency), "sparse-active frequency must be finite");
    require(frequency >= 0.0, "sparse-active frequency must be nonnegative");
}

double SparseActiveComposition::raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(!x.empty(), "sparse-active composition point must not be empty");
    require(!z.empty(), "sparse-active composition requires at least one component");
    const double coordinate = std::abs(x.front());
    const auto bucket = static_cast<std::uint64_t>(std::floor(frequency_ * coordinate));
    return z[static_cast<std::size_t>(bucket % z.size())];
}

CompositionClass SparseActiveComposition::composition_class() const {
    return CompositionClass::SparseActive;
}

DeceptiveSoftmaxComposition::DeceptiveSoftmaxComposition(
    std::vector<std::vector<double>> centers,
    double sharpness,
    std::vector<double> biases)
    : centers_(std::move(centers)),
      biases_(std::move(biases)),
      sharpness_(sharpness) {
    require(!centers_.empty(), "deceptive softmax needs at least one center");
    require(biases_.empty() || biases_.size() == centers_.size(), "deceptive softmax bias/center size mismatch");
    require_dpm_biases(biases_);
    require(std::isfinite(sharpness_), "softmax sharpness must be finite");
    require(sharpness_ >= 0.0, "softmax sharpness must be nonnegative");
    const std::size_t dimension = centers_.front().size();
    for (const auto& center : centers_) {
        require(center.size() == dimension, "deceptive centers must have common dimension");
        for (double value : center) {
            require(std::isfinite(value), "deceptive centers must be finite");
        }
    }
}

double DeceptiveSoftmaxComposition::deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(z.size() == centers_.size(), "deceptive component size mismatch");
    require(x.size() == centers_.front().size(), "deceptive point dimension mismatch");

    const double effective_sharpness = sharpness_ / static_cast<double>(x.size());
    double max_logit = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        max_logit = std::max(max_logit, -effective_sharpness * squared_distance(x, centers_[i]));
    }

    const double optimum_distance_sq = squared_distance(x, centers_.front());
    const double selective_mask = one_minus_exp_neg(optimum_distance_sq);

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        double w = std::exp((-effective_sharpness * squared_distance(x, centers_[i])) - max_logit);
        if (i > 0) {
            w *= selective_mask;
        }
        numerator += w * (z[i] + (biases_.empty() ? 0.0 : biases_[i]));
        denominator += w;
    }
    return numerator / denominator;
}

CompositionClass DeceptiveSoftmaxComposition::composition_class() const {
    return CompositionClass::DeceptivePointSoftmax;
}

DeceptiveBgSoftmaxComposition::DeceptiveBgSoftmaxComposition(
    std::vector<std::vector<double>> centers,
    double sharpness,
    double background_strength,
    double background_sharpness,
    std::vector<double> biases)
    : centers_(std::move(centers)),
      biases_(std::move(biases)),
      sharpness_(sharpness),
      background_strength_(background_strength),
      background_sharpness_(background_sharpness) {
    require(!centers_.empty(), "deceptive bg softmax needs at least one center");
    require(biases_.empty() || biases_.size() == centers_.size(), "deceptive bg softmax bias/center size mismatch");
    require_dpm_biases(biases_);
    require(std::isfinite(sharpness_), "softmax sharpness must be finite");
    require(std::isfinite(background_strength_), "background strength must be finite");
    require(std::isfinite(background_sharpness_), "background sharpness must be finite");
    require(sharpness_ >= 0.0, "softmax sharpness must be nonnegative");
    require(background_strength_ >= 0.0, "background strength must be nonnegative");
    require(background_sharpness_ >= 0.0, "background sharpness must be nonnegative");
    const std::size_t dimension = centers_.front().size();
    for (const auto& center : centers_) {
        require(center.size() == dimension, "deceptive bg centers must have common dimension");
        for (double value : center) {
            require(std::isfinite(value), "deceptive bg centers must be finite");
        }
    }
}

double DeceptiveBgSoftmaxComposition::deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(z.size() == centers_.size(), "deceptive bg component size mismatch");
    require(x.size() == centers_.front().size(), "deceptive bg point dimension mismatch");

    const double effective_sharpness = sharpness_ / static_cast<double>(x.size());
    double max_logit = -std::numeric_limits<double>::infinity();
    double min_distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        const double distance_sq = squared_distance(x, centers_[i]);
        max_logit = std::max(max_logit, -effective_sharpness * distance_sq);
        min_distance = std::min(min_distance, std::sqrt(distance_sq));
    }
    const double background = background_strength_ * one_minus_exp_neg(background_sharpness_ * min_distance);
    const double optimum_distance_sq = squared_distance(x, centers_.front());
    const double selective_mask = one_minus_exp_neg(optimum_distance_sq);

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        double w = std::exp((-effective_sharpness * squared_distance(x, centers_[i])) - max_logit);
        if (i > 0) {
            w *= selective_mask;
        }
        numerator += (w + background) * (z[i] + (biases_.empty() ? 0.0 : biases_[i]));
        denominator += w + background;
    }
    return numerator / denominator;
}

CompositionClass DeceptiveBgSoftmaxComposition::composition_class() const {
    return CompositionClass::DeceptivePointBgSoftmax;
}

} // namespace FuncCraft
