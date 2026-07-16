#include "coordinate_transforms.h"
#include "support.h"

#include <cmath>
#include <utility>

namespace FuncCraft {
using namespace detail;

IdentityTransform::IdentityTransform(int dimension)
    : dimension_(dimension) {
    require(dimension > 0, "identity transform dimension must be positive");
}

std::vector<double> IdentityTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, dimension_, "identity transform input");
    return x;
}

int IdentityTransform::input_dimension() const {
    return dimension_;
}

int IdentityTransform::output_dimension() const {
    return dimension_;
}

CoordinateTransformClass IdentityTransform::transform_class() const {
    return CoordinateTransformClass::None;
}

SubspaceTransform::SubspaceTransform(int input_dimension, std::vector<int> indices)
    : input_dimension_(input_dimension),
      indices_(std::move(indices)),
      center_(indices_.size(), 0.0) {
    require(input_dimension > 0, "subspace input dimension must be positive");
    require(!indices_.empty(), "subspace transform needs at least one index");
    for (int idx : indices_) {
        require(idx >= 0 && idx < input_dimension_, "subspace index out of range");
    }
}

SubspaceTransform::SubspaceTransform(int input_dimension, std::vector<int> indices, std::vector<double> center)
    : input_dimension_(input_dimension),
      indices_(std::move(indices)),
      center_(std::move(center)) {
    require(input_dimension > 0, "subspace input dimension must be positive");
    require(!indices_.empty(), "subspace transform needs at least one index");
    require(center_.size() == indices_.size(), "subspace center dimension mismatch");
    for (int idx : indices_) {
        require(idx >= 0 && idx < input_dimension_, "subspace index out of range");
    }
}

std::vector<double> SubspaceTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension_, "subspace transform input");
    std::vector<double> y;
    y.reserve(indices_.size());
    for (std::size_t i = 0; i < indices_.size(); ++i) {
        y.push_back(x[static_cast<std::size_t>(indices_[i])] - center_[i]);
    }
    return y;
}

int SubspaceTransform::input_dimension() const {
    return input_dimension_;
}

int SubspaceTransform::output_dimension() const {
    return static_cast<int>(indices_.size());
}

CoordinateTransformClass SubspaceTransform::transform_class() const {
    return CoordinateTransformClass::Block;
}

const std::vector<int>& SubspaceTransform::indices() const {
    return indices_;
}

RotationTransform::RotationTransform(std::vector<std::vector<double>> matrix)
    : matrix_(std::move(matrix)),
      center_(matrix_.size(), 0.0) {
    require(!matrix_.empty(), "rotation matrix must have at least one row");
    const std::size_t dimension = matrix_.size();
    for (const auto& row : matrix_) {
        require(row.size() == dimension, "rotation matrix must be square");
    }
}

RotationTransform::RotationTransform(std::vector<std::vector<double>> matrix, std::vector<double> center)
    : matrix_(std::move(matrix)),
      center_(std::move(center)) {
    require(!matrix_.empty(), "rotation matrix must have at least one row");
    const std::size_t dimension = matrix_.size();
    require(center_.size() == dimension, "rotation center dimension mismatch");
    for (const auto& row : matrix_) {
        require(row.size() == dimension, "rotation matrix must be square");
    }
}

std::vector<double> RotationTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension(), "rotation transform input");
    std::vector<double> y(matrix_.size(), 0.0);
    for (std::size_t r = 0; r < matrix_.size(); ++r) {
        for (std::size_t c = 0; c < x.size(); ++c) {
            y[r] += matrix_[r][c] * (x[c] - center_[c]);
        }
    }
    return y;
}

int RotationTransform::input_dimension() const {
    return static_cast<int>(matrix_.size());
}

int RotationTransform::output_dimension() const {
    return static_cast<int>(matrix_.size());
}

CoordinateTransformClass RotationTransform::transform_class() const {
    return CoordinateTransformClass::Rotation;
}

AffineTransform::AffineTransform(int input_dimension, std::vector<std::vector<double>> matrix, std::vector<double> offset)
    : input_dimension_(input_dimension),
      matrix_(std::move(matrix)),
      offset_(std::move(offset)),
      center_(static_cast<std::size_t>(input_dimension), 0.0) {
    require(input_dimension > 0, "affine input dimension must be positive");
    require(!matrix_.empty(), "affine matrix must have at least one row");
    require(matrix_.size() == offset_.size(), "affine offset size must match matrix rows");
    for (const auto& row : matrix_) {
        require(static_cast<int>(row.size()) == input_dimension_, "affine matrix row dimension mismatch");
    }
}

AffineTransform::AffineTransform(std::vector<std::vector<double>> matrix, std::vector<double> center, std::vector<double> target)
    : input_dimension_(static_cast<int>(center.size())),
      matrix_(std::move(matrix)),
      offset_(std::move(target)),
      center_(std::move(center)) {
    require(input_dimension_ > 0, "affine input dimension must be positive");
    require(!matrix_.empty(), "affine matrix must have at least one row");
    require(matrix_.size() == offset_.size(), "affine target size must match matrix rows");
    for (const auto& row : matrix_) {
        require(static_cast<int>(row.size()) == input_dimension_, "affine matrix row dimension mismatch");
    }
}

AffineTransform AffineTransform::map_point_to_point(
    std::vector<std::vector<double>> matrix,
    const std::vector<double>& source_point,
    const std::vector<double>& target_point) {
    require(!matrix.empty(), "affine matrix must have at least one row");
    require(matrix.size() == target_point.size(), "target point dimension must match matrix rows");
    const int input_dimension = static_cast<int>(source_point.size());
    std::vector<double> offset(target_point.size(), 0.0);
    for (std::size_t r = 0; r < matrix.size(); ++r) {
        require(static_cast<int>(matrix[r].size()) == input_dimension, "affine matrix row dimension mismatch");
        double mapped = 0.0;
        for (std::size_t c = 0; c < source_point.size(); ++c) {
            mapped += matrix[r][c] * source_point[c];
        }
        offset[r] = target_point[r] - mapped;
    }
    return AffineTransform(input_dimension, std::move(matrix), std::move(offset));
}

std::vector<double> AffineTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension_, "affine transform input");
    std::vector<double> y(matrix_.size(), 0.0);
    for (std::size_t r = 0; r < matrix_.size(); ++r) {
        y[r] = offset_[r];
        for (std::size_t c = 0; c < x.size(); ++c) {
            y[r] += matrix_[r][c] * (x[c] - center_[c]);
        }
    }
    return y;
}

int AffineTransform::input_dimension() const {
    return input_dimension_;
}

int AffineTransform::output_dimension() const {
    return static_cast<int>(matrix_.size());
}

CoordinateTransformClass AffineTransform::transform_class() const {
    return CoordinateTransformClass::Affine;
}

FoldTransform::FoldTransform(int dimension, int folded_coordinate)
    : dimension_(dimension),
      folded_coordinate_(folded_coordinate) {
    require(dimension > 0, "fold transform dimension must be positive");
    require(folded_coordinate >= 0 && folded_coordinate < dimension, "folded coordinate out of range");
}

std::vector<double> FoldTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, dimension_, "fold transform input");
    std::vector<double> y = x;
    const auto idx = static_cast<std::size_t>(folded_coordinate_);
    y[idx] = std::fabs(y[idx]);
    return y;
}

int FoldTransform::input_dimension() const {
    return dimension_;
}

int FoldTransform::output_dimension() const {
    return dimension_;
}

CoordinateTransformClass FoldTransform::transform_class() const {
    return CoordinateTransformClass::NonlinearFold;
}

} // namespace FuncCraft
