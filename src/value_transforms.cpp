#include "value_transforms.h"
#include "support.h"

#include <cmath>

namespace FuncCraft {
using namespace detail;

double ValueTransform::apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    const double at_origin = raw_apply(0.0);
    require(std::abs(at_origin) <= 1.0e-12, "value transform must satisfy f(0) = 0");
    const double value = raw_apply(u);
    require(value >= 0.0, "value transform output must be nonnegative");
    return value;
}

double IdentityValueTransform::raw_apply(double u) const {
    return u;
}

ValueTransformClass IdentityValueTransform::transform_class() const {
    return ValueTransformClass::None;
}

ValueTransformSpec IdentityValueTransform::spec() const {
    ValueTransformSpec spec;
    spec.kind = "identity";
    spec.seed = 0;
    return spec;
}

PowerValueTransform::PowerValueTransform(double alpha, double p)
    : alpha_(alpha),
      p_(p) {
    require(alpha >= 0.0, "power transform alpha must be nonnegative");
    require(p > 0.0, "power transform exponent must be positive");
}

double PowerValueTransform::raw_apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return alpha_ * std::pow(u, p_);
}

ValueTransformClass PowerValueTransform::transform_class() const {
    return ValueTransformClass::Power;
}

ValueTransformSpec PowerValueTransform::spec() const {
    ValueTransformSpec spec;
    spec.kind = "power";
    spec.parameters = {alpha_, p_};
    return spec;
}

OscillatoryValueTransform::OscillatoryValueTransform(double epsilon, double alpha)
    : epsilon_(epsilon),
      alpha_(alpha) {
    require(epsilon >= 0.0 && epsilon < 1.0, "oscillatory epsilon must be in [0, 1)");
    require(alpha >= 0.0, "oscillatory alpha must be nonnegative");
}

double OscillatoryValueTransform::raw_apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return u * (1.0 + epsilon_ * std::sin(alpha_ * u));
}

ValueTransformClass OscillatoryValueTransform::transform_class() const {
    return ValueTransformClass::Oscillatory;
}

ValueTransformSpec OscillatoryValueTransform::spec() const {
    ValueTransformSpec spec;
    spec.kind = "oscillatory";
    spec.parameters = {epsilon_, alpha_};
    return spec;
}

CosineZeroValueTransform::CosineZeroValueTransform(double alpha)
    : alpha_(alpha) {
    require(alpha >= 0.0, "cosine-zero alpha must be nonnegative");
}

double CosineZeroValueTransform::raw_apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return 1.0 - std::cos(alpha_ * u);
}

ValueTransformClass CosineZeroValueTransform::transform_class() const {
    return ValueTransformClass::CosineZero;
}

ValueTransformSpec CosineZeroValueTransform::spec() const {
    ValueTransformSpec spec;
    spec.kind = "cosine_zero";
    spec.parameters = {alpha_};
    return spec;
}

} // namespace FuncCraft
