#include "value_transforms.h"
#include "support.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace FuncCraft {
using namespace detail;

namespace {
constexpr double kValueTransformTolerance = 1.0e-12;

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
    const double at_origin = raw_apply(0.0);
    require(std::abs(at_origin) <= 1.0e-12, "value transform must satisfy f(0) = 0");
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
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = alpha_ * std::pow(u, p_);
    return clamp_finite_nonnegative(value);
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
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = u * (1.0 + epsilon_ * std::sin(alpha_ * u));
    return clamp_finite_nonnegative(value);
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
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(1.0 - std::cos(alpha_ * u));
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
