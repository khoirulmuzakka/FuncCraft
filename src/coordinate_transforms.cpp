#include "coordinate_transforms.h"
#include "support.h"

#include <cmath>
#include <random>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {

void require_matrix_shape(const std::vector<std::vector<double>>& matrix, int rows, int cols, const std::string& name) {
    require(static_cast<int>(matrix.size()) == rows, name + " row count mismatch");
    for (const auto& row : matrix) {
        require(static_cast<int>(row.size()) == cols, name + " column count mismatch");
        for (double value : row) {
            require(std::isfinite(value), name + " entries must be finite");
        }
    }
}

void require_finite_vector(const std::vector<double>& values, const std::string& name) {
    for (double value : values) {
        require(std::isfinite(value), name + " entries must be finite");
    }
}

void require_unique_selected_indices(const std::vector<int>& selected_indices, int dimension) {
    std::vector<bool> seen(static_cast<std::size_t>(dimension), false);
    for (int idx : selected_indices) {
        require(idx >= 0 && idx < dimension, "block rotation selected index out of range");
        auto pos = static_cast<std::size_t>(idx);
        require(!seen[pos], "block rotation selected indices must be unique");
        seen[pos] = true;
    }
}

} // namespace

CoordinateTransform::CoordinateTransform(
    int input_dimension,
    int output_dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : input_dimension_(input_dimension),
      output_dimension_(output_dimension),
      assigned_xopt_(std::move(assigned_xopt)),
      target_xopt_(std::move(target_xopt)),
      seed_(seed) {
    require(input_dimension_ > 0, "coordinate transform input dimension must be positive");
    require(output_dimension_ > 0, "coordinate transform output dimension must be positive");
    require(!assigned_xopt_.empty(), "coordinate transform assigned_xopt must not be empty");
    require(!target_xopt_.empty(), "coordinate transform target_xopt must not be empty");
    require_finite_vector(assigned_xopt_, "coordinate transform assigned_xopt");
    require_finite_vector(target_xopt_, "coordinate transform target_xopt");
}

std::uint64_t CoordinateTransform::seed() const {
    return seed_;
}

const std::vector<double>& CoordinateTransform::assigned_xopt() const {
    return assigned_xopt_;
}

const std::vector<double>& CoordinateTransform::target_xopt() const {
    return target_xopt_;
}

IdentityTransform::IdentityTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : CoordinateTransform(dimension, dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, output_dimension_, "identity transform assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "identity transform target_xopt");
}

void IdentityTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "identity transform input");
    out.assign(static_cast<std::size_t>(output_dimension_), 0.0);
    for (int i = 0; i < output_dimension_; ++i) {
        const auto idx = static_cast<std::size_t>(i);
        out[idx] = target_xopt_[idx] + (x[idx] - assigned_xopt_[idx]);
    }
}

int IdentityTransform::input_dimension() const {
    return input_dimension_;
}

int IdentityTransform::output_dimension() const {
    return output_dimension_;
}

CoordinateTransformClass IdentityTransform::transform_class() const {
    return CoordinateTransformClass::None;
}

RotationTransform::RotationTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : CoordinateTransform(dimension, dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, output_dimension_, "rotation transform assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "rotation transform target_xopt");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, output_dimension_);
}

RotationTransform::RotationTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(dimension, dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      matrix_(std::move(matrix)) {
    require_dimension(assigned_xopt_, output_dimension_, "rotation transform assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "rotation transform target_xopt");
    require_matrix_shape(matrix_, output_dimension_, input_dimension_, "rotation transform matrix");
}

void RotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "rotation transform input");
    out.assign(static_cast<std::size_t>(output_dimension_), 0.0);
    for (int r = 0; r < output_dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_xopt_[rr];
        for (int c = 0; c < input_dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - assigned_xopt_[static_cast<std::size_t>(c)]);
        }
    }
}

