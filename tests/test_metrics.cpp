// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_metrics.cpp
 * @brief Tests for core observability counters.
 */

#include "test_framework.h"

#include "../src/core/Metrics.h"
#include "../src/core/Readiness.h"

#include <string>

void test_readiness_updates_metrics_counters() {
    qe::core::Metrics::reset_for_tests();

    (void)qe::core::alive_status();
    (void)qe::core::ready_status();

    qe::core::ReadinessInputs inputs;
    inputs.assets_available = false;
    (void)qe::core::ready_status(inputs);

    const auto snapshot = qe::core::Metrics::snapshot();
    ASSERT_TRUE(snapshot.alive_checks == 1);
    ASSERT_TRUE(snapshot.ready_checks == 2);
    ASSERT_TRUE(snapshot.ready_failures == 1);
}

void test_metrics_snapshot_serializes_to_prometheus_text() {
    qe::core::Metrics::reset_for_tests();

    (void)qe::core::alive_status();
    (void)qe::core::ready_status();

    const std::string metrics =
        qe::core::to_prometheus(qe::core::Metrics::snapshot());

    ASSERT_TRUE(
        metrics.find("quatengine_alive_checks_total 1") != std::string::npos);
    ASSERT_TRUE(
        metrics.find("quatengine_ready_checks_total 1") != std::string::npos);
    ASSERT_TRUE(
        metrics.find("quatengine_ready_failures_total 0") != std::string::npos);
}

int main() {
    RUN_TEST(test_readiness_updates_metrics_counters);
    RUN_TEST(test_metrics_snapshot_serializes_to_prometheus_text);
    return TEST_REPORT();
}
