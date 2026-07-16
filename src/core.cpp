#include "core.h"
#include "support.h"

#include <cmath>
#include <sstream>

namespace FuncCraft {
using namespace detail;

Domain::Domain(int dimension, double lower_bound, double upper_bound)
    : lower(static_cast<std::size_t>(dimension), lower_bound),
      upper(static_cast<std::size_t>(dimension), upper_bound) {
    require(dimension >= 0, "domain dimension must be nonnegative");
    require(lower_bound <= upper_bound, "domain lower bound must not exceed upper bound");
}

int Domain::dimension() const {
    require(lower.size() == upper.size(), "domain bound size mismatch");
    return static_cast<int>(lower.size());
}

bool Domain::contains(const std::vector<double>& x) const {
    if (x.size() != lower.size()) {
        return false;
    }
    for (std::size_t i = 0; i < x.size(); ++i) {
        if (x[i] < lower[i] || x[i] > upper[i]) {
            return false;
        }
    }
    return true;
}

std::string to_string(CompositionClass cls) {
    switch (cls) {
    case CompositionClass::None:
        return "NONE";
    case CompositionClass::CommonPointWeightedSum:
        return "CPM-SUM";
    case CompositionClass::CommonPointPowerMean:
        return "CPM-PMEAN";
    case CompositionClass::CommonPointLevelWell:
        return "CPM-LWELL";
    case CompositionClass::DeceptivePointSoftmax:
        return "DPM-SOFTMAX";
    default:
        return "UNKNOWN";
    }
}

std::string to_string(ValueTransformClass cls) {
    switch (cls) {
    case ValueTransformClass::None:
        return "NONE";
    case ValueTransformClass::CosineZero:
        return "COSZERO";
    case ValueTransformClass::Oscillatory:
        return "OSC";
    case ValueTransformClass::Power:
        return "POWER";
    case ValueTransformClass::Mixed:
        return "MIXED";
    default:
        return "UNKNOWN";
    }
}

std::string to_string(CoordinateTransformClass cls) {
    switch (cls) {
    case CoordinateTransformClass::None:
        return "NONE";
    case CoordinateTransformClass::Rotation:
        return "ROT";
    case CoordinateTransformClass::Block:
        return "BLOCK";
    case CoordinateTransformClass::Affine:
        return "AFF";
    case CoordinateTransformClass::NonlinearFold:
        return "FOLD";
    case CoordinateTransformClass::Mixed:
        return "MIXED";
    default:
        return "UNKNOWN";
    }
}

std::string class_label(const FunctionClass& cls) {
    std::ostringstream out;
    out << "C=" << to_string(cls.composition)
        << ";P=" << to_string(cls.value_transform)
        << ";T=" << to_string(cls.coordinate_transform)
        << ";G=" << join_base_ids(cls.base_functions);
    return out.str();
}

} // namespace FuncCraft
