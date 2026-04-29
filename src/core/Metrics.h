// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Metrics.h
 * @brief Minimal in-process counters for QuatEngine observability.
 */

#ifndef QE_CORE_METRICS_H
#define QE_CORE_METRICS_H

#include <atomic>
#include <cstdint>
#include <string>

namespace qe::core {

struct MetricsSnapshot {
    std::uint64_t alive_checks;
    std::uint64_t ready_checks;
    std::uint64_t ready_failures;
};

class Metrics {
public:
    static void record_alive_check() {
        counters().alive_checks.fetch_add(1, std::memory_order_relaxed);
    }

    static void record_ready_check(bool ready) {
        counters().ready_checks.fetch_add(1, std::memory_order_relaxed);
        if (!ready) {
            counters().ready_failures.fetch_add(1, std::memory_order_relaxed);
        }
    }

    static MetricsSnapshot snapshot() {
        return {
            counters().alive_checks.load(std::memory_order_relaxed),
            counters().ready_checks.load(std::memory_order_relaxed),
            counters().ready_failures.load(std::memory_order_relaxed),
        };
    }

    static void reset_for_tests() {
        counters().alive_checks.store(0, std::memory_order_relaxed);
        counters().ready_checks.store(0, std::memory_order_relaxed);
        counters().ready_failures.store(0, std::memory_order_relaxed);
    }

private:
    struct Counters {
        std::atomic<std::uint64_t> alive_checks{0};
        std::atomic<std::uint64_t> ready_checks{0};
        std::atomic<std::uint64_t> ready_failures{0};
    };

    static Counters& counters() {
        static Counters instance;
        return instance;
    }
};

inline std::string to_prometheus(const MetricsSnapshot& snapshot) {
    std::string output;
    output += "# TYPE quatengine_alive_checks_total counter\n";
    output += "quatengine_alive_checks_total ";
    output += std::to_string(snapshot.alive_checks);
    output += "\n# TYPE quatengine_ready_checks_total counter\n";
    output += "quatengine_ready_checks_total ";
    output += std::to_string(snapshot.ready_checks);
    output += "\n# TYPE quatengine_ready_failures_total counter\n";
    output += "quatengine_ready_failures_total ";
    output += std::to_string(snapshot.ready_failures);
    output += "\n";
    return output;
}

}  // namespace qe::core

#endif  // QE_CORE_METRICS_H
