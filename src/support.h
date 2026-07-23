#ifndef FUNCCRAFT_SUPPORT_H
#define FUNCCRAFT_SUPPORT_H

/**
 * @file support.h
 * @brief Internal helper routines shared by the benchmark implementation.
 *
 * This header contains validation helpers, numeric utilities, and random
 * matrix generators used by the implementation files.
 */

#include "core.h"

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace FuncCraft {

struct FunctionSpec;
struct SuiteSpec;

namespace detail {

constexpr double kPi = 3.14159265358979323846;
constexpr std::uint64_t kAssignedXoptSeedRole = 0xA5516EED0D7ULL;
constexpr std::uint64_t kDpmCenterSeedRole = 0xD9CE17E25ULL;

/**
 * @brief Throw if the condition is false.
 */
void require(bool condition, const std::string& message);
/**
 * @brief Validate that a point has the expected dimension.
 */
void require_dimension(const std::vector<double>& x, int dimension, const std::string& name);
/**
 * @brief Map one point from one axis-aligned domain into another.
 */
void map_point_between_domains(
    const std::vector<double>& point,
    const Domain& source_domain,
    const Domain& target_domain,
    std::vector<double>& out);
/**
 * @brief Map one point from one axis-aligned domain into another.
 */
std::vector<double> map_point_between_domains(
    const std::vector<double>& point,
    const Domain& source_domain,
    const Domain& target_domain);
/**
 * @brief Compute squared Euclidean distance.
 */
double squared_distance(const std::vector<double>& a, const std::vector<double>& b);
/**
 * @brief Compute a weighted sum of values.
 */
double weighted_sum(const std::vector<double>& weights, const std::vector<double>& z);
/**
 * @brief Join base-function identifiers into a compact metadata string.
 */
std::string join_base_ids(const std::vector<BasicFunctionId>& ids);
/**
 * @brief Mix one 64-bit seed into a new deterministic seed.
 */
std::uint64_t mix_seed(std::uint64_t x);
/**
 * @brief Generate a deterministic child seed from a base seed, role, and two indices.
 */
std::uint64_t indexed_seed(std::uint64_t seed, std::uint64_t role, std::uint64_t index0, std::uint64_t index1);
/**
 * @brief Draw one uniform random number from [0, 1).
 */
double uniform01(std::mt19937_64& rng);
/**
 * @brief Draw one integer uniformly from [lo, hi].
 */
int uniform_int(std::mt19937_64& rng, int lo, int hi);
/**
 * @brief Draw one standard normal random variate.
 */
double normal01(std::mt19937_64& rng);
/**
 * @brief Return a centered copy of a domain scaled by factor.
 */
Domain centered_scaled_domain(const Domain& domain, double factor);
/**
 * @brief Generate Latin-hypercube points whose coordinate prefixes are stable across dimensions.
 */
std::vector<std::vector<double>> prefix_stable_latin_hypercube_points_in_domain(
    std::uint64_t seed,
    std::uint64_t role,
    const Domain& domain,
    int count);
/**
 * @brief Generate centered Latin-hypercube points whose coordinate prefixes are stable across dimensions.
 */
std::vector<std::vector<double>> prefix_stable_latin_hypercube_centers(
    std::uint64_t seed,
    std::uint64_t role,
    const Domain& domain,
    int count,
    double shrink_factor);
/**
 * @brief Complete a user-supplied coordinate prefix to the domain dimension deterministically.
 */
std::vector<double> complete_prefix_stable_point(
    std::uint64_t seed,
    std::uint64_t role,
    const std::vector<double>& prefix,
    const Domain& domain,
    double shrink_factor);
/**
 * @brief Normalize spec identifiers for matching.
 *
 * The normalization rule is intentionally simple: lower case and remove
 * underscores, hyphens, and whitespace.
 */
std::string normalize_spec_name(const std::string& value);
/**
 * @brief Return the canonical coordinate-transform spec kind.
 */
std::string canonical_coordinate_transform_kind(const std::string& kind);
/**
 * @brief Return the canonical value-transform spec kind.
 */
std::string canonical_value_transform_kind(const std::string& kind);
/**
 * @brief Return the canonical composition spec kind.
 */
std::string canonical_composition_kind(const std::string& kind);
CoordinateTransformKind parse_coordinate_transform_kind(const std::string& kind);
ValueTransformKind parse_value_transform_kind(const std::string& kind);
CompositionKind parse_composition_kind(const std::string& kind);
CompositionMode composition_mode(CompositionKind kind);
std::string to_spec_name(CoordinateTransformKind kind);
std::string to_spec_name(ValueTransformKind kind);
std::string to_spec_name(CompositionKind kind);

template <typename T>
void stable_shuffle(std::vector<T>& values, std::mt19937_64& rng) {
    for (int i = static_cast<int>(values.size()) - 1; i > 0; --i) {
        const int j = uniform_int(rng, 0, i);
        std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(j)]);
    }
}

/**
 * @brief Generate a random orthogonal rotation matrix.
 */
std::vector<std::vector<double>> random_rotation_matrix(std::mt19937_64& rng, int dimension);
/**
 * @brief Generate a random affine transform matrix.
 */
std::vector<std::vector<double>> random_affine_matrix(std::mt19937_64& rng, int dimension);
/**
 * @brief Convert a generated function spec to a YAML node.
 */
YAML::Node function_spec_to_yaml(const FunctionSpec& spec);
/**
 * @brief Load a function spec from a YAML node.
 */
FunctionSpec function_spec_from_yaml(const YAML::Node& node);
/**
 * @brief Convert a suite spec to a YAML node.
 */
YAML::Node suite_spec_to_yaml(const SuiteSpec& spec);
/**
 * @brief Write a YAML node to disk.
 */
void write_yaml_file(const std::string& path, const YAML::Node& node);

} // namespace detail
} // namespace FuncCraft

#endif // FUNCCRAFT_SUPPORT_H
