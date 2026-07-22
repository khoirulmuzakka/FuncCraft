#ifndef FUNCCRAFT_COORDINATE_TRANSFORMS_H
#define FUNCCRAFT_COORDINATE_TRANSFORMS_H

/**
 * @file coordinate_transforms.h
 * @brief Coordinate transforms used to remap benchmark input vectors.
 */

#include "function_spec.h"
#include "core.h"

#include <cstdint>
#include <vector>

namespace FuncCraft {

class CoordinateTransform {
public:
    virtual ~CoordinateTransform() = default;

    virtual void apply(const std::vector<double>& x, std::vector<double>& out) const = 0;
    virtual int input_dimension() const = 0;
    virtual int output_dimension() const = 0;
    virtual CoordinateTransformClass transform_class() const = 0;
    virtual TransformSpec spec() const = 0;

    int dimension() const;
    std::uint64_t seed() const;
    const std::vector<double>& source_point() const;
    const std::vector<double>& target_point() const;

protected:
    CoordinateTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed);

    int dimension_ = 0;
    std::vector<double> source_point_;
    std::vector<double> target_point_;
    std::uint64_t seed_ = 0;
};

/**
 * @brief Identity transform with the standard source/target/seed signature.
 */
class IdentityTransform final : public CoordinateTransform {
public:
    IdentityTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed);

    void apply(const std::vector<double>& x, std::vector<double>& out) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;
    TransformSpec spec() const override;
};

/**
 * @brief Applies a seed-generated rotation matrix around a source point.
 *
 * The input, source point, and target point all have the full transform
 * dimension. This transform uses
 * `T(x) = target_point + matrix * (x - source_point)`, so `source_point` is
 * the desired minimizer in generated/search coordinates and `target_point` is
 * the primitive-coordinate minimizer, which must be the base function's
 * `x_opt` by construction.
 */
class RotationTransform final : public CoordinateTransform {
public:
    RotationTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed);
    RotationTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed,
        std::vector<std::vector<double>> matrix);

    void apply(const std::vector<double>& x, std::vector<double>& out) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;
    TransformSpec spec() const override;

private:
    std::vector<std::vector<double>> matrix_;
};

/**
 * @brief Applies a seed-generated affine matrix around a source point.
 *
 * The input, source point, and target point all have the full transform
 * dimension. This transform uses
 * `T(x) = target_point + matrix * (x - source_point)`, so `source_point` is
 * the desired minimizer in generated/search coordinates and `target_point` is
 * the primitive-coordinate minimizer, which must be the base function's
 * `x_opt` by construction.
 */
class AffineTransform final : public CoordinateTransform {
public:
    AffineTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed);
    AffineTransform(
        int dimension,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed,
        std::vector<std::vector<double>> matrix);

    void apply(const std::vector<double>& x, std::vector<double>& out) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;
    TransformSpec spec() const override;

private:
    std::vector<std::vector<double>> matrix_;
};

/**
 * @brief Selects coordinates from the full vector and rotates only that block.
 *
 * `selected_indices` uses 0-based indices into the full input vector. The full
 * vector dimension is preserved; only the selected coordinates are replaced by
 * the rotated block. Unselected coordinates pass through unchanged.
 */
class BlockRotationTransform final : public CoordinateTransform {
public:
    BlockRotationTransform(
        int dimension,
        std::vector<int> selected_indices,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed);
    BlockRotationTransform(
        int dimension,
        std::vector<int> selected_indices,
        std::vector<double> source_point,
        std::vector<double> target_point,
        std::uint64_t seed,
        std::vector<std::vector<double>> matrix);

    void apply(const std::vector<double>& x, std::vector<double>& out) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;
    TransformSpec spec() const override;

    const std::vector<int>& selected_indices() const;

private:
    std::vector<int> selected_indices_;
    std::vector<std::vector<double>> matrix_;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_COORDINATE_TRANSFORMS_H