int RotationTransform::input_dimension() const {
    return input_dimension_;
}

int RotationTransform::output_dimension() const {
    return output_dimension_;
}

CoordinateTransformClass RotationTransform::transform_class() const {
    return CoordinateTransformClass::Rotation;
}

const std::vector<std::vector<double>>& RotationTransform::matrix() const {
    return matrix_;
}

AffineTransform::AffineTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : CoordinateTransform(dimension, dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, output_dimension_, "affine transform assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "affine transform target_xopt");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_affine_matrix(rng, output_dimension_);
}

AffineTransform::AffineTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(dimension, dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      matrix_(std::move(matrix)) {
    require_dimension(assigned_xopt_, output_dimension_, "affine transform assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "affine transform target_xopt");
    require_matrix_shape(matrix_, output_dimension_, input_dimension_, "affine transform matrix");
}

void AffineTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "affine transform input");
    out.assign(static_cast<std::size_t>(output_dimension_), 0.0);
    for (int r = 0; r < output_dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_xopt_[rr];
        for (int c = 0; c < input_dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - assigned_xopt_[static_cast<std::size_t>(c)]);
        }
    }
}

int AffineTransform::input_dimension() const {
    return input_dimension_;
}

int AffineTransform::output_dimension() const {
    return output_dimension_;
}

CoordinateTransformClass AffineTransform::transform_class() const {
    return CoordinateTransformClass::Affine;
}

const std::vector<std::vector<double>>& AffineTransform::matrix() const {
    return matrix_;
}

BlockRotationTransform::BlockRotationTransform(
    int dimension,
    std::vector<int> selected_indices,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : CoordinateTransform(
          dimension,
          static_cast<int>(selected_indices.size()),
          std::move(assigned_xopt),
          std::move(target_xopt),
          seed),
      selected_indices_(std::move(selected_indices)) {
    require(!selected_indices_.empty(), "block rotation transform needs at least one selected index");
    require_unique_selected_indices(selected_indices_, input_dimension_);
    require_dimension(assigned_xopt_, output_dimension_, "block rotation assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "block rotation target_xopt");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, output_dimension_);
}

BlockRotationTransform::BlockRotationTransform(
    int dimension,
    std::vector<int> selected_indices,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(
          dimension,
          static_cast<int>(selected_indices.size()),
          std::move(assigned_xopt),
          std::move(target_xopt),
          seed),
      selected_indices_(std::move(selected_indices)),
      matrix_(std::move(matrix)) {
    require(!selected_indices_.empty(), "block rotation transform needs at least one selected index");
    require_unique_selected_indices(selected_indices_, input_dimension_);
    require_dimension(assigned_xopt_, output_dimension_, "block rotation assigned_xopt");
    require_dimension(target_xopt_, output_dimension_, "block rotation target_xopt");
    require_matrix_shape(matrix_, output_dimension_, output_dimension_, "block rotation transform matrix");
}

void BlockRotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "block rotation transform input");
    out.assign(selected_indices_.size(), 0.0);

    for (std::size_t r = 0; r < selected_indices_.size(); ++r) {
        out[r] = target_xopt_[r];
        for (std::size_t c = 0; c < selected_indices_.size(); ++c) {
            const auto in_idx = static_cast<std::size_t>(selected_indices_[c]);
            out[r] += matrix_[r][c] * (x[in_idx] - assigned_xopt_[c]);
        }
    }
}

int BlockRotationTransform::input_dimension() const {
    return input_dimension_;
}

int BlockRotationTransform::output_dimension() const {
    return output_dimension_;
}

CoordinateTransformClass BlockRotationTransform::transform_class() const {
    return CoordinateTransformClass::BlockRotation;
}

const std::vector<int>& BlockRotationTransform::selected_indices() const {
    return selected_indices_;
}

const std::vector<std::vector<double>>& BlockRotationTransform::matrix() const {
    return matrix_;
}

} // namespace FuncCraft

