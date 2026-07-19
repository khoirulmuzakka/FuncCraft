#include "coordinate_transforms.h"
#include "support.h"

#include <random>
#include <utility>

namespace FuncCraft {
using namespace detail;

CoordinateTransform::CoordinateTransform(
    int dimension,
    std::vector<double> source_point,
    std::vector<double> target_point,
    std::uint64_t seed)
    : dimension_(dimension),
      source_point_(std::move(source_point)),
      target_point_(std::move(target_point)),
      seed_(seed) {
    require(dimension_ > 0, "coordinate transform dimension must be positive");
    require(!source_point_.empty(), "coordinate transform source point must not be empty");
    require(!target_point_.empty(), "coordinate transform target point must not be empty");
}

int CoordinateTransform::dimension() const {
    return dimension_;
}

std::uint64_t CoordinateTransform::seed() const {
    return seed_;
}

const std::vector<double>& CoordinateTransform::source_point() const {
    return source_point_;
}

const std::vector<double>& CoordinateTransform::target_point() const {
    return target_point_;
}

IdentityTransform::IdentityTransform(
    int dimension,
    std::vector<double> source_point,
    std::vector<double> target_point,
    std::uint64_t seed)
    : CoordinateTransform(dimension, std::move(source_point), std::move(target_point), seed) {
    require_dimension(source_point_, dimension_, "identity transform source point");
    require_dimension(target_point_, dimension_, "identity transform target point");
}

std::vector<double> IdentityTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension(), "identity transform input");
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

TransformSpec IdentityTransform::spec() const {
    TransformSpec spec;
    spec.kind = "identity";
    spec.dimension = dimension_;
    spec.seed = static_cast<int>(seed_);
    spec.source_point = source_point_;
    spec.target_point = target_point_;
    return spec;
}

RotationTransform::RotationTransform(
    int dimension,
    std::vector<double> source_point,
    std::vector<double> target_point,
    std::uint64_t seed)
    : CoordinateTransform(dimension, std::move(source_point), std::move(target_point), seed) {
    require_dimension(source_point_, dimension_, "rotation transform source point");
    require_dimension(target_point_, dimension_, "rotation transform target point");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, dimension_);
}

std::vector<double> RotationTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension(), "rotation transform input");
    std::vector<double> y(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        y[rr] = target_point_[rr];
        for (int c = 0; c < dimension_; ++c) {
            y[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - source_point_[static_cast<std::size_t>(c)]);
        }
    }
    return y;
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

TransformSpec RotationTransform::spec() const {
    TransformSpec spec;
    spec.kind = "rotation";
    spec.dimension = dimension_;
    spec.seed = static_cast<int>(seed_);
    spec.source_point = source_point_;
    spec.target_point = target_point_;
    return spec;
}

AffineTransform::AffineTransform(
    int dimension,
    std::vector<double> source_point,
    std::vector<double> target_point,
    std::uint64_t seed)
    : CoordinateTransform(dimension, std::move(source_point), std::move(target_point), seed) {
    require_dimension(source_point_, dimension_, "affine transform source point");
    require_dimension(target_point_, dimension_, "affine transform target point");
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_affine_matrix(rng, dimension_);
}

std::vector<double> AffineTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension(), "affine transform input");
    std::vector<double> y(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        y[rr] = target_point_[rr];
        for (int c = 0; c < dimension_; ++c) {
            y[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - source_point_[static_cast<std::size_t>(c)]);
        }
    }
    return y;
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

TransformSpec AffineTransform::spec() const {
    TransformSpec spec;
    spec.kind = "affine";
    spec.dimension = dimension_;
    spec.seed = static_cast<int>(seed_);
    spec.source_point = source_point_;
    spec.target_point = target_point_;
    return spec;
}

BlockRotationTransform::BlockRotationTransform(
    int dimension,
    std::vector<int> selected_indices,
    std::vector<double> source_point,
    std::vector<double> target_point,
    std::uint64_t seed)
    : CoordinateTransform(dimension, std::move(source_point), std::move(target_point), seed),
      selected_indices_(std::move(selected_indices)) {
    require(dimension_ > 0, "block rotation transform dimension must be positive");
    require(!selected_indices_.empty(), "block rotation transform needs at least one selected index");
    require_dimension(source_point_, dimension_, "block rotation source point");
    require_dimension(target_point_, dimension_, "block rotation target point");
    for (int idx : selected_indices_) {
        require(idx >= 0 && idx < dimension_, "block rotation selected index out of range");
    }
    std::mt19937_64 rng(mix_seed(seed_));
    matrix_ = random_rotation_matrix(rng, static_cast<int>(selected_indices_.size()));
}

std::vector<double> BlockRotationTransform::apply(const std::vector<double>& x) const {
    require_dimension(x, input_dimension(), "block rotation transform input");
    std::vector<double> y = target_point_;
    std::vector<double> selected(selected_indices_.size(), 0.0);
    for (std::size_t i = 0; i < selected_indices_.size(); ++i) {
        const auto idx = static_cast<std::size_t>(selected_indices_[i]);
        selected[i] = x[idx] - source_point_[idx];
    }

    for (std::size_t r = 0; r < selected_indices_.size(); ++r) {
        const auto out_idx = static_cast<std::size_t>(selected_indices_[r]);
        y[out_idx] = target_point_[out_idx];
        for (std::size_t c = 0; c < selected_indices_.size(); ++c) {
            y[out_idx] += matrix_[r][c] * selected[c];
        }
    }
    return y;
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

TransformSpec BlockRotationTransform::spec() const {
    TransformSpec spec;
    spec.kind = "block_rotation";
    spec.dimension = dimension_;
    spec.seed = static_cast<int>(seed_);
    spec.selected_indices = selected_indices_;
    spec.source_point = source_point_;
    spec.target_point = target_point_;
    return spec;
}

const std::vector<int>& BlockRotationTransform::selected_indices() const {
    return selected_indices_;
}

} // namespace FuncCraft
