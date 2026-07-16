#ifndef FUNCCRAFT_VALUE_TRANSFORMS_H
#define FUNCCRAFT_VALUE_TRANSFORMS_H

/**
 * @file value_transforms.h
 * @brief Scalar transforms applied to primitive function values.
 *
 * These transforms reshape the output of a base function before composition.
 */

#include "core.h"

namespace FuncCraft {

class ValueTransform {
public:
    virtual ~ValueTransform() = default;
    /**
     * @brief Transform one scalar function value.
     */
    virtual double apply(double u) const = 0;
    /**
     * @brief Return the transform family used by this object.
     */
    virtual ValueTransformClass transform_class() const = 0;
};

/**
 * @brief Identity value transform that returns the input unchanged.
 */
class IdentityValueTransform final : public ValueTransform {
public:
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;
};

/**
 * @brief Power-law scalar transform.
 */
class PowerValueTransform final : public ValueTransform {
public:
    PowerValueTransform(double alpha, double p);
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;

private:
    double alpha_ = 1.0;
    double p_ = 1.0;
};

/**
 * @brief Oscillatory scalar transform used to create rugged landscapes.
 */
class OscillatoryValueTransform final : public ValueTransform {
public:
    OscillatoryValueTransform(double epsilon, double alpha);
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;

private:
    double epsilon_ = 0.1;
    double alpha_ = 1.0;
};

/**
 * @brief Cosine-based transform that forces the value near zero at the origin.
 */
class CosineZeroValueTransform final : public ValueTransform {
public:
    explicit CosineZeroValueTransform(double alpha);
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;

private:
    double alpha_ = 1.0;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_VALUE_TRANSFORMS_H
