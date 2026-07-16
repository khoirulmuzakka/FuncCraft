#ifndef FUNCCRAFT_CORE_H
#define FUNCCRAFT_CORE_H

#include "basicf.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace FuncCraft {

enum class CompositionClass {
    None,
    CommonPointWeightedSum,
    CommonPointPowerMean,
    CommonPointLevelWell,
    DeceptivePointSoftmax,
};

enum class ValueTransformClass {
    None,
    CosineZero,
    Oscillatory,
    Power,
    Mixed,
};

enum class CoordinateTransformClass {
    None,
    Rotation,
    Block,
    Affine,
    NonlinearFold,
    Mixed,
};

struct Domain {
    std::vector<double> lower;
    std::vector<double> upper;

    explicit Domain(int dimension = 0, double lower_bound = -100.0, double upper_bound = 100.0);
    int dimension() const;
    bool contains(const std::vector<double>& x) const;
};

struct FunctionClass {
    CompositionClass composition = CompositionClass::None;
    ValueTransformClass value_transform = ValueTransformClass::None;
    CoordinateTransformClass coordinate_transform = CoordinateTransformClass::None;
    std::vector<BasicFunctionId> base_functions;
};

struct FunctionMetadata {
    FunctionClass function_class;
    int dimension = 0;
    int components = 0;
    unsigned long long seed = 0;
    std::vector<double> known_global_minimizer;
    double known_global_value = 0.0;
    std::map<std::string, std::string> parameters;
};

std::string to_string(CompositionClass cls);
std::string to_string(ValueTransformClass cls);
std::string to_string(CoordinateTransformClass cls);
std::string class_label(const FunctionClass& cls);

class BaseFunction {
public:
    virtual ~BaseFunction() = default;
    virtual double evaluate(const std::vector<double>& x) const = 0;
    virtual int dimension() const = 0;
    virtual std::vector<double> minimizer() const = 0;
    virtual double minimum_value() const = 0;
    virtual std::string name() const = 0;
};

class BasicFunction final : public BaseFunction {
public:
    BasicFunction(BasicFunctionId id, int dimension, bool shift_to_zero = true);

    double evaluate(const std::vector<double>& x) const override;
    int dimension() const override;
    std::vector<double> minimizer() const override;
    double minimum_value() const override;
    std::string name() const override;
    BasicFunctionId id() const;

private:
    BasicF function_;
    bool shift_to_zero_ = true;
};

} // namespace FuncCraft

#endif // FUNCCRAFT_CORE_H
