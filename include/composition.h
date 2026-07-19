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

#include "function_spec.h"
#include "core.h"

#include <memory>
#include <vector>

namespace FuncCraft {

class CompositionFunction {
public:
    virtual ~CompositionFunction() = default;
    /**
     * @brief Combine transformed component values into one scalar output.
     */
    double apply(const std::vector<double>& x, const std::vector<double>& z) const;
    /**
     * @brief Return the composition family used by this strategy.
     */
    virtual CompositionClass composition_class() const = 0;
    virtual CompositionSpec spec() const = 0;

protected:
    virtual double raw_apply(const std::vector<double>& x, const std::vector<double>& z) const = 0;
};

/**
 * @brief Base for compositions that only depend on the component values.
 */
class CommonPointComposition : public CompositionFunction {
protected:
    double raw_apply(const std::vector<double>& x, const std::vector<double>& z) const final;
    virtual double common_raw_apply(const std::vector<double>& z) const = 0;
};

/**
 * @brief Base for compositions that may depend on both the point and values.
 */
class DeceptivePointComposition : public CompositionFunction {
protected:
    double raw_apply(const std::vector<double>& x, const std::vector<double>& z) const final;
    virtual double deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const = 0;
};

/**
 * @brief Trivial composition that forwards the single component value.
 */
class SingleComponentComposition final : public CommonPointComposition {
public:
    CompositionClass composition_class() const override;
    CompositionSpec spec() const override;

protected:
    double common_raw_apply(const std::vector<double>& z) const override;
};

/**
 * @brief Weighted sum composition across multiple components.
 */
class WeightedSumComposition final : public CommonPointComposition {
public:
    explicit WeightedSumComposition(std::vector<double> weights);
    explicit WeightedSumComposition(std::size_t components);
    CompositionClass composition_class() const override;

private:
    double common_raw_apply(const std::vector<double>& z) const override;
    CompositionSpec spec() const override;
    std::vector<double> weights_;
};

/**
 * @brief Power-mean composition across multiple components.
 */
class PowerMeanComposition final : public CommonPointComposition {
public:
    PowerMeanComposition(std::vector<double> weights, double p);
    PowerMeanComposition(std::size_t components, double p = 2.0);
    CompositionClass composition_class() const override;

private:
    double common_raw_apply(const std::vector<double>& z) const override;
    CompositionSpec spec() const override;
    std::vector<double> weights_;
    double p_ = 1.0;
};

/**
 * @brief Level-well composition with sinusoidal shaping.
 */
class LevelWellComposition final : public CommonPointComposition {
public:
    LevelWellComposition(std::vector<double> weights, double epsilon, double alpha);
    LevelWellComposition(std::size_t components, double epsilon = 0.1, double alpha = 1.0);
    CompositionClass composition_class() const override;

private:
    double common_raw_apply(const std::vector<double>& z) const override;
    CompositionSpec spec() const override;
    std::vector<double> weights_;
    double epsilon_ = 0.1;
    double alpha_ = 1.0;
};

/**
 * @brief Softmax-based deceptive composition with optional local selection.
 */
class DeceptiveSoftmaxComposition final : public DeceptivePointComposition {
public:
    DeceptiveSoftmaxComposition(std::vector<std::vector<double>> centers, std::vector<double> offsets, double sharpness);
    DeceptiveSoftmaxComposition(
        std::vector<std::vector<double>> centers,
        std::vector<double> offsets,
        double sharpness,
        double local_selection_radius);
    DeceptiveSoftmaxComposition(
        std::vector<std::vector<double>> centers,
        double sharpness = 1.0,
        double local_selection_radius = 0.0);
    CompositionClass composition_class() const override;

private:
    double deceptive_raw_apply(const std::vector<double>& x, const std::vector<double>& z) const override;
    CompositionSpec spec() const override;
    std::vector<std::vector<double>> centers_;
    std::vector<double> offsets_;
    double sharpness_ = 1.0;
    double local_selection_radius_ = 0.0;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_COMPOSITION_H
