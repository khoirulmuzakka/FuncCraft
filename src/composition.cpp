#include "composition.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {

void require_nonnegative_weights(const std::vector<double>& weights) {
    for (double weight : weights) {
        require(weight >= 0.0, "weights must be nonnegative");
    }
}

double weighted_sum_unchecked(const std::vector<double>& weights, const std::vector<double>& z) {
    double result = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        result += weights[i] * z[i];
    }
    return result;
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
    require(p > 0.0, "power mean exponent must be positive");
}

PowerMeanComposition::PowerMeanComposition(std::size_t components, double p)
    : weights_(components, 1.0),
      p_(p) {
    require(components > 0, "power mean needs at least one component");
    require(p > 0.0, "power mean exponent must be positive");
}

double PowerMeanComposition::common_raw_apply(const std::vector<double>& z) const {
    require(weights_.size() == z.size(), "weight/component size mismatch");
    double sum = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        require(z[i] >= 0.0, "power mean components must be nonnegative");
        sum += weights_[i] * std::pow(z[i], p_);
    }
    return std::pow(sum, 1.0 / p_);
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
    require(epsilon >= 0.0 && epsilon < 1.0, "level-well epsilon must be in [0, 1)");
    require(alpha >= 0.0, "level-well alpha must be nonnegative");
}

LevelWellComposition::LevelWellComposition(std::size_t components, double epsilon, double alpha)
    : weights_(components, 1.0),
      epsilon_(epsilon),
      alpha_(alpha) {
    require(components > 0, "level well needs at least one component");
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

DeceptiveSoftmaxComposition::DeceptiveSoftmaxComposition(
    std::vector<std::vector<double>> centers,
    double sharpness,
    std::vector<double> biases)
    : centers_(std::move(centers)),
      biases_(std::move(biases)),
      sharpness_(sharpness) {
    require(!centers_.empty(), "deceptive softmax needs at least one center");
    require(biases_.empty() || biases_.size() == centers_.size(), "deceptive softmax bias/center size mismatch");
    require(sharpness_ >= 0.0, "softmax sharpness must be nonnegative");
    const std::size_t dimension = centers_.front().size();
    for (const auto& center : centers_) {
        require(center.size() == dimension, "deceptive centers must have common dimension");
    }
}

double DeceptiveSoftmaxComposition::deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(z.size() == centers_.size(), "deceptive component size mismatch");
    require(x.size() == centers_.front().size(), "deceptive point dimension mismatch");

    double max_logit = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        max_logit = std::max(max_logit, -sharpness_ * squared_distance(x, centers_[i]));
    }

    const double optimum_distance_sq = squared_distance(x, centers_.front());
    const double selective_mask = 1.0 - std::exp(-optimum_distance_sq);

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        double w = std::exp((-sharpness_ * squared_distance(x, centers_[i])) - max_logit);
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
    require(sharpness_ >= 0.0, "softmax sharpness must be nonnegative");
    require(background_strength_ >= 0.0, "background strength must be nonnegative");
    require(background_sharpness_ >= 0.0, "background sharpness must be nonnegative");
    const std::size_t dimension = centers_.front().size();
    for (const auto& center : centers_) {
        require(center.size() == dimension, "deceptive bg centers must have common dimension");
    }
}

double DeceptiveBgSoftmaxComposition::deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(z.size() == centers_.size(), "deceptive bg component size mismatch");
    require(x.size() == centers_.front().size(), "deceptive bg point dimension mismatch");

    double max_logit = -std::numeric_limits<double>::infinity();
    double min_distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        const double distance_sq = squared_distance(x, centers_[i]);
        max_logit = std::max(max_logit, -sharpness_ * distance_sq);
        min_distance = std::min(min_distance, std::sqrt(distance_sq));
    }
    const double background = background_strength_ * (1.0 - std::exp(-background_sharpness_ * min_distance));
    const double optimum_distance_sq = squared_distance(x, centers_.front());
    const double selective_mask = 1.0 - std::exp(-optimum_distance_sq);

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        double w = std::exp((-sharpness_ * squared_distance(x, centers_[i])) - max_logit);
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
