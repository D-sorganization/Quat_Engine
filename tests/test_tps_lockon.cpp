/**
 * @file test_tps_lockon.cpp
 * @brief Tests for LockOnSystem — targeting, wobble, switching, break conditions.
 *
 * Validates:
 *   - Lock-on acquires nearest valid target
 *   - Wobble produces non-zero aim deviation
 *   - Wobble respects stability and ADS reduction
 *   - Target switching works
 *   - Lock breaks on target death or out-of-range
 */

#include "test_framework.h"

#include "../src/game/tps/LockOnSystem.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>


using namespace qe::game::tps;
using namespace qe::math;

// ── Lock-On Acquisition Tests ────────────────────────────────────────────────

void test_lock_on_nearest_target() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -20), true, 0.0f},
        {2, Vec3(0, 0, -10), true, 0.0f},  // Closer
        {3, Vec3(0, 0, -30), true, 0.0f},
    };

    ASSERT_TRUE(lock.try_lock_on(player_pos, forward, targets));
    ASSERT_TRUE(lock.is_locked());
    ASSERT_TRUE(lock.locked_target_id() == 2);  // Nearest
}

void test_lock_on_respects_angle() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    // Target is behind the player
    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, 20), true, 0.0f},  // Behind
    };

    ASSERT_TRUE(!lock.try_lock_on(player_pos, forward, targets));
    ASSERT_TRUE(!lock.is_locked());
}

void test_lock_on_ignores_dead_targets() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), false, 0.0f},  // Dead
        {2, Vec3(0, 0, -15), true, 0.0f},   // Alive
    };

    ASSERT_TRUE(lock.try_lock_on(player_pos, forward, targets));
    ASSERT_TRUE(lock.locked_target_id() == 2);
}

void test_lock_on_out_of_range() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -100), true, 0.0f},  // Beyond max range
    };

    ASSERT_TRUE(!lock.try_lock_on(player_pos, forward, targets));
}

// ── Wobble Tests ─────────────────────────────────────────────────────────────

void test_wobble_produces_deviation() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };

    lock.try_lock_on(player_pos, forward, targets);

    // Run for a bit to generate wobble
    for (int i = 0; i < 60; ++i) lock.update(0.016f);

    Vec3 wobbled = lock.get_wobbled_aim(forward, 0.5f, false);
    // Wobbled direction should differ from perfect aim
    float dot = forward.dot(wobbled);
    ASSERT_TRUE(dot < 1.0f);  // Not perfectly aligned
    ASSERT_TRUE(dot > 0.9f);  // But close (wobble is subtle)
}

void test_ads_reduces_wobble() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };

    lock.try_lock_on(player_pos, forward, targets);
    for (int i = 0; i < 60; ++i) lock.update(0.016f);

    Vec3 hip_aim = lock.get_wobbled_aim(forward, 0.5f, false);
    Vec3 ads_aim = lock.get_wobbled_aim(forward, 0.5f, true);

    // ADS aim should be closer to perfect than hip aim
    float hip_dot = forward.dot(hip_aim);
    float ads_dot = forward.dot(ads_aim);
    ASSERT_TRUE(ads_dot >= hip_dot);
}

void test_stability_affects_wobble() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };

    lock.try_lock_on(player_pos, forward, targets);
    for (int i = 0; i < 60; ++i) lock.update(0.016f);

    // Low stability (Recon: 0.15) should wobble less than high stability (Heavy: 0.5)
    Vec3 stable_aim = lock.get_wobbled_aim(forward, 0.15f, false);
    Vec3 unstable_aim = lock.get_wobbled_aim(forward, 0.5f, false);

    float stable_dot = forward.dot(stable_aim);
    float unstable_dot = forward.dot(unstable_aim);
    ASSERT_TRUE(stable_dot >= unstable_dot);
}

// ── Lock Release & Break Tests ───────────────────────────────────────────────

void test_release_lock() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };

    lock.try_lock_on(player_pos, forward, targets);
    ASSERT_TRUE(lock.is_locked());
    lock.release_lock();
    ASSERT_TRUE(!lock.is_locked());
}

void test_lock_breaks_on_death() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };
    lock.try_lock_on(player_pos, Vec3(0, 0, -1), targets);
    ASSERT_TRUE(lock.should_break_lock(player_pos, false));
}

void test_lock_breaks_on_distance() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(0, 0, -10), true, 0.0f},
    };
    lock.try_lock_on(player_pos, Vec3(0, 0, -1), targets);

    // Move target very far away
    lock.update_target_position(Vec3(0, 0, -200));
    ASSERT_TRUE(lock.should_break_lock(player_pos, true));
}

// ── Target Switching Tests ───────────────────────────────────────────────────

void test_switch_target() {
    LockOnState lock;
    Vec3 player_pos(0, 0, 0);
    Vec3 forward(0, 0, -1);

    std::vector<LockOnTarget> targets = {
        {1, Vec3(-5, 0, -10), true, 0.0f},
        {2, Vec3(5, 0, -10), true, 0.0f},
    };

    lock.try_lock_on(player_pos, forward, targets);
    int first_id = lock.locked_target_id();

    // Switch to the right
    ASSERT_TRUE(lock.switch_target(player_pos, Vec3(1, 0, 0), targets));
    ASSERT_TRUE(lock.locked_target_id() != first_id);
}

void test_no_lock_without_targets() {
    LockOnState lock;
    std::vector<LockOnTarget> empty;
    ASSERT_TRUE(!lock.try_lock_on(Vec3::zero(), Vec3(0, 0, -1), empty));
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Lock-On System Tests ===" << std::endl;

    std::cout << "\n--- Acquisition ---" << std::endl;
    RUN_TEST(test_lock_on_nearest_target);
    RUN_TEST(test_lock_on_respects_angle);
    RUN_TEST(test_lock_on_ignores_dead_targets);
    RUN_TEST(test_lock_on_out_of_range);

    std::cout << "\n--- Wobble ---" << std::endl;
    RUN_TEST(test_wobble_produces_deviation);
    RUN_TEST(test_ads_reduces_wobble);
    RUN_TEST(test_stability_affects_wobble);

    std::cout << "\n--- Lock Break ---" << std::endl;
    RUN_TEST(test_release_lock);
    RUN_TEST(test_lock_breaks_on_death);
    RUN_TEST(test_lock_breaks_on_distance);

    std::cout << "\n--- Switching ---" << std::endl;
    RUN_TEST(test_switch_target);
    RUN_TEST(test_no_lock_without_targets);

    return TEST_REPORT();
}
