#include "value_transforms.h"
#include "support.h"

#include <cmath>

namespace FuncCraft {
using namespace detail;

double IdentityValueTransform::apply(double u) const {
    return u;
}

ValueTransformClass IdentityValueTransform::transform_class() const {
    return ValueTransformClass::None;
}

PowerValueTransform::PowerValueTransform(double alpha, double p)
    : alpha_(alpha),
      p_(p) {
    require(alpha >= 0.0, "power transform alpha must be nonnegative");
    require(p > 0.0, "power transform exponent must be positive");
}

double PowerValueTransform::apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return alpha_ * std::pow(u, p_);
}

ValueTransformClass PowerValueTransform::transform_class() const {
    return ValueTransformClass::Power;
}

OscillatoryValueTransform::OscillatoryValueTransform(double epsilon, double alpha)
    : epsilon_(epsilon),
      alpha_(alpha) {
    require(epsilon >= 0.0 && epsilon < 1.0, "oscillatory epsilon must be in [0, 1)");
    require(alpha >= 0.0, "oscillatory alpha must be nonnegative");
}

double OscillatoryValueTransform::apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return u * (1.0 + epsilon_ * std::sin(alpha_ * u));
}

ValueTransformClass OscillatoryValueTransform::transform_class() const {
    return ValueTransformClass::Oscillatory;
}

CosineZeroValueTransform::CosineZeroValueTransform(double alpha)
    : alpha_(alpha) {
    require(alpha >= 0.0, "cosine-zero alpha must be nonnegative");
}

double CosineZeroValueTransform::apply(double u) const {
    require(u >= 0.0, "value transform input must be nonnegative");
    return 1.0 - std::cos(alpha_ * u);
}

ValueTransformClass CosineZeroValueTransform::transform_class() const {
    return ValueTransformClass::CosineZero;
}

} // namespace FuncCraft
