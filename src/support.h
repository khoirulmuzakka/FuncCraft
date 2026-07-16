#ifndef FUNCCRAFT_SUPPORT_H
#define FUNCCRAFT_SUPPORT_H

#include "core.h"

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace FuncCraft {
namespace detail {

constexpr double kPi = 3.14159265358979323846;

void require(bool condition, const std::string& message);
void require_dimension(const std::vector<double>& x, int dimension, const std::string& name);
double squared_distance(const std::vector<double>& a, const std::vector<double>& b);
double weighted_sum(const std::vector<double>& weights, const std::vector<double>& z);
std::string join_base_ids(const std::vector<BasicFunctionId>& ids);
std::uint64_t mix_seed(std::uint64_t x);
double uniform01(std::mt19937_64& rng);
int uniform_int(std::mt19937_64& rng, int lo, int hi);
double normal01(std::mt19937_64& rng);

template <typename T>
void stable_shuffle(std::vector<T>& values, std::mt19937_64& rng) {
    for (int i = static_cast<int>(values.size()) - 1; i > 0; --i) {
        const int j = uniform_int(rng, 0, i);
        std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(j)]);
    }
}

std::vector<std::vector<double>> random_rotation_matrix(std::mt19937_64& rng, int dimension);
std::vector<std::vector<double>> random_affine_matrix(std::mt19937_64& rng, int dimension);

} // namespace detail
} // namespace FuncCraft

#endif // FUNCCRAFT_SUPPORT_H
