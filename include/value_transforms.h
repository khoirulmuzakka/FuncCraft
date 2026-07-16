#ifndef FUNCCRAFT_VALUE_TRANSFORMS_H
#define FUNCCRAFT_VALUE_TRANSFORMS_H

#include "core.h"

namespace FuncCraft {

class ValueTransform {
public:
    virtual ~ValueTransform() = default;
    virtual double apply(double u) const = 0;
    virtual ValueTransformClass transform_class() const = 0;
};

class IdentityValueTransform final : public ValueTransform {
public:
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;
};

class PowerValueTransform final : public ValueTransform {
public:
    PowerValueTransform(double alpha, double p);
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;

private:
    double alpha_ = 1.0;
    double p_ = 1.0;
};

class OscillatoryValueTransform final : public ValueTransform {
public:
    OscillatoryValueTransform(double epsilon, double alpha);
    double apply(double u) const override;
    ValueTransformClass transform_class() const override;

private:
    double epsilon_ = 0.1;
    double alpha_ = 1.0;
};

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
