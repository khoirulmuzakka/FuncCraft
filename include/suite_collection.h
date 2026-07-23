#ifndef FUNCCRAFT_SUITE_COLLECTION_H
#define FUNCCRAFT_SUITE_COLLECTION_H

/**
 * @file suite_collection.h
 * @brief Public registry for named FuncCraft benchmark-suite collections.
 */

#include "suite.h"
#include "suite_spec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace FuncCraft {

/**
 * @brief Lightweight identifier for a built-in suite collection.
 */
struct SuiteCollectionId {
    int year = 0;
    int version = 0;
    std::string name;
};

/**
 * @brief Accessor for a built-in, versioned benchmark-suite collection.
 *
 * Use this when you want a stable public benchmark family such as
 * FuncCraft-2026-v1 without loading a YAML file path manually.
 */
class SuiteCollection final {
public:
    SuiteCollection(int year, int version);

    int year() const;
    int version() const;
    const std::string& name() const;
    int number_of_functions() const;

    SuiteSpec spec() const;
    BenchmarkSuite benchmark_suite(int dimension) const;

private:
    int year_ = 0;
    int version_ = 0;
    std::string name_;
};

std::vector<SuiteCollectionId> list_suite_collections();
SuiteCollection suite_collection(int year, int version);
SuiteSpec suite_collection_spec(int year, int version);
int suite_collection_number_of_functions(int year, int version);
BenchmarkSuite suite_collection_benchmark_suite(
    int year,
    int version,
    int dimension);

} // namespace FuncCraft

#endif // FUNCCRAFT_SUITE_COLLECTION_H
