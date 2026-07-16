#include "composition.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace FuncCraft {
using namespace detail;

double SingleComponentComposition::apply(const std::vector<double>&, const std::vector<double>& z) const {
    require(z.size() == 1, "single-component composition requires exactly one component");
    return z.front();
}

CompositionClass SingleComponentComposition::composition_class() const {
    return CompositionClass::None;
}

WeightedSumComposition::WeightedSumComposition(std::vector<double> weights)
    : weights_(std::move(weights)) {
    require(!weights_.empty(), "weighted sum needs at least one weight");
}

double WeightedSumComposition::apply(const std::vector<double>&, const std::vector<double>& z) const {
    return weighted_sum(weights_, z);
}

CompositionClass WeightedSumComposition::composition_class() const {
    return CompositionClass::CommonPointWeightedSum;
}

PowerMeanComposition::PowerMeanComposition(std::vector<double> weights, double p)
    : weights_(std::move(weights)),
      p_(p) {
    require(!weights_.empty(), "power mean needs at least one weight");
    require(p > 0.0, "power mean exponent must be positive");
}

double PowerMeanComposition::apply(const std::vector<double>&, const std::vector<double>& z) const {
    require(weights_.size() == z.size(), "weight/component size mismatch");
    double sum = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        require(weights_[i] >= 0.0, "weights must be nonnegative");
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
    require(epsilon >= 0.0 && epsilon < 1.0, "level-well epsilon must be in [0, 1)");
    require(alpha >= 0.0, "level-well alpha must be nonnegative");
}

double LevelWellComposition::apply(const std::vector<double>&, const std::vector<double>& z) const {
    const double s = weighted_sum(weights_, z);
    return s * (1.0 + epsilon_ * std::sin(alpha_ * s));
}

CompositionClass LevelWellComposition::composition_class() const {
    return CompositionClass::CommonPointLevelWell;
}

DeceptiveSoftmaxComposition::DeceptiveSoftmaxComposition(
    std::vector<std::vector<double>> centers,
    std::vector<double> offsets,
    double sharpness)
    : centers_(std::move(centers)),
      offsets_(std::move(offsets)),
      sharpness_(sharpness) {
    require(!centers_.empty(), "deceptive softmax needs at least one center");
    require(centers_.size() == offsets_.size(), "deceptive offsets must match centers");
    require(sharpness_ >= 0.0, "softmax sharpness must be nonnegative");
    const std::size_t dimension = centers_.front().size();
    for (const auto& center : centers_) {
        require(center.size() == dimension, "deceptive centers must have common dimension");
    }
}

DeceptiveSoftmaxComposition::DeceptiveSoftmaxComposition(
    std::vector<std::vector<double>> centers,
    std::vector<double> offsets,
    double sharpness,
    double local_selection_radius)
    : DeceptiveSoftmaxComposition(std::move(centers), std::move(offsets), sharpness) {
    require(local_selection_radius >= 0.0, "local selection radius must be nonnegative");
    local_selection_radius_ = local_selection_radius;
}

double DeceptiveSoftmaxComposition::apply(const std::vector<double>& x, const std::vector<double>& z) const {
    require(z.size() == centers_.size(), "deceptive component size mismatch");
    require(x.size() == centers_.front().size(), "deceptive point dimension mismatch");

    if (local_selection_radius_ > 0.0) {
        const double radius_sq = local_selection_radius_ * local_selection_radius_;
        for (std::size_t i = 0; i < centers_.size(); ++i) {
            if (squared_distance(x, centers_[i]) <= radius_sq) {
                return z[i] + offsets_[i];
            }
        }
    }

    std::vector<double> logits(centers_.size(), 0.0);
    double max_logit = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        logits[i] = -sharpness_ * squared_distance(x, centers_[i]);
        max_logit = std::max(max_logit, logits[i]);
    }

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = 0; i < centers_.size(); ++i) {
        const double w = std::exp(logits[i] - max_logit);
        numerator += w * (z[i] + offsets_[i]);
        denominator += w;
    }
    return numerator / denominator;
}

CompositionClass DeceptiveSoftmaxComposition::composition_class() const {
    return CompositionClass::DeceptivePointSoftmax;
}

ComposedFunction::ComposedFunction(
    Domain domain,
    std::vector<Component> components,
    std::shared_ptr<CompositionFunction> composition,
    FunctionMetadata metadata)
    : domain_(std::move(domain)),
      components_(std::move(components)),
      composition_(std::move(composition)),
      metadata_(std::move(metadata)) {
    require(domain_.dimension() > 0, "composed function domain must have positive dimension");
    require(!components_.empty(), "composed function needs at least one component");
    require(static_cast<bool>(composition_), "composed function needs a composition");
    for (const auto& component : components_) {
        require(static_cast<bool>(component.base), "component base function is null");
        require(static_cast<bool>(component.coordinate_transform), "component coordinate transform is null");
        require(static_cast<bool>(component.value_transform), "component value transform is null");
        require(component.coordinate_transform->input_dimension() == domain_.dimension(), "component transform input dimension mismatch");
        require(component.coordinate_transform->output_dimension() == component.base->dimension, "component transform output dimension mismatch");
    }
}

double ComposedFunction::evaluate_single(const std::vector<double>& x) const {
    require_dimension(x, domain_.dimension(), "composed function input");
    return metadata_.known_global_value + composition_->apply(x, component_values(x));
}

std::vector<double> ComposedFunction::component_values(const std::vector<double>& x) const {
    require_dimension(x, domain_.dimension(), "composed function input");
    std::vector<double> z;
    z.reserve(components_.size());
    for (const auto& component : components_) {
        const std::vector<double> y = component.coordinate_transform->apply(x);
        const double u = (*component.base)(std::vector<std::vector<double>>{y}).front();
        z.push_back(component.value_transform->apply(u));
    }
    return z;
}

std::vector<double> ComposedFunction::operator()(const std::vector<std::vector<double>>& X) const {
    std::vector<double> values;
    values.reserve(X.size());
    for (const auto& x : X) {
        values.push_back(evaluate_single(x));
    }
    return values;
}

const FunctionMetadata& ComposedFunction::metadata() const {
    return metadata_;
}

const Domain& ComposedFunction::domain() const {
    return domain_;
}

int ComposedFunction::dimension() const {
    return domain_.dimension();
}

} // namespace FuncCraft
