#ifndef FUNCCRAFT_CORE_H
#define FUNCCRAFT_CORE_H

/**
 * @file core.h
 * @brief Core benchmark types and shared enumerations.
 *
 * This header defines the basic domain container, the classification enums
 * used to describe generated benchmark functions, and the compact class label
 * helper used by the builder.
 */

#include "basicf.h"

#include <string>
#include <vector>

namespace FuncCraft {

enum class CompositionClass {
    /// No composition, a single transformed component.
    None,
    /// Weighted sum of components evaluated at a common point.
    CommonPointWeightedSum,
    /// Power mean aggregation of components evaluated at a common point.
    CommonPointPowerMean,
    /// Level-well aggregation of components evaluated at a common point.
    CommonPointLevelWell,
    /// Deceptive softmax selection over multiple component centers.
    DeceptivePointSoftmax,
    /// Deceptive softmax with a smooth background weight.
    DeceptivePointBgSoftmax,
};

enum class ValueTransformClass {
    /// No value transform.
    None,
    /// Cosine-based zeroing transform.
    CosineZero,
    /// Oscillatory transform.
    Oscillatory,
    /// Power-law transform.
    Power,
    /// Mixed value transforms across components.
    Mixed,
};

enum class CoordinateTransformClass {
    /// No coordinate transform.
    None,
    /// Rotation transform.
    Rotation,
    /// Affine transform.
    Affine,
    /// Block rotation transform on a selected subspace.
    BlockRotation,
    /// Mixed coordinate transforms across components.
    Mixed,
};

/**
 * @brief Axis-aligned rectangular domain for benchmark inputs.
 */
struct Domain {
    std::vector<double> lower;
    std::vector<double> upper;

    explicit Domain(int dimension = 0, double lower_bound = -100.0, double upper_bound = 100.0);
    int dimension() const;
    bool contains(const std::vector<double>& x) const;
};

/**
 * @brief Classification summary for a composed benchmark function.
 */
struct FunctionClass {
    CompositionClass composition = CompositionClass::None;
    ValueTransformClass value_transform = ValueTransformClass::None;
    CoordinateTransformClass coordinate_transform = CoordinateTransformClass::None;
    std::vector<BasicFunctionId> base_functions;
};

/**
 * @brief Render a composition class as a short label.
 */
std::string to_string(CompositionClass cls);
/**
 * @brief Render a value transform class as a short label.
 */
std::string to_string(ValueTransformClass cls);
/**
 * @brief Render a coordinate transform class as a short label.
 */
std::string to_string(CoordinateTransformClass cls);
/**
 * @brief Build a compact label describing a complete function class.
 */
std::string class_label(const FunctionClass& cls);

} // namespace FuncCraft

#endif // FUNCCRAFT_CORE_H
