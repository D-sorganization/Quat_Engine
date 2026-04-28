// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_readiness.cpp
 * @brief Tests for the embeddable health/readiness status surface.
 */

#include "test_framework.h"

#include "../src/core/Readiness.h"

#include <string>

void test_alive_status_is_stable() {
    const auto status = qe::core::alive_status();

    ASSERT_TRUE(status.endpoint == "/alive");
    ASSERT_TRUE(status.component == "QuatEngine");
    ASSERT_TRUE(status.status == "alive");
    ASSERT_TRUE(status.ready);
}

void test_ready_status_defaults_to_ready() {
    const auto status = qe::core::ready_status();

    ASSERT_TRUE(status.endpoint == "/ready");
    ASSERT_TRUE(status.status == "ready");
    ASSERT_TRUE(status.ready);
}

void test_ready_status_reports_dependency_failure() {
    qe::core::ReadinessInputs inputs;
    inputs.assets_available = false;

    const auto status = qe::core::ready_status(inputs);

    ASSERT_TRUE(status.endpoint == "/ready");
    ASSERT_TRUE(status.status == "not_ready");
    ASSERT_TRUE(!status.ready);
}

void test_health_status_serializes_to_json() {
    qe::core::ReadinessInputs inputs;
    inputs.renderer_available = false;

    const std::string json = qe::core::to_json(qe::core::ready_status(inputs));

    ASSERT_TRUE(json.find("\"endpoint\":\"/ready\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"component\":\"QuatEngine\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"status\":\"not_ready\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"ready\":false") != std::string::npos);
}

int main() {
    RUN_TEST(test_alive_status_is_stable);
    RUN_TEST(test_ready_status_defaults_to_ready);
    RUN_TEST(test_ready_status_reports_dependency_failure);
    RUN_TEST(test_health_status_serializes_to_json);
    return TEST_REPORT();
}
