// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_game_extended.cpp
 * @brief Extended coverage tests for AABB, Scoring, Behavior, and PowerUp.
 *
 * Targets branches and methods NOT yet covered by existing test files:
 *   - AABB: size(), half_extents(), from_center with Vec3 half, parallel-ray edge case,
 *           touching boundaries, negative-scale transform
 *   - Scoring: record_hit, record_miss, add_bonus, update (timer), has_recent_event,
 *              high_score tracking, register_kill total_kills, combo_timer reset
 *   - TargetBehavior: dodge rotation with non-zero dodge_offset, patrol SLERP produces
 *                     unit quaternion, figure-8 rotation unit quaternion, static rotation,
 *                     spiral rotation
 *   - PowerUp: flash_rate(), try_spawn_random, clear(), update does NOT kill before lifetime,
 *              dead powerup update is no-op
 */

#include "test_framework.h"

#include "../src/core/AABB.h"
#include "../src/game/PowerUp.h"
#include "../src/game/Scoring.h"
#include "../src/game/TargetBehavior.h"

#include <cmath>
#include <iostream>


// ============================================================================
//  AABB Extended Tests
// ============================================================================

void test_aabb_size_and_half_extents() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 2.0f);
    qe::math::Vec3 sz = box.size();
    ASSERT_NEAR(sz.x, 4.0f, 1e-5f);
    ASSERT_NEAR(sz.y, 4.0f, 1e-5f);
    ASSERT_NEAR(sz.z, 4.0f, 1e-5f);

    qe::math::Vec3 he = box.half_extents();
    ASSERT_NEAR(he.x, 2.0f, 1e-5f);
    ASSERT_NEAR(he.y, 2.0f, 1e-5f);
    ASSERT_NEAR(he.z, 2.0f, 1e-5f);
}

void test_aabb_from_center_vec3_half() {
    qe::math::Vec3 center(5.0f, 3.0f, 1.0f);
    qe::math::Vec3 half(1.0f, 2.0f, 0.5f);
    auto box = qe::core::AABB::from_center(center, half);

    ASSERT_NEAR(box.min.x, 4.0f, 1e-5f);
    ASSERT_NEAR(box.max.x, 6.0f, 1e-5f);
    ASSERT_NEAR(box.min.y, 1.0f, 1e-5f);
    ASSERT_NEAR(box.max.y, 5.0f, 1e-5f);
    ASSERT_NEAR(box.min.z, 0.5f, 1e-5f);
    ASSERT_NEAR(box.max.z, 1.5f, 1e-5f);
}

void test_aabb_center_query() {
    auto box = qe::core::AABB::from_center({3.0f, 5.0f, 7.0f}, 1.0f);
    qe::math::Vec3 c = box.center();
    ASSERT_NEAR(c.x, 3.0f, 1e-5f);
    ASSERT_NEAR(c.y, 5.0f, 1e-5f);
    ASSERT_NEAR(c.z, 7.0f, 1e-5f);
}

void test_aabb_boundary_contains() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    // Points exactly on the boundary should be contained
    ASSERT_TRUE(box.contains({1.0f, 0.0f, 0.0f}));
    ASSERT_TRUE(box.contains({-1.0f, 0.0f, 0.0f}));
    ASSERT_TRUE(box.contains({0.0f, 1.0f, 0.0f}));
    ASSERT_TRUE(box.contains({0.0f, -1.0f, 0.0f}));
}

void test_aabb_touching_just_outside() {
    auto a = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    auto b = qe::core::AABB::from_center({2.001f, 0, 0}, 1.0f);
    // b starts at x=1.001, which does not overlap with a.max.x=1
    ASSERT_TRUE(!a.intersects(b));
}

void test_aabb_touching_exactly() {
    // Two boxes sharing a face
    auto a = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    auto b = qe::core::AABB::from_center({2.0f, 0, 0}, 1.0f);
    // b.min.x = 1.0f = a.max.x → just touching, <=/>= so they intersect
    ASSERT_TRUE(a.intersects(b));
}

void test_aabb_ray_parallel_inside_slab() {
    // Ray parallel to X-axis but passing through the box in Y
    auto box = qe::core::AABB{{-1, -1, -1}, {1, 1, 1}};
    float t = 0;
    // Ray origin inside the box in Y range, parallel in Y direction
    bool hit = box.ray_intersect({0, 0, 0}, {0, 0, -1}, t);
    ASSERT_TRUE(hit);  // Origin inside, shooting into box
}

