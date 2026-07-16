#ifndef FUNCCRAFT_COMPOSITION_H
#define FUNCCRAFT_COMPOSITION_H

#include "core.h"
#include "coordinate_transforms.h"
#include "value_transforms.h"

#include <memory>
#include <vector>

namespace FuncCraft {

class CompositionFunction {
public:
    virtual ~CompositionFunction() = default;
    virtual double apply(const std::vector<double>& x, const std::vector<double>& z) const = 0;
    virtual CompositionClass composition_class() const = 0;
};

class SingleComponentComposition final : public CompositionFunction {
public:
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;
};

class WeightedSumComposition final : public CompositionFunction {
public:
    explicit WeightedSumComposition(std::vector<double> weights);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<double> weights_;
};

class PowerMeanComposition final : public CompositionFunction {
public:
    PowerMeanComposition(std::vector<double> weights, double p);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<double> weights_;
    double p_ = 1.0;
};

class LevelWellComposition final : public CompositionFunction {
public:
    LevelWellComposition(std::vector<double> weights, double epsilon, double alpha);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<double> weights_;
    double epsilon_ = 0.1;
    double alpha_ = 1.0;
};

class DeceptiveSoftmaxComposition final : public CompositionFunction {
public:
    DeceptiveSoftmaxComposition(std::vector<std::vector<double>> centers, std::vector<double> offsets, double sharpness);
    DeceptiveSoftmaxComposition(
        std::vector<std::vector<double>> centers,
        std::vector<double> offsets,
        double sharpness,
        double local_selection_radius);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<std::vector<double>> centers_;
    std::vector<double> offsets_;
    double sharpness_ = 1.0;
    double local_selection_radius_ = 0.0;
};

struct Component {
    std::shared_ptr<BaseFunction> base;
    std::shared_ptr<CoordinateTransform> coordinate_transform;
    std::shared_ptr<ValueTransform> value_transform;
};

class ComposedFunction final {
public:
    ComposedFunction(
        Domain domain,
        std::vector<Component> components,
        std::shared_ptr<CompositionFunction> composition,
        FunctionMetadata metadata);

    double evaluate(const std::vector<double>& x) const;
    std::vector<double> component_values(const std::vector<double>& x) const;
    const FunctionMetadata& metadata() const;
    const Domain& domain() const;
    int dimension() const;

private:
    Domain domain_;
    std::vector<Component> components_;
    std::shared_ptr<CompositionFunction> composition_;
    FunctionMetadata metadata_;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_COMPOSITION_H
