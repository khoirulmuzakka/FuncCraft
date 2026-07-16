#ifndef FUNCCRAFT_COMPOSITION_H
#define FUNCCRAFT_COMPOSITION_H

/**
 * @file composition.h
 * @brief Composition strategies and composed benchmark function wrappers.
 *
 * This header defines how primitive functions are combined into higher-level
 * benchmark functions through composition, coordinate transforms, and value
 * transforms.
 */

#include "core.h"
#include "coordinate_transforms.h"
#include "value_transforms.h"

#include <memory>
#include <vector>

namespace FuncCraft {

class CompositionFunction {
public:
    virtual ~CompositionFunction() = default;
    /**
     * @brief Combine transformed component values into one scalar output.
     */
    virtual double apply(const std::vector<double>& x, const std::vector<double>& z) const = 0;
    /**
     * @brief Return the composition family used by this strategy.
     */
    virtual CompositionClass composition_class() const = 0;
};

/**
 * @brief Trivial composition that forwards the single component value.
 */
class SingleComponentComposition final : public CompositionFunction {
public:
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;
};

/**
 * @brief Weighted sum composition across multiple components.
 */
class WeightedSumComposition final : public CompositionFunction {
public:
    explicit WeightedSumComposition(std::vector<double> weights);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<double> weights_;
};

/**
 * @brief Power-mean composition across multiple components.
 */
class PowerMeanComposition final : public CompositionFunction {
public:
    PowerMeanComposition(std::vector<double> weights, double p);
    double apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionClass composition_class() const override;

private:
    std::vector<double> weights_;
    double p_ = 1.0;
};

/**
 * @brief Level-well composition with sinusoidal shaping.
 */
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

/**
 * @brief Softmax-based deceptive composition with optional local selection.
 */
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

/**
 * @brief One primitive component inside a composed benchmark function.
 */
struct Component {
    std::shared_ptr<BasicF> base;
    std::shared_ptr<CoordinateTransform> coordinate_transform;
    std::shared_ptr<ValueTransform> value_transform;
};

/**
 * @brief Concrete benchmark function assembled from components and a composition rule.
 */
class ComposedFunction final {
public:
    ComposedFunction(
        Domain domain,
        std::vector<Component> components,
        std::shared_ptr<CompositionFunction> composition,
        FunctionMetadata metadata);

    std::vector<double> operator()(const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the metadata describing this composed function.
     */
    const FunctionMetadata& metadata() const;
    /**
     * @brief Return the input domain of this composed function.
     */
    const Domain& domain() const;
    /**
     * @brief Return the problem dimension.
     */
    int dimension() const;
    /**
     * @brief Evaluate all transformed component values for a single input point.
     */
    std::vector<double> component_values(const std::vector<double>& x) const;

private:
    double evaluate_single(const std::vector<double>& x) const;

    Domain domain_;
    std::vector<Component> components_;
    std::shared_ptr<CompositionFunction> composition_;
    FunctionMetadata metadata_;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_COMPOSITION_H
