/**
 * @file test_runtime_session.cpp
 * @brief Tests for extracted demo runtime/session helpers.
 */

#include "test_framework.h"

#include "../src/demo/RuntimeSession.h"

#include <vector>

void test_count_alive_counts_only_active_entities() {
    std::vector<qe::core::Entity> entities(4);
    entities[0].alive = true;
    entities[1].alive = false;
    entities[2].alive = true;
    entities[3].alive = false;

    ASSERT_TRUE(qe::demo::count_alive(entities) == 2);
}

void test_weapon_name_for_index_returns_expected_names() {
    ASSERT_TRUE(qe::demo::weapon_name_for_index(0) == "Pistol");
    ASSERT_TRUE(qe::demo::weapon_name_for_index(2) == "RailGun");
    ASSERT_TRUE(qe::demo::weapon_name_for_index(4) == "MiniGun");
    ASSERT_TRUE(qe::demo::weapon_name_for_index(99) == "?");
}

int main() {
    RUN_TEST(test_count_alive_counts_only_active_entities);
    RUN_TEST(test_weapon_name_for_index_returns_expected_names);
    return TEST_REPORT();
}
