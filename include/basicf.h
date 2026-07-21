#ifndef FUNCCRAFT_BASICF_H
#define FUNCCRAFT_BASICF_H

/**
 * @file basicf.h
 * @brief Primitive benchmark functions and their identifiers.
 *
 * This header defines the canonical base benchmark landscapes used by the
 * higher-level composition layer.
 */

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace FuncCraft {

struct Domain;

enum class BasicFunctionId {
    /// Sphere function.
    Sphere,
    /// Ellipsoidal function.
    Ellipsoidal,
    /// Sum of different powers.
    SumDifferentPowers,
    /// Bueche-Rastrigin function.
    BuecheRastrigin,
    /// Linear slope function.
    LinearSlope,
    /// Attractive sector function.
    AttractiveSector,
    /// Step ellipsoidal function.
    StepEllipsoidal,
    /// Step Rastrigin function.
    StepRastrigin,
    /// Rosenbrock function.
    Rosenbrock,
    /// Ackley function.
    Ackley,
    /// Rastrigin function.
    Rastrigin,
    /// Griewank function.
    Griewank,
    /// Schwefel function.
    Schwefel,
    /// Sharp ridge function.
    SharpRidge,
    /// Different powers function.
    DifferentPowers,
    /// Weierstrass function.
    Weierstrass,
    /// Schaffer F7 function.
    SchafferF7,
    /// Schaffer F7 conditioned on 1000.
    SchafferF7Cond1000,
    /// Griewank-Rosenbrock function.
    GriewankRosenbrock,
    /// Gallagher 21 peaks function.
    Gallagher21,
    /// Katsuura function.
    Katsuura,
    /// Lunacek bi-Rastrigin function.
    LunacekBiRastrigin,
    /// Zakharov function.
    Zakharov,
    /// Levy function.
    Levy,
    /// Michalewicz function.
    Michalewicz,
    /// Dixon-Price function.
    DixonPrice,
    /// Bent cigar function.
    BentCigar,
    /// Discus function.
    Discus,
    /// HappyCat function.
    HappyCat,
    /// HGBat function.
    HGBat,
    /// HCF function.
    HCF,
    /// Griewank-Rosenbrock function.
    GrieRosen,
    /// Schaffer F6 function.
    SchafferF6,
    /// Step function.
    Step,
    /// Quartic function.
    Quartic,
    /// Brown function.
    Brown = 36,
    /// Exponential function.
    Exponential,
    /// Styblinski-Tang function.
    StyblinskiTang,
    /// Sum squares function.
    SumSquares,
};

/**
 * @brief Canonical basic benchmark function.
 *
 * A `BasicF` is a normalized primitive benchmark landscape. All built-in basic
 * functions are exposed in primitive coordinates:
 * - `dimension` is the requested problem dimension;
 * - `default_domain` is the canonical plotting/evaluation box for the primitive;
 * - `x_opt` stores the global optimum of the implemented primitive;
 * - `f_opt` stores the primitive minimum value;
 * - `properties` is a human-readable summary of the landscape.
 *
 * For CEC-derived primitives, the implementation follows the unshifted CEC base
 * formula, including any internal scale factor and any intrinsic optimum
 * location implied by that formula.
 *
 * BBOB-style "rotated" primitives are also exposed here. At this low level they
 * are stored in canonical coordinates without any implicit random rotation;
 * callers that want rotated variants should apply rotation explicitly, or use a
 * higher-level wrapper such as `MinionBenchmark` with `useRotation=true`.
 */
class BasicF {
public:
    /**
     * @brief Construct one primitive benchmark function.
     */
    BasicF(BasicFunctionId id, int dimension);

    /**
     * @brief Batch-evaluate the primitive at the supplied points.
     */
    std::vector<double> operator()(const std::vector<std::vector<double>>& X) const;
    /**
     * @brief Return the primitive identifier.
     */
    BasicFunctionId id() const;

    std::string name;
    int dimension = 0;
    std::vector<double> x_opt;
    double f_opt = 0.0;
    std::string properties;

    /**
     * @brief Return the canonical domain recommended for plotting this primitive.
     */
    Domain default_domain() const;

private:
    struct PeakData {
        std::vector<double> center;
        double weight = 1.0;
    };

    void initialize_state();
    double evaluate_impl(const double* x) const;

    BasicFunctionId id_;
    std::vector<PeakData> peaks_;
};

/**
 * @brief Convert a basic-function identifier to a human-readable name.
 */
std::string to_string(BasicFunctionId id);
/**
 * @brief Return true if the primitive is multimodal.
 */
bool is_multimodal(BasicFunctionId id);
/**
 * @brief Return true if the primitive is unimodal.
 */
bool is_unimodal(BasicFunctionId id);
/**
 * @brief Return the ordered list of supported base functions.
 */
std::vector<BasicFunctionId> list_basic_functions();
/**
 * @brief Return the subset of supported unimodal base functions.
 */
std::vector<BasicFunctionId> unimodalF_list();
/**
 * @brief Return the subset of supported multimodal base functions.
 */
std::vector<BasicFunctionId> multimodalF_list();
/**
 * @brief Create a heap-allocated primitive benchmark function.
 */
std::shared_ptr<BasicF> make_basicf_ptr(BasicFunctionId id, int dimension);

} // namespace FuncCraft

#endif // FUNCCRAFT_BASICF_H
