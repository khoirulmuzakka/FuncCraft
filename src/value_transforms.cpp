#include "value_transforms.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace FuncCraft {
using namespace detail;

namespace {
constexpr double kValueTransformTolerance = 1.0e-12;
constexpr double kTwoPi = 6.2831853071795864769252867665590057683943387987502;
constexpr double kInvTwoPi = 0.15915494309189533576888376337251436203445964574046;

double reduce_trig_phase(double phase) {
    const double turns = std::nearbyint(phase * kInvTwoPi);
    return phase - turns * kTwoPi;
}

double clamp_nonnegative(double value) {
    if (value < 0.0 && value >= -kValueTransformTolerance) {
        return 0.0;
    }
    return value;
}

double clamp_finite_nonnegative(double value) {
    if (!std::isfinite(value)) {
        return std::numeric_limits<double>::max();
    }
    if (value < 0.0 && value >= -kValueTransformTolerance) {
        return 0.0;
    }
    return value;
}

std::string describe_scalar(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0.0 ? "inf" : "-inf";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    return buffer;
}
} // namespace

double ValueTransform::apply(double u) const {
    u = clamp_nonnegative(u);
    require(u >= 0.0, "value transform input must be nonnegative (u=" + describe_scalar(u) + ")");
    const double value = clamp_finite_nonnegative(raw_apply(u));
    require(value >= 0.0, "value transform output must be nonnegative (f(u)=" + describe_scalar(value) + ")");
    return value;
}

double IdentityValueTransform::raw_apply(double u) const {
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

double PowerValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = alpha_ * std::pow(u, p_);
    return clamp_finite_nonnegative(value);
}

ValueTransformClass PowerValueTransform::transform_class() const {
    return ValueTransformClass::Power;
}

double PowerValueTransform::alpha() const {
    return alpha_;
}

double PowerValueTransform::p() const {
    return p_;
}

OscillatoryValueTransform::OscillatoryValueTransform(double epsilon, double alpha)
    : epsilon_(epsilon),
      alpha_(alpha) {
    require(epsilon >= 0.0 && epsilon < 1.0, "oscillatory epsilon must be in [0, 1)");
    require(alpha >= 0.0, "oscillatory alpha must be nonnegative");
}

double OscillatoryValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = u * (1.0 + epsilon_ * std::sin(reduce_trig_phase(alpha_ * u)));
    return clamp_finite_nonnegative(value);
}

ValueTransformClass OscillatoryValueTransform::transform_class() const {
    return ValueTransformClass::Oscillatory;
}

double OscillatoryValueTransform::epsilon() const {
    return epsilon_;
}

double OscillatoryValueTransform::alpha() const {
    return alpha_;
}

CosineZeroValueTransform::CosineZeroValueTransform(double alpha)
    : alpha_(alpha) {
    require(alpha >= 0.0, "cosine-zero alpha must be nonnegative");
}

double CosineZeroValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(1.0 - std::cos(reduce_trig_phase(alpha_ * u)));
}

ValueTransformClass CosineZeroValueTransform::transform_class() const {
    return ValueTransformClass::CosineZero;
}

double CosineZeroValueTransform::alpha() const {
    return alpha_;
}

} // namespace FuncCraft