void test_aabb_ray_parallel_outside_slab() {
    // Ray parallel to Y-axis but outside the Y range of the box
    auto box = qe::core::AABB{{0, 2, 0}, {1, 3, 1}};
    float t = 0;
    // Ray origin at y=0 (outside box's Y range [2,3]), pointing in Y dir
    // For the X slab: origin.x=0.5 is inside [0,1], ok
    // For the Z slab: origin.z=0.5 is inside [0,1], ok
    // But for the Y slab: origin.y=0 < 2 (box.min.y), and dir.y=0 → outside slab
    bool hit = box.ray_intersect({0.5f, 0.0f, 0.5f}, {1, 0, 0}, t);
    // The Y slab has parallel ray outside → should NOT hit
    ASSERT_TRUE(!hit);
}

void test_aabb_transform_negative_scale() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    // Apply a negative scale: result should still have min < max
    auto transformed = box.transformed({0, 0, 0}, {-2.0f, 1.0f, 1.0f});
    ASSERT_TRUE(transformed.min.x <= transformed.max.x);
    ASSERT_TRUE(transformed.min.y <= transformed.max.y);
    ASSERT_TRUE(transformed.min.z <= transformed.max.z);
    ASSERT_NEAR(transformed.half_extents().x, 2.0f, 1e-5f);
}

// ============================================================================
//  Scoring Extended Tests
// ============================================================================

void test_scoring_record_hit_and_miss() {
    qe::game::ScoreTracker tracker;
    tracker.record_hit();
    ASSERT_TRUE(tracker.combo().streak == 1);

    tracker.record_hit();
    ASSERT_TRUE(tracker.combo().streak == 2);

    tracker.record_miss();
    ASSERT_TRUE(tracker.combo().streak == 0);
}

void test_scoring_add_bonus() {
    qe::game::ScoreTracker tracker;
    tracker.add_bonus(500);
    ASSERT_TRUE(tracker.score() == 500);
    ASSERT_TRUE(tracker.high_score() == 500);
}

void test_scoring_high_score_tracks_maximum() {
    qe::game::ScoreTracker tracker;
    tracker.record_kill(100);
    int score_after_1 = tracker.score();
    ASSERT_TRUE(tracker.high_score() == score_after_1);

    tracker.reset();
    // High score should be preserved across reset
    ASSERT_TRUE(tracker.score() == 0);
    ASSERT_TRUE(tracker.high_score() == score_after_1);
}

void test_scoring_update_clears_combo_on_timeout() {
    qe::game::ScoreTracker tracker;
    tracker.record_hit();
    tracker.record_hit();
    ASSERT_TRUE(tracker.combo().streak == 2);

    // Update past combo_window (2.0s)
    tracker.update(2.1f);
    ASSERT_TRUE(tracker.combo().streak == 0);
}

void test_scoring_has_recent_event() {
    qe::game::ScoreTracker tracker;
    ASSERT_TRUE(!tracker.has_recent_event());

    tracker.record_kill(100);
    ASSERT_TRUE(tracker.has_recent_event());

    // Advance past display timer (2s)
    tracker.update(2.1f);
    ASSERT_TRUE(!tracker.has_recent_event());
}

void test_combo_register_kill_increments_total_kills() {
    qe::game::ComboState combo;
    ASSERT_TRUE(combo.total_kills == 0);
    combo.register_kill();
    ASSERT_TRUE(combo.total_kills == 1);
    combo.register_kill();
    ASSERT_TRUE(combo.total_kills == 2);
}

void test_combo_reset_clears_total_kills() {
    qe::game::ComboState combo;
    combo.register_kill();
    combo.register_kill();
    ASSERT_TRUE(combo.total_kills == 2);
    combo.reset();
    ASSERT_TRUE(combo.total_kills == 0);
    ASSERT_TRUE(combo.total_headshots == 0);
}

void test_scoring_multiplier_at_boundaries() {
    qe::game::ComboState combo;
    // streak < 2: 1.0
    ASSERT_NEAR(combo.multiplier(), 1.0f, 1e-5f);
    combo.register_hit();  // streak=1
    ASSERT_NEAR(combo.multiplier(), 1.0f, 1e-5f);

    // streak in [2,4]: 1.5
    combo.register_hit();  // streak=2
    ASSERT_NEAR(combo.multiplier(), 1.5f, 1e-5f);
    combo.register_hit();  // streak=3
    ASSERT_NEAR(combo.multiplier(), 1.5f, 1e-5f);

    // streak in [5,9]: 2.0
    combo.register_hit();  // streak=4
    combo.register_hit();  // streak=5
    ASSERT_NEAR(combo.multiplier(), 2.0f, 1e-5f);
}

// ============================================================================
//  TargetBehavior Extended Tests
// ============================================================================

void test_behavior_dodge_rotation_with_offset() {
    auto b = qe::game::TargetBehavior::create_dodge(
        qe::math::Vec3(0.0f, 0.0f, 0.0f), 1.0f);
    b.alert();

    // After alert, dodge_offset should be non-zero
    // Compute rotation: should produce a unit quaternion (not identity with offset)
    qe::math::Quaternion rot = b.compute_rotation(0.0f);
    ASSERT_NEAR(rot.norm(), 1.0f, 0.01f);
}

