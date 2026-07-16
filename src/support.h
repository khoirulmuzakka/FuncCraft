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

namespace FuncCraft {
namespace detail {

constexpr double kPi = 3.14159265358979323846;

/**
 * @brief Throw if the condition is false.
 */
void require(bool condition, const std::string& message);
/**
 * @brief Validate that a point has the expected dimension.
 */
void require_dimension(const std::vector<double>& x, int dimension, const std::string& name);
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

} // namespace detail
} // namespace FuncCraft

#endif // FUNCCRAFT_SUPPORT_H
