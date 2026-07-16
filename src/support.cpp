#include "support.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace FuncCraft {
namespace detail {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}

void require_dimension(const std::vector<double>& x, int dimension, const std::string& name) {
    require(static_cast<int>(x.size()) == dimension, name + " dimension mismatch");
}

double squared_distance(const std::vector<double>& a, const std::vector<double>& b) {
    require(a.size() == b.size(), "distance dimension mismatch");
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const double d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

double weighted_sum(const std::vector<double>& weights, const std::vector<double>& z) {
    require(weights.size() == z.size(), "weight/component size mismatch");
    double result = 0.0;
    for (std::size_t i = 0; i < z.size(); ++i) {
        require(weights[i] >= 0.0, "weights must be nonnegative");
        result += weights[i] * z[i];
    }
    return result;
}

std::string join_base_ids(const std::vector<BasicFunctionId>& ids) {
    std::ostringstream out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            out << "+";
        }
        out << to_string(ids[i]);
    }
    return out.str();
}

std::uint64_t mix_seed(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

double uniform01(std::mt19937_64& rng) {
    return std::generate_canonical<double, 53>(rng);
}

int uniform_int(std::mt19937_64& rng, int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

double normal01(std::mt19937_64& rng) {
    std::normal_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

std::vector<std::vector<double>> random_rotation_matrix(std::mt19937_64& rng, int dimension) {
    std::vector<std::vector<double>> q(
        static_cast<std::size_t>(dimension),
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0));
    std::vector<double> column(static_cast<std::size_t>(dimension), 0.0);

    for (int col = 0; col < dimension; ++col) {
        while (true) {
            for (double& value : column) {
                value = normal01(rng);
            }
            for (int prev = 0; prev < col; ++prev) {
                double dot = 0.0;
                for (int row = 0; row < dimension; ++row) {
                    dot += column[static_cast<std::size_t>(row)] *
                           q[static_cast<std::size_t>(row)][static_cast<std::size_t>(prev)];
                }
                for (int row = 0; row < dimension; ++row) {
                    column[static_cast<std::size_t>(row)] -=
                        dot * q[static_cast<std::size_t>(row)][static_cast<std::size_t>(prev)];
                }
            }

            double norm_sq = 0.0;
            for (double value : column) {
                norm_sq += value * value;
            }
            if (norm_sq <= 1.0e-14) {
                continue;
            }

            const double inv_norm = 1.0 / std::sqrt(norm_sq);
            for (int row = 0; row < dimension; ++row) {
                q[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] =
                    column[static_cast<std::size_t>(row)] * inv_norm;
            }
            break;
        }
    }
    return q;
}

std::vector<std::vector<double>> random_affine_matrix(std::mt19937_64& rng, int dimension) {
    auto matrix = random_rotation_matrix(rng, dimension);
    for (int row = 0; row < dimension; ++row) {
        const double exponent = dimension > 1
            ? static_cast<double>(row) / static_cast<double>(dimension - 1)
            : 0.0;
        const double scale = std::pow(10.0, 2.0 * exponent);
        for (int col = 0; col < dimension; ++col) {
            matrix[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] *= scale;
        }
    }
    return matrix;
}

} // namespace detail
} // namespace FuncCraft
