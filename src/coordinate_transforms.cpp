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

void IdentityTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "identity transform input");
    out = x;
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

void RotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "rotation transform input");
    out.assign(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_point_[rr];
        for (int c = 0; c < dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - source_point_[static_cast<std::size_t>(c)]);
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

void AffineTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "affine transform input");
    out.assign(static_cast<std::size_t>(dimension_), 0.0);
    for (int r = 0; r < dimension_; ++r) {
        const auto rr = static_cast<std::size_t>(r);
        out[rr] = target_point_[rr];
        for (int c = 0; c < dimension_; ++c) {
            out[rr] += matrix_[rr][static_cast<std::size_t>(c)]
                * (x[static_cast<std::size_t>(c)] - source_point_[static_cast<std::size_t>(c)]);
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

void BlockRotationTransform::apply(const std::vector<double>& x, std::vector<double>& out) const {
    require_dimension(x, input_dimension(), "block rotation transform input");
    out = target_point_;

    for (std::size_t r = 0; r < selected_indices_.size(); ++r) {
        const auto out_idx = static_cast<std::size_t>(selected_indices_[r]);
        out[out_idx] = target_point_[out_idx];
        for (std::size_t c = 0; c < selected_indices_.size(); ++c) {
            const auto in_idx = static_cast<std::size_t>(selected_indices_[c]);
            out[out_idx] += matrix_[r][c] * (x[in_idx] - source_point_[in_idx]);
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