void test_behavior_patrol_rotation_is_slerped_unit_quaternion() {
    qe::math::Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = qe::game::TargetBehavior::create_patrol(center, 8.0f, 1.0f, 0.0f);

    // Sample the patrol rotation at many time points
    for (float t = 0.0f; t < 15.0f; t += 0.4f) {
        qe::math::Quaternion q = b.compute_rotation(t);
        ASSERT_NEAR(q.norm(), 1.0f, 0.002f);
    }
}

void test_behavior_figure8_rotation_unit_quaternion() {
    qe::math::Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = qe::game::TargetBehavior::create_figure8(center, 4.0f, 1.0f, 0.0f);

    for (float t = 0.0f; t < 10.0f; t += 0.5f) {
        qe::math::Quaternion q = b.compute_rotation(t);
        ASSERT_NEAR(q.norm(), 1.0f, 0.002f);
    }
}

void test_behavior_static_rotation_is_base_orientation() {
    qe::game::TargetBehavior b;
    b.type = qe::game::BehaviorType::Static;
    b.center = qe::math::Vec3(0.0f, 0.0f, 0.0f);
    b.base_orientation = qe::math::Quaternion::from_axis_angle(
        qe::math::Vec3::up(), 0.5f);

    qe::math::Quaternion rot = b.compute_rotation(5.0f);
    ASSERT_TRUE(rot.approx_equal(b.base_orientation, 1e-4f));
}

void test_behavior_spiral_rotation_unit_quaternion() {
    qe::math::Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = qe::game::TargetBehavior::create_spiral(center, 5.0f, 1.0f, 0.0f);

    for (float t = 0.0f; t < 10.0f; t += 0.7f) {
        qe::math::Quaternion q = b.compute_rotation(t);
        ASSERT_NEAR(q.norm(), 1.0f, 0.002f);
    }
}

void test_behavior_update_resets_dodge_offset_after_expiry() {
    auto b = qe::game::TargetBehavior::create_dodge(
        qe::math::Vec3(10.0f, 0.0f, 0.0f), 1.0f);

    b.alert();
    ASSERT_TRUE(b.is_alerted);

    // Update to expire alert
    b.update(1.5f);
    ASSERT_TRUE(!b.is_alerted);

    // dodge_offset should be reset to zero
    ASSERT_NEAR(b.dodge_offset.length(), 0.0f, 1e-5f);
}

void test_behavior_orbit_center_offset() {
    // Test that orbit respects non-origin center
    qe::math::Vec3 center(10.0f, 5.0f, 3.0f);
    auto b = qe::game::TargetBehavior::create_orbit(center, 4.0f, 1.0f, 0.0f);

    for (float t = 0.0f; t < 10.0f; t += 1.0f) {
        qe::math::Vec3 pos = b.compute_position(t);
        float dist = (pos - center).length();
        ASSERT_NEAR(dist, 4.0f, 0.01f);
    }
}

// ============================================================================
//  PowerUp Extended Tests
// ============================================================================

void test_powerup_flash_rate_normal() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::ScoreMultiplier;
    p.lifetime = 15.0f;
    p.age = 0.0f;

    // When more than 3s remain, no flash
    float fr = p.flash_rate();
    ASSERT_NEAR(fr, 0.0f, 1e-5f);
}

void test_powerup_flash_rate_near_despawn() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::RapidFire;
    p.lifetime = 15.0f;
    p.age = 12.5f;  // Only 2.5s remaining < 3s threshold

    float fr = p.flash_rate();
    ASSERT_NEAR(fr, 8.0f, 1e-5f);  // Fast flash
}

void test_powerup_update_noop_when_not_alive() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::RapidFire;
    p.alive = false;
    p.age = 0.0f;

    p.update(10.0f);
    // Age should NOT advance when not alive
    ASSERT_NEAR(p.age, 0.0f, 1e-5f);
}

void test_powerup_update_does_not_kill_before_lifetime() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::DamageBoost;
    p.lifetime = 15.0f;

    p.update(7.0f);
    ASSERT_TRUE(p.alive);
    ASSERT_NEAR(p.age, 7.0f, 1e-5f);
}

void test_powerup_manager_clear() {
    qe::game::PowerUpManager mgr;
    mgr.spawn(qe::game::PowerUpType::Shield, qe::math::Vec3(0, 0, 0));
    mgr.spawn(qe::game::PowerUpType::RapidFire, qe::math::Vec3(1, 0, 0));
    ASSERT_TRUE(mgr.pickups().size() == 2);

    // Collect one to create an effect
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_TRUE(!mgr.effects().empty());

    mgr.clear();
    ASSERT_TRUE(mgr.pickups().empty());
    ASSERT_TRUE(mgr.effects().empty());
}

