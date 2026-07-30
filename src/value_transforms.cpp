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

double positive_power(double value, double exponent) {
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
    require(std::isfinite(alpha), "power transform alpha must be finite");
    require(std::isfinite(p), "power transform exponent must be finite");
    require(alpha > 0.0, "power transform alpha must be positive");
    require(p > 0.0, "power transform exponent must be positive");
}

double PowerValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = alpha_ * positive_power(u, p_);
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
    require(std::isfinite(epsilon), "oscillatory epsilon must be finite");
    require(std::isfinite(alpha), "oscillatory alpha must be finite");
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

double OscillatoryValueTransform::epsilon() const {
    return epsilon_;
}

double OscillatoryValueTransform::alpha() const {
    return alpha_;
}

CosineZeroValueTransform::CosineZeroValueTransform(double alpha)
    : alpha_(alpha) {
    require(std::isfinite(alpha), "cosine-zero alpha must be finite");
    require(alpha > 0.0, "cosine-zero alpha must be positive");
}

double CosineZeroValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(1.0 - std::cos(alpha_ * u));
}

ValueTransformClass CosineZeroValueTransform::transform_class() const {
    return ValueTransformClass::CosineZero;
}

double CosineZeroValueTransform::alpha() const {
    return alpha_;
}

HuberValueTransform::HuberValueTransform(double delta)
    : delta_(delta) {
    require(std::isfinite(delta), "huber delta must be finite");
    require(delta > 0.0, "huber delta must be positive");
}

double HuberValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double value = u <= delta_
        ? 0.5 * u * u / delta_
        : u - 0.5 * delta_;
    return clamp_finite_nonnegative(value);
}

ValueTransformClass HuberValueTransform::transform_class() const {
    return ValueTransformClass::Huber;
}

double HuberValueTransform::delta() const {
    return delta_;
}

LogValueTransform::LogValueTransform(double alpha)
    : alpha_(alpha) {
    require(std::isfinite(alpha), "log transform alpha must be finite");
    require(alpha > 0.0, "log transform alpha must be positive");
}

double LogValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(std::log1p(alpha_ * u) / alpha_);
}

ValueTransformClass LogValueTransform::transform_class() const {
    return ValueTransformClass::Log;
}

double LogValueTransform::alpha() const {
    return alpha_;
}

SoftplusThresholdValueTransform::SoftplusThresholdValueTransform(double tau, double alpha)
    : tau_(tau),
      alpha_(alpha) {
    require(std::isfinite(tau), "softplus-threshold tau must be finite");
    require(std::isfinite(alpha), "softplus-threshold alpha must be finite");
    require(tau >= 0.0, "softplus-threshold tau must be nonnegative");
    require(alpha > 0.0, "softplus-threshold alpha must be positive");
}

double SoftplusThresholdValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const auto softplus = [](double x) {
        if (x > 40.0) {
            return x;
        }
        if (x < -40.0) {
            return std::exp(x);
        }
        return std::log1p(std::exp(x));
    };
    const double value = (softplus(alpha_ * (u - tau_)) - softplus(-alpha_ * tau_)) / alpha_;
    return clamp_finite_nonnegative(value);
}

ValueTransformClass SoftplusThresholdValueTransform::transform_class() const {
    return ValueTransformClass::SoftplusThreshold;
}

double SoftplusThresholdValueTransform::tau() const {
    return tau_;
}

double SoftplusThresholdValueTransform::alpha() const {
    return alpha_;
}

DeadZoneValueTransform::DeadZoneValueTransform(double tau, double p)
    : tau_(tau),
      p_(p) {
    require(std::isfinite(tau), "dead-zone tau must be finite");
    require(std::isfinite(p), "dead-zone exponent must be finite");
    require(tau >= 0.0, "dead-zone tau must be nonnegative");
    require(p > 0.0, "dead-zone exponent must be positive");
}

double DeadZoneValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(positive_power(std::max(0.0, u - tau_), p_));
}

ValueTransformClass DeadZoneValueTransform::transform_class() const {
    return ValueTransformClass::DeadZone;
}

double DeadZoneValueTransform::tau() const {
    return tau_;
}

double DeadZoneValueTransform::p() const {
    return p_;
}

SaturatingValueTransform::SaturatingValueTransform(double cap, double c)
    : cap_(cap),
      c_(c) {
    require(std::isfinite(cap), "saturating cap must be finite");
    require(std::isfinite(c), "saturating c must be finite");
    require(cap > 0.0, "saturating cap must be positive");
    require(c > 0.0, "saturating c must be positive");
}

double SaturatingValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    return clamp_finite_nonnegative(cap_ * u / (u + c_));
}

ValueTransformClass SaturatingValueTransform::transform_class() const {
    return ValueTransformClass::Saturating;
}

double SaturatingValueTransform::cap() const {
    return cap_;
}

double SaturatingValueTransform::c() const {
    return c_;
}

PiecewisePowerValueTransform::PiecewisePowerValueTransform(double tau, double p1, double p2)
    : tau_(tau),
      p1_(p1),
      p2_(p2) {
    require(std::isfinite(tau), "piecewise-power tau must be finite");
    require(std::isfinite(p1), "piecewise-power p1 must be finite");
    require(std::isfinite(p2), "piecewise-power p2 must be finite");
    require(tau > 0.0, "piecewise-power tau must be positive");
    require(p1 > 0.0, "piecewise-power p1 must be positive");
    require(p2 > 0.0, "piecewise-power p2 must be positive");
}

double PiecewisePowerValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    if (u <= tau_) {
        return clamp_finite_nonnegative(positive_power(u, p1_));
    }
    return clamp_finite_nonnegative(positive_power(tau_, p1_) + positive_power(u - tau_, p2_));
}

ValueTransformClass PiecewisePowerValueTransform::transform_class() const {
    return ValueTransformClass::PiecewisePower;
}

double PiecewisePowerValueTransform::tau() const {
    return tau_;
}

double PiecewisePowerValueTransform::p1() const {
    return p1_;
}

double PiecewisePowerValueTransform::p2() const {
    return p2_;
}

NoisySmoothValueTransform::NoisySmoothValueTransform(double epsilon, double alpha)
    : epsilon_(epsilon),
      alpha_(alpha) {
    require(std::isfinite(epsilon), "noisy-smooth epsilon must be finite");
    require(std::isfinite(alpha), "noisy-smooth alpha must be finite");
    require(epsilon >= 0.0 && epsilon < 1.0, "noisy-smooth epsilon must be in [0, 1)");
    require(alpha >= 0.0, "noisy-smooth alpha must be nonnegative");
}

double NoisySmoothValueTransform::raw_apply(double u) const {
    require(u >= -kValueTransformTolerance, "value transform input must be nonnegative");
    u = std::max(0.0, u);
    const double roughness = std::sin(alpha_ * u) * std::sin(0.371 * alpha_ * u + 1.2345);
    return clamp_finite_nonnegative(u * (1.0 + epsilon_ * roughness));
}

ValueTransformClass NoisySmoothValueTransform::transform_class() const {
    return ValueTransformClass::NoisySmooth;
}

double NoisySmoothValueTransform::epsilon() const {
    return epsilon_;
}

double NoisySmoothValueTransform::alpha() const {
    return alpha_;
}

} // namespace FuncCraft
