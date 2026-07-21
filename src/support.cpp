#include "support.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace FuncCraft {
namespace detail {

namespace {

double uniform_signed01(std::mt19937_64& rng) {
    return 2.0 * uniform01(rng) - 1.0;
}

double matrix_determinant(std::vector<std::vector<double>> matrix) {
    const std::size_t n = matrix.size();
    double det = 1.0;
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t pivot = i;
        double pivot_abs = std::fabs(matrix[i][i]);
        for (std::size_t r = i + 1; r < n; ++r) {
            const double candidate = std::fabs(matrix[r][i]);
            if (candidate > pivot_abs) {
                pivot_abs = candidate;
                pivot = r;
            }
        }
        if (pivot_abs < 1.0e-15) {
            return 0.0;
        }
        if (pivot != i) {
            std::swap(matrix[pivot], matrix[i]);
            det = -det;
        }
        const double pivot_value = matrix[i][i];
        det *= pivot_value;
        for (std::size_t r = i + 1; r < n; ++r) {
            const double factor = matrix[r][i] / pivot_value;
            for (std::size_t c = i; c < n; ++c) {
                matrix[r][c] -= factor * matrix[i][c];
            }
        }
    }
    return det;
}

} // namespace

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}

void require_dimension(const std::vector<double>& x, int dimension, const std::string& name) {
    require(static_cast<int>(x.size()) == dimension, name + " dimension mismatch");
}

void map_point_to_default_domain(const std::vector<double>& point, const Domain& domain, std::vector<double>& out) {
    require_dimension(point, domain.dimension(), "mapping point");
    constexpr double kDefaultDomainLower = -5.0;
    constexpr double kDefaultDomainUpper = 5.0;

    out.assign(point.size(), 0.0);
    for (std::size_t i = 0; i < point.size(); ++i) {
        const double lo = domain.lower[i];
        const double hi = domain.upper[i];
        if (hi == lo) {
            out[i] = 0.0;
            continue;
        }
        const double t = (point[i] - lo) / (hi - lo);
        out[i] = kDefaultDomainLower + t * (kDefaultDomainUpper - kDefaultDomainLower);
    }
}

std::vector<double> map_point_from_default_domain(const std::vector<double>& point, const Domain& domain) {
    require_dimension(point, domain.dimension(), "inverse mapping point");
    constexpr double kDefaultDomainLower = -5.0;
    constexpr double kDefaultDomainUpper = 5.0;

    std::vector<double> mapped(point.size(), 0.0);
    for (std::size_t i = 0; i < point.size(); ++i) {
        const double lo = domain.lower[i];
        const double hi = domain.upper[i];
        if (hi == lo) {
            mapped[i] = lo;
            continue;
        }
        const double t = (point[i] - kDefaultDomainLower) / (kDefaultDomainUpper - kDefaultDomainLower);
        mapped[i] = lo + t * (hi - lo);
    }
    return mapped;
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
    double u1 = uniform01(rng);
    while (u1 <= 0.0) {
        u1 = uniform01(rng);
    }
    const double u2 = uniform01(rng);
    const double r = std::sqrt(-2.0 * std::log(u1));
    const double theta = 2.0 * kPi * u2;
    return r * std::cos(theta);
}

std::vector<std::vector<double>> random_rotation_matrix(std::mt19937_64& rng, int dimension) {
    require(dimension > 0, "rotation matrix dimension must be positive");

    std::vector<std::vector<double>> q(
        static_cast<std::size_t>(dimension),
        std::vector<double>(static_cast<std::size_t>(dimension), 0.0));
    std::vector<double> column(static_cast<std::size_t>(dimension), 0.0);

    for (int col = 0; col < dimension; ++col) {
        bool accepted = false;
        for (int attempt = 0; attempt < 128 && !accepted; ++attempt) {
            for (double& value : column) {
                value = uniform_signed01(rng);
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
            accepted = true;
        }
        require(accepted, "could not generate a stable rotation matrix");
    }

    if (matrix_determinant(q) < 0.0) {
        for (int row = 0; row < dimension; ++row) {
            q[static_cast<std::size_t>(row)][0] = -q[static_cast<std::size_t>(row)][0];
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