void test_powerup_manager_try_spawn_random_deterministic() {
    // With fixed RNG seed (54321), try_spawn_random should spawn something
    // or not; just verify it doesn't crash and list stays bounded
    qe::game::PowerUpManager mgr;
    for (int i = 0; i < 20; ++i) {
        mgr.try_spawn_random(qe::math::Vec3(static_cast<float>(i), 0, 0));
    }
    // Number of spawns may vary (probabilistic) but should be >= 0
    ASSERT_TRUE(mgr.pickups().size() <= 20);
}

void test_powerup_can_collect_after_death() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::TripleShot;
    p.position = qe::math::Vec3(0, 0, 0);
    p.radius = 5.0f;   // Large collection radius
    p.alive = false;   // Already dead

    // Should NOT be collectible when dead
    ASSERT_TRUE(!p.can_collect(qe::math::Vec3(0, 0, 0)));
}

void test_active_effect_progress_bounds() {
    qe::game::ActiveEffect e;
    e.type = qe::game::PowerUpType::SlowMotion;
    e.duration = 6.0f;
    e.elapsed = 0.0f;
    e.value = 0.5f;

    ASSERT_NEAR(e.progress(), 0.0f, 1e-5f);

    e.update(3.0f);
    ASSERT_NEAR(e.progress(), 0.5f, 1e-5f);

    e.update(4.0f);  // Past duration
    ASSERT_NEAR(e.remaining(), 0.0f, 1e-5f);
    ASSERT_NEAR(e.progress(), 7.0f / 6.0f, 0.1f);  // > 1.0 since elapsed > duration
    ASSERT_TRUE(!e.is_active());
}

void test_active_effect_progress_zero_duration() {
    // Duration = 0 should not divide by zero
    qe::game::ActiveEffect e;
    e.type = qe::game::PowerUpType::RapidFire;
    e.duration = 0.0f;
    e.elapsed = 0.0f;
    e.value = 0.5f;

    float prog = e.progress();
    ASSERT_NEAR(prog, 1.0f, 1e-5f);  // duration=0 → returns 1.0
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "=== QuatEngine Game Extended Tests ===" << std::endl;

    std::cout << "\n--- AABB Extended ---" << std::endl;
    RUN_TEST(test_aabb_size_and_half_extents);
    RUN_TEST(test_aabb_from_center_vec3_half);
    RUN_TEST(test_aabb_center_query);
    RUN_TEST(test_aabb_boundary_contains);
    RUN_TEST(test_aabb_touching_just_outside);
    RUN_TEST(test_aabb_touching_exactly);
    RUN_TEST(test_aabb_ray_parallel_inside_slab);
    RUN_TEST(test_aabb_ray_parallel_outside_slab);
    RUN_TEST(test_aabb_transform_negative_scale);

    std::cout << "\n--- Scoring Extended ---" << std::endl;
    RUN_TEST(test_scoring_record_hit_and_miss);
    RUN_TEST(test_scoring_add_bonus);
    RUN_TEST(test_scoring_high_score_tracks_maximum);
    RUN_TEST(test_scoring_update_clears_combo_on_timeout);
    RUN_TEST(test_scoring_has_recent_event);
    RUN_TEST(test_combo_register_kill_increments_total_kills);
    RUN_TEST(test_combo_reset_clears_total_kills);
    RUN_TEST(test_scoring_multiplier_at_boundaries);

    std::cout << "\n--- TargetBehavior Extended ---" << std::endl;
    RUN_TEST(test_behavior_dodge_rotation_with_offset);
    RUN_TEST(test_behavior_patrol_rotation_is_slerped_unit_quaternion);
    RUN_TEST(test_behavior_figure8_rotation_unit_quaternion);
    RUN_TEST(test_behavior_static_rotation_is_base_orientation);
    RUN_TEST(test_behavior_spiral_rotation_unit_quaternion);
    RUN_TEST(test_behavior_update_resets_dodge_offset_after_expiry);
    RUN_TEST(test_behavior_orbit_center_offset);

    std::cout << "\n--- PowerUp Extended ---" << std::endl;
    RUN_TEST(test_powerup_flash_rate_normal);
    RUN_TEST(test_powerup_flash_rate_near_despawn);
    RUN_TEST(test_powerup_update_noop_when_not_alive);
    RUN_TEST(test_powerup_update_does_not_kill_before_lifetime);
    RUN_TEST(test_powerup_manager_clear);
    RUN_TEST(test_powerup_manager_try_spawn_random_deterministic);
    RUN_TEST(test_powerup_can_collect_after_death);
    RUN_TEST(test_active_effect_progress_bounds);
    RUN_TEST(test_active_effect_progress_zero_duration);

    return TEST_REPORT();
}
