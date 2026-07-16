#ifndef FUNCCRAFT_COORDINATE_TRANSFORMS_H
#define FUNCCRAFT_COORDINATE_TRANSFORMS_H

#include "core.h"

#include <vector>

namespace FuncCraft {

class CoordinateTransform {
public:
    virtual ~CoordinateTransform() = default;
    virtual std::vector<double> apply(const std::vector<double>& x) const = 0;
    virtual int input_dimension() const = 0;
    virtual int output_dimension() const = 0;
    virtual CoordinateTransformClass transform_class() const = 0;
};

class IdentityTransform final : public CoordinateTransform {
public:
    explicit IdentityTransform(int dimension);
    std::vector<double> apply(const std::vector<double>& x) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;

private:
    int dimension_ = 0;
};

class SubspaceTransform final : public CoordinateTransform {
public:
    SubspaceTransform(int input_dimension, std::vector<int> indices);
    SubspaceTransform(int input_dimension, std::vector<int> indices, std::vector<double> center);
    std::vector<double> apply(const std::vector<double>& x) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;
    const std::vector<int>& indices() const;

private:
    int input_dimension_ = 0;
    std::vector<int> indices_;
    std::vector<double> center_;
};

class RotationTransform final : public CoordinateTransform {
public:
    explicit RotationTransform(std::vector<std::vector<double>> matrix);
    RotationTransform(std::vector<std::vector<double>> matrix, std::vector<double> center);
    std::vector<double> apply(const std::vector<double>& x) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;

private:
    std::vector<std::vector<double>> matrix_;
    std::vector<double> center_;
};

class AffineTransform final : public CoordinateTransform {
public:
    AffineTransform(int input_dimension, std::vector<std::vector<double>> matrix, std::vector<double> offset);
    AffineTransform(std::vector<std::vector<double>> matrix, std::vector<double> center, std::vector<double> target);
    static AffineTransform map_point_to_point(
        std::vector<std::vector<double>> matrix,
        const std::vector<double>& source_point,
        const std::vector<double>& target_point);

    std::vector<double> apply(const std::vector<double>& x) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;

private:
    int input_dimension_ = 0;
    std::vector<std::vector<double>> matrix_;
    std::vector<double> offset_;
    std::vector<double> center_;
};

class FoldTransform final : public CoordinateTransform {
public:
    FoldTransform(int dimension, int folded_coordinate = 0);
    std::vector<double> apply(const std::vector<double>& x) const override;
    int input_dimension() const override;
    int output_dimension() const override;
    CoordinateTransformClass transform_class() const override;

private:
    int dimension_ = 0;
    int folded_coordinate_ = 0;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_COORDINATE_TRANSFORMS_H
