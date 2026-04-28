// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Readiness.h
 * @brief Minimal health/readiness status surface for QuatEngine hosts.
 */

#ifndef QE_CORE_READINESS_H
#define QE_CORE_READINESS_H

#include "Metrics.h"

#include <string>
#include <string_view>

namespace qe::core {

struct ReadinessInputs {
    bool configuration_loaded = true;
    bool assets_available = true;
    bool renderer_available = true;
};

struct HealthStatus {
    std::string_view endpoint;
    std::string_view component;
    std::string_view status;
    bool ready;
};

inline HealthStatus alive_status() {
    Metrics::record_alive_check();
    return {"/alive", "QuatEngine", "alive", true};
}

inline HealthStatus ready_status(const ReadinessInputs& inputs = {}) {
    const bool ready = inputs.configuration_loaded && inputs.assets_available &&
        inputs.renderer_available;
    Metrics::record_ready_check(ready);
    return {"/ready", "QuatEngine", ready ? "ready" : "not_ready", ready};
}

inline std::string to_json(const HealthStatus& status) {
    std::string json = "{\"endpoint\":\"";
    json += status.endpoint;
    json += "\",\"component\":\"";
    json += status.component;
    json += "\",\"status\":\"";
    json += status.status;
    json += "\",\"ready\":";
    json += (status.ready ? "true" : "false");
    json += "}";
    return json;
}

}  // namespace qe::core

#endif  // QE_CORE_READINESS_H
