#ifndef FUNCCRAFT_RUNTIME_PROFILE_H
#define FUNCCRAFT_RUNTIME_PROFILE_H

#ifdef FUNCCRAFT_ENABLE_PROFILE

#include "basicf.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace FuncCraft::runtime_profile {

enum class Counter : std::size_t {
    BenchmarkTotal,
    RawFunction,
    GlobalScale,
    ComponentTotal,
    CoordinateTransform,
    DomainMap,
    PrimitiveEvaluate,
    NestedEvaluate,
    FinalizeValue,
    Composition,
    BasicEvaluate,
    Count
};

inline const char* name(Counter counter) {
    switch (counter) {
    case Counter::BenchmarkTotal: return "benchmark_total";
    case Counter::RawFunction: return "raw_function";
    case Counter::GlobalScale: return "global_scale";
    case Counter::ComponentTotal: return "component_total";
    case Counter::CoordinateTransform: return "coordinate_transform";
    case Counter::DomainMap: return "domain_map";
    case Counter::PrimitiveEvaluate: return "primitive_evaluate";
    case Counter::NestedEvaluate: return "nested_evaluate";
    case Counter::FinalizeValue: return "finalize_value";
    case Counter::Composition: return "composition";
    case Counter::BasicEvaluate: return "basic_evaluate";
    case Counter::Count: return "count";
    }
    return "unknown";
}

inline bool enabled() {
    static const bool value = std::getenv("FUNCCRAFT_PROFILE") != nullptr;
    return value;
}

inline std::array<std::atomic<long long>, static_cast<std::size_t>(Counter::Count)> ns = {};
inline std::array<std::atomic<long long>, static_cast<std::size_t>(Counter::Count)> calls = {};
inline std::array<std::atomic<long long>, 35> basic_ns = {};
inline std::array<std::atomic<long long>, 35> basic_calls = {};

inline void add(Counter counter, long long elapsed_ns) {
    const auto idx = static_cast<std::size_t>(counter);
    ns[idx].fetch_add(elapsed_ns, std::memory_order_relaxed);
    calls[idx].fetch_add(1, std::memory_order_relaxed);
}

inline void add_basic(BasicFunctionId id, long long elapsed_ns) {
    const auto idx = static_cast<std::size_t>(id);
    if (idx < basic_ns.size()) {
        basic_ns[idx].fetch_add(elapsed_ns, std::memory_order_relaxed);
        basic_calls[idx].fetch_add(1, std::memory_order_relaxed);
    }
}

class Scope {
public:
    explicit Scope(Counter counter)
        : counter_(counter),
          active_(enabled()),
          start_(active_ ? clock::now() : clock::time_point{}) {}

    ~Scope() {
        if (!active_) {
            return;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_).count();
        add(counter_, elapsed);
    }

private:
    using clock = std::chrono::steady_clock;
    Counter counter_;
    bool active_;
    clock::time_point start_;
};

class BasicScope {
public:
    explicit BasicScope(BasicFunctionId id)
        : id_(id),
          active_(enabled()),
          start_(active_ ? clock::now() : clock::time_point{}) {}

    ~BasicScope() {
        if (!active_) {
            return;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_).count();
        add(Counter::BasicEvaluate, elapsed);
        add_basic(id_, elapsed);
    }

private:
    using clock = std::chrono::steady_clock;
    BasicFunctionId id_;
    bool active_;
    clock::time_point start_;
};

inline void report() {
    if (!enabled()) {
        return;
    }
    std::cerr << "\n[funccraft-profile]\n";
    for (std::size_t i = 0; i < static_cast<std::size_t>(Counter::Count); ++i) {
        const auto call_count = calls[i].load(std::memory_order_relaxed);
        const auto total_ns = ns[i].load(std::memory_order_relaxed);
        if (call_count == 0) {
            continue;
        }
        std::cerr << name(static_cast<Counter>(i))
                  << " calls=" << call_count
                  << " seconds=" << (static_cast<double>(total_ns) * 1.0e-9)
                  << " avg_ns=" << (total_ns / call_count)
                  << "\n";
    }
    std::cerr << "[funccraft-profile-basic]\n";
    for (std::size_t i = 1; i < basic_ns.size(); ++i) {
        const auto call_count = basic_calls[i].load(std::memory_order_relaxed);
        const auto total_ns = basic_ns[i].load(std::memory_order_relaxed);
        if (call_count == 0) {
            continue;
        }
        std::cerr << static_cast<int>(i)
                  << " calls=" << call_count
                  << " seconds=" << (static_cast<double>(total_ns) * 1.0e-9)
                  << " avg_ns=" << (total_ns / call_count)
                  << "\n";
    }
}

struct Reporter {
    Reporter() {
        std::atexit(report);
    }
};

inline Reporter reporter;

} // namespace FuncCraft::runtime_profile

#define FUNCCRAFT_PROFILE_JOIN_IMPL(a, b) a##b
#define FUNCCRAFT_PROFILE_JOIN(a, b) FUNCCRAFT_PROFILE_JOIN_IMPL(a, b)
#define FUNCCRAFT_PROFILE_SCOPE(counter_name) \
    ::FuncCraft::runtime_profile::Scope FUNCCRAFT_PROFILE_JOIN(funccraft_profile_scope_, __COUNTER__)( \
        ::FuncCraft::runtime_profile::Counter::counter_name)
#define FUNCCRAFT_PROFILE_BASIC_SCOPE(function_id) \
    ::FuncCraft::runtime_profile::BasicScope FUNCCRAFT_PROFILE_JOIN(funccraft_profile_basic_scope_, __COUNTER__)(function_id)

#else

#define FUNCCRAFT_PROFILE_SCOPE(counter_name) ((void)0)
#define FUNCCRAFT_PROFILE_BASIC_SCOPE(function_id) ((void)0)

#endif

#endif // FUNCCRAFT_RUNTIME_PROFILE_H
