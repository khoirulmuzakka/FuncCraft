#include "coordinate_transforms.h"
#include "support.h"

#include <random>
#include <utility>

namespace FuncCraft {
using namespace detail;

namespace {

void require_matrix_shape(const std::vector<std::vector<double>>& matrix, int rows, int cols, const std::string& name) {
    require(static_cast<int>(matrix.size()) == rows, name + " row count mismatch");
    for (const auto& row : matrix) {
        require(static_cast<int>(row.size()) == cols, name + " column count mismatch");
    }
}

} // namespace

CoordinateTransform::CoordinateTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : dimension_(dimension),
      assigned_xopt_(std::move(assigned_xopt)),
      target_xopt_(std::move(target_xopt)),
      seed_(seed) {
    require(dimension_ > 0, "coordinate transform dimension must be positive");
    require(!assigned_xopt_.empty(), "coordinate transform assigned_xopt must not be empty");
    require(!target_xopt_.empty(), "coordinate transform target_xopt must not be empty");
}

int CoordinateTransform::dimension() const {
    return dimension_;
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
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, dimension_, "identity transform assigned_xopt");
    require_dimension(target_xopt_, dimension_, "identity transform target_xopt");
}

void IdentityTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "identity transform input");
    out.assign(static_cast<std::size_t>(dimension_), 0.0);
    for (int i = 0; i < dimension_; ++i) {
        const auto idx = static_cast<std::size_t>(i);
        out[idx] = target_xopt_[idx] + (x[idx] - assigned_xopt_[idx]);
    }
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

RotationTransform::RotationTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed)
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, dimension_, "rotation transform assigned_xopt");
    require_dimension(target_xopt_, dimension_, "rotation transform target_xopt");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, dimension_);
}

RotationTransform::RotationTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      matrix_(std::move(matrix)) {
    require_dimension(assigned_xopt_, dimension_, "rotation transform assigned_xopt");
    require_dimension(target_xopt_, dimension_, "rotation transform target_xopt");
    require_matrix_shape(matrix_, dimension_, dimension_, "rotation transform matrix");
}

void RotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "rotation transform input");
    out.assign(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_xopt_[rr];
        for (int c = 0; c < dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - assigned_xopt_[static_cast<std::size_t>(c)]);
        }
    }
}

int RotationTransform::input_dimension() const {
    return dimension_;
}

int RotationTransform::output_dimension() const {
    return dimension_;
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
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed) {
    require_dimension(assigned_xopt_, dimension_, "affine transform assigned_xopt");
    require_dimension(target_xopt_, dimension_, "affine transform target_xopt");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_affine_matrix(rng, dimension_);
}

AffineTransform::AffineTransform(
    int dimension,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      matrix_(std::move(matrix)) {
    require_dimension(assigned_xopt_, dimension_, "affine transform assigned_xopt");
    require_dimension(target_xopt_, dimension_, "affine transform target_xopt");
    require_matrix_shape(matrix_, dimension_, dimension_, "affine transform matrix");
}

void AffineTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "affine transform input");
    out.assign(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_xopt_[rr];
        for (int c = 0; c < dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - assigned_xopt_[static_cast<std::size_t>(c)]);
        }
    }
}

int AffineTransform::input_dimension() const {
    return dimension_;
}

int AffineTransform::output_dimension() const {
    return dimension_;
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
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      selected_indices_(std::move(selected_indices)) {
    require(dimension_ > 0, "block rotation transform dimension must be positive");
    require(!selected_indices_.empty(), "block rotation transform needs at least one selected index");
    require_dimension(assigned_xopt_, dimension_, "block rotation assigned_xopt");
    require_dimension(target_xopt_, dimension_, "block rotation target_xopt");
    for (int idx : selected_indices_) {
        require(idx >= 0 && idx < dimension_, "block rotation selected index out of range");
    }
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, static_cast<int>(selected_indices_.size()));
}

BlockRotationTransform::BlockRotationTransform(
    int dimension,
    std::vector<int> selected_indices,
    std::vector<double> assigned_xopt,
    std::vector<double> target_xopt,
    std::uint64_t seed,
    std::vector<std::vector<double>> matrix)
    : CoordinateTransform(dimension, std::move(assigned_xopt), std::move(target_xopt), seed),
      selected_indices_(std::move(selected_indices)),
      matrix_(std::move(matrix)) {
    require(dimension_ > 0, "block rotation transform dimension must be positive");
    require(!selected_indices_.empty(), "block rotation transform needs at least one selected index");
    require_dimension(assigned_xopt_, dimension_, "block rotation assigned_xopt");
    require_dimension(target_xopt_, dimension_, "block rotation target_xopt");
    for (int idx : selected_indices_) {
        require(idx >= 0 && idx < dimension_, "block rotation selected index out of range");
    }
    const int block_size = static_cast<int>(selected_indices_.size());
    require_matrix_shape(matrix_, block_size, block_size, "block rotation transform matrix");
}

void BlockRotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "block rotation transform input");
    out = target_xopt_;

    for (std::size_t r = 0; r < selected_indices_.size(); ++r) {
        const auto out_idx = static_cast<std::size_t>(selected_indices_[r]);
        out[out_idx] = target_xopt_[out_idx];
        for (std::size_t c = 0; c < selected_indices_.size(); ++c) {
            const auto in_idx = static_cast<std::size_t>(selected_indices_[c]);
            out[out_idx] += matrix_[r][c] * (x[in_idx] - assigned_xopt_[in_idx]);
        }
    }
}

int BlockRotationTransform::input_dimension() const {
    return dimension_;
}

int BlockRotationTransform::output_dimension() const {
    return dimension_;
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

