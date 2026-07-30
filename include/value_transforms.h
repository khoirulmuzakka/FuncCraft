#ifndef FUNCCRAFT_VALUE_TRANSFORMS_H
#define FUNCCRAFT_VALUE_TRANSFORMS_H

/**
 * @file value_transforms.h
 * @brief Scalar transforms applied to primitive function values.
 *
 * These transforms reshape the output of a base function before composition.
 * Every value transform must preserve the origin: `f(0) == 0`.
 * The runtime checks are tolerance-based so tiny floating-point roundoff near
 * zero does not fail a valid benchmark evaluation.
 */

#include "core.h"

namespace FuncCraft {

class ValueTransform {
public:
    virtual ~ValueTransform() = default;
    /**
 * @brief Transform one scalar function value.
 *
 * The base class rejects materially negative inputs and materially negative
 * transformed outputs. Concrete transforms are parameterized so that
 * `raw_apply(0.0) == 0.0`.
 */
    double apply(double u) const;
    /**
     * @brief Return the transform family used by this object.
     */
    virtual ValueTransformClass transform_class() const = 0;

protected:
    virtual double raw_apply(double u) const = 0;
};

/**
 * @brief Identity value transform that returns the input unchanged.
 *
 * This is valid because `f(0) == 0`.
 */
class IdentityValueTransform final : public ValueTransform {
public:
    ValueTransformClass transform_class() const override;

protected:
    double raw_apply(double u) const override;
};

/**
 * @brief Power-law scalar transform.
 *
 * This transform is origin-preserving for the supported parameter ranges.
 */
class PowerValueTransform final : public ValueTransform {
public:
    PowerValueTransform(double alpha = 1.0, double p = 1.0);
    ValueTransformClass transform_class() const override;
    double alpha() const;
    double p() const;

protected:
    double raw_apply(double u) const override;

private:
    double alpha_ = 1.0;
    double p_ = 1.0;
};

/**
 * @brief Oscillatory scalar transform used to create rugged landscapes.
 *
 * This transform is origin-preserving for the supported parameter ranges.
 */
class OscillatoryValueTransform final : public ValueTransform {
public:
    OscillatoryValueTransform(double epsilon = 0.1, double alpha = 1.0);
    ValueTransformClass transform_class() const override;
    double epsilon() const;
    double alpha() const;

protected:
    double raw_apply(double u) const override;

private:
    double epsilon_ = 0.1;
    double alpha_ = 1.0;
};

/**
 * @brief Cosine-based transform that forces the value near zero at the origin.
 *
 * This transform is origin-preserving for the supported parameter ranges.
 */
class CosineZeroValueTransform final : public ValueTransform {
public:
    explicit CosineZeroValueTransform(double alpha = 1.0);
    ValueTransformClass transform_class() const override;
    double alpha() const;

protected:
    double raw_apply(double u) const override;

private:
    double alpha_ = 1.0;
};

class HuberValueTransform final : public ValueTransform {
public:
    explicit HuberValueTransform(double delta = 1.0);
    ValueTransformClass transform_class() const override;
    double delta() const;

protected:
    double raw_apply(double u) const override;

private:
    double delta_ = 1.0;
};

class LogValueTransform final : public ValueTransform {
public:
    explicit LogValueTransform(double alpha = 1.0);
    ValueTransformClass transform_class() const override;
    double alpha() const;

protected:
    double raw_apply(double u) const override;

private:
    double alpha_ = 1.0;
};

class SoftplusThresholdValueTransform final : public ValueTransform {
public:
    SoftplusThresholdValueTransform(double tau = 1.0, double alpha = 1.0);
    ValueTransformClass transform_class() const override;
    double tau() const;
    double alpha() const;

protected:
    double raw_apply(double u) const override;

private:
    double tau_ = 1.0;
    double alpha_ = 1.0;
};

class DeadZoneValueTransform final : public ValueTransform {
public:
    DeadZoneValueTransform(double tau = 1.0, double p = 1.0);
    ValueTransformClass transform_class() const override;
    double tau() const;
    double p() const;

protected:
    double raw_apply(double u) const override;

private:
    double tau_ = 1.0;
    double p_ = 1.0;
};

class SaturatingValueTransform final : public ValueTransform {
public:
    SaturatingValueTransform(double cap = 1.0, double c = 1.0);
    ValueTransformClass transform_class() const override;
    double cap() const;
    double c() const;

protected:
    double raw_apply(double u) const override;

private:
    double cap_ = 1.0;
    double c_ = 1.0;
};

class PiecewisePowerValueTransform final : public ValueTransform {
public:
    PiecewisePowerValueTransform(double tau = 1.0, double p1 = 1.0, double p2 = 2.0);
    ValueTransformClass transform_class() const override;
    double tau() const;
    double p1() const;
    double p2() const;

protected:
    double raw_apply(double u) const override;

private:
    double tau_ = 1.0;
    double p1_ = 1.0;
    double p2_ = 2.0;
};

class NoisySmoothValueTransform final : public ValueTransform {
public:
    NoisySmoothValueTransform(double epsilon = 0.05, double alpha = 1.0);
    ValueTransformClass transform_class() const override;
    double epsilon() const;
    double alpha() const;

protected:
    double raw_apply(double u) const override;

private:
    double epsilon_ = 0.05;
    double alpha_ = 1.0;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_VALUE_TRANSFORMS_H
