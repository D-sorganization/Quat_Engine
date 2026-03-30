/**
 * @file test_powerup.cpp
 * @brief Tests for PowerUp system: quaternion animation, collection,
 *        active effects, manager operations, and config validation.
 */

#include "test_framework.h"

#include "../src/game/PowerUp.h"

#include <cmath>
#include <iostream>
#include <string>


// ── PowerUp Lifecycle ───────────────────────────────────────────────────────

void test_powerup_starts_alive_dies_after_lifetime() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::RapidFire;
    p.position = qe::math::Vec3(0, 1, 0);
    p.lifetime = 15.0f;

    ASSERT_TRUE(p.alive);
    ASSERT_TRUE(!p.collected);
    ASSERT_NEAR(p.age, 0.0f, 1e-5f);

    // Update partway through lifetime
    p.update(10.0f);
    ASSERT_TRUE(p.alive);

    // Update past lifetime
    p.update(6.0f);
    ASSERT_TRUE(!p.alive);
}

// ── Quaternion Animation ────────────────────────────────────────────────────

void test_powerup_rotation_changes_over_time() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::DamageBoost;
    p.position = qe::math::Vec3(0, 1, 0);

    qe::math::Quaternion initial = p.rotation;
    ASSERT_TRUE(initial.approx_equal(qe::math::Quaternion::identity()));

    // After some updates, rotation should change
    p.update(1.0f);
    qe::math::Quaternion after_1s = p.rotation;
    ASSERT_TRUE(!after_1s.approx_equal(initial, 0.01f));

    // Further update should produce another different rotation
    p.update(0.5f);
    qe::math::Quaternion after_1_5s = p.rotation;
    ASSERT_TRUE(!after_1_5s.approx_equal(after_1s, 0.01f));

    // Rotation should remain unit quaternion
    ASSERT_NEAR(after_1_5s.norm(), 1.0f, 1e-4f);
}

// ── Bob Animation ───────────────────────────────────────────────────────────

void test_powerup_display_position_includes_bob() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::Shield;
    p.position = qe::math::Vec3(5, 2, 3);
    p.bob_amplitude = 0.3f;
    p.bob_speed = 3.0f;

    // At age=0, bob_phase=0, sin(0)=0, so display_position == position
    qe::math::Vec3 dp0 = p.display_position();
    ASSERT_NEAR(dp0.x, 5.0f, 1e-5f);
    ASSERT_NEAR(dp0.y, 2.0f, 1e-5f);
    ASSERT_NEAR(dp0.z, 3.0f, 1e-5f);

    // After update, bob_phase changes and y should differ
    p.update(0.5f);
    qe::math::Vec3 dp1 = p.display_position();
    ASSERT_NEAR(dp1.x, 5.0f, 1e-5f);
    ASSERT_NEAR(dp1.z, 3.0f, 1e-5f);
    // Y should include sin(0.5 * 3.0) * 0.3 offset
    float expected_offset = std::sin(0.5f * 3.0f) * 0.3f;
    ASSERT_NEAR(dp1.y, 2.0f + expected_offset, 1e-4f);
}

// ── Collection Radius ───────────────────────────────────────────────────────

void test_powerup_can_collect_within_radius() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::TripleShot;
    p.position = qe::math::Vec3(10, 0, 0);
    p.radius = 0.8f;

    // Player very close
    qe::math::Vec3 close(10.2f, 0, 0);
    ASSERT_TRUE(p.can_collect(close));
}

void test_powerup_can_collect_fails_when_too_far() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::TripleShot;
    p.position = qe::math::Vec3(10, 0, 0);
    p.radius = 0.8f;

    // Player far away
    qe::math::Vec3 far_away(15, 0, 0);
    ASSERT_TRUE(!p.can_collect(far_away));
}

// ── Collect ─────────────────────────────────────────────────────────────────

void test_powerup_collect_marks_not_alive() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::SlowMotion;
    p.position = qe::math::Vec3(0, 0, 0);

    ASSERT_TRUE(p.alive);
    ASSERT_TRUE(!p.collected);

    p.collect();
    ASSERT_TRUE(!p.alive);
    ASSERT_TRUE(p.collected);

    // Can no longer collect
    qe::math::Vec3 right_here(0, 0, 0);
    ASSERT_TRUE(!p.can_collect(right_here));
}

// ── ActiveEffect ────────────────────────────────────────────────────────────

void test_active_effect_tracks_duration() {
    qe::game::ActiveEffect e;
    e.type = qe::game::PowerUpType::DamageBoost;
    e.duration = 8.0f;
    e.elapsed = 0.0f;
    e.value = 2.0f;

    ASSERT_TRUE(e.is_active());
    ASSERT_NEAR(e.remaining(), 8.0f, 1e-5f);
    ASSERT_NEAR(e.progress(), 0.0f, 1e-5f);

    e.update(4.0f);
    ASSERT_TRUE(e.is_active());
    ASSERT_NEAR(e.remaining(), 4.0f, 1e-5f);
    ASSERT_NEAR(e.progress(), 0.5f, 1e-5f);

    e.update(5.0f);
    ASSERT_TRUE(!e.is_active());
    ASSERT_NEAR(e.remaining(), 0.0f, 1e-5f);
}

void test_active_effect_is_active_and_remaining() {
    qe::game::ActiveEffect e;
    e.type = qe::game::PowerUpType::RapidFire;
    e.duration = 10.0f;
    e.elapsed = 0.0f;
    e.value = 0.5f;

    ASSERT_TRUE(e.is_active());
    ASSERT_NEAR(e.remaining(), 10.0f, 1e-5f);

    e.update(9.9f);
    ASSERT_TRUE(e.is_active());
    ASSERT_NEAR(e.remaining(), 0.1f, 1e-3f);

    e.update(0.2f);
    ASSERT_TRUE(!e.is_active());
    ASSERT_NEAR(e.remaining(), 0.0f, 1e-5f);
}

// ── PowerUpManager Spawn ────────────────────────────────────────────────────

void test_manager_spawn_creates_pickup() {
    qe::game::PowerUpManager mgr;
    ASSERT_TRUE(mgr.pickups().empty());

    mgr.spawn(qe::game::PowerUpType::Shield, qe::math::Vec3(1, 2, 3));
    ASSERT_TRUE(mgr.pickups().size() == 1);
    ASSERT_TRUE(mgr.pickups()[0].type == qe::game::PowerUpType::Shield);
    ASSERT_NEAR(mgr.pickups()[0].position.x, 1.0f, 1e-5f);
    ASSERT_NEAR(mgr.pickups()[0].position.y, 2.0f, 1e-5f);
    ASSERT_NEAR(mgr.pickups()[0].position.z, 3.0f, 1e-5f);
    ASSERT_TRUE(mgr.pickups()[0].alive);
}

// ── PowerUpManager Collection ───────────────────────────────────────────────

void test_manager_try_collect_within_radius() {
    qe::game::PowerUpManager mgr;
    mgr.spawn(qe::game::PowerUpType::RapidFire, qe::math::Vec3(5, 0, 0));

    // Too far
    int result = mgr.try_collect(qe::math::Vec3(100, 0, 0));
    ASSERT_TRUE(result == -1);

    // Close enough
    result = mgr.try_collect(qe::math::Vec3(5.3f, 0, 0));
    ASSERT_TRUE(result == static_cast<int>(qe::game::PowerUpType::RapidFire));
}

void test_manager_effects_activate_on_collect() {
    qe::game::PowerUpManager mgr;
    mgr.spawn(qe::game::PowerUpType::DamageBoost, qe::math::Vec3(0, 0, 0));

    ASSERT_TRUE(mgr.effects().empty());
    ASSERT_TRUE(!mgr.has_effect(qe::game::PowerUpType::DamageBoost));

    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_TRUE(mgr.effects().size() == 1);
    ASSERT_TRUE(mgr.has_effect(qe::game::PowerUpType::DamageBoost));
}

// ── PowerUpManager Multipliers ──────────────────────────────────────────────

void test_manager_multipliers_correct_values() {
    qe::game::PowerUpManager mgr;

    // Default multipliers
    ASSERT_NEAR(mgr.get_fire_rate_multiplier(), 1.0f, 1e-5f);
    ASSERT_NEAR(mgr.get_damage_multiplier(), 1.0f, 1e-5f);
    ASSERT_NEAR(mgr.get_score_multiplier(), 1.0f, 1e-5f);
    ASSERT_NEAR(mgr.get_enemy_speed_multiplier(), 1.0f, 1e-5f);
    ASSERT_TRUE(mgr.get_triple_shot_count() == 1);
    ASSERT_TRUE(mgr.get_shield_hits() == 0);

    // Activate RapidFire
    mgr.spawn(qe::game::PowerUpType::RapidFire, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_NEAR(mgr.get_fire_rate_multiplier(), 0.5f, 1e-5f);

    // Activate DamageBoost
    mgr.spawn(qe::game::PowerUpType::DamageBoost, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_NEAR(mgr.get_damage_multiplier(), 2.0f, 1e-5f);

    // Activate ScoreMultiplier
    mgr.spawn(qe::game::PowerUpType::ScoreMultiplier, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_NEAR(mgr.get_score_multiplier(), 3.0f, 1e-5f);

    // Activate SlowMotion
    mgr.spawn(qe::game::PowerUpType::SlowMotion, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_NEAR(mgr.get_enemy_speed_multiplier(), 0.5f, 1e-5f);

    // Activate TripleShot
    mgr.spawn(qe::game::PowerUpType::TripleShot, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_TRUE(mgr.get_triple_shot_count() == 3);
}

// ── Shield Absorb ───────────────────────────────────────────────────────────

void test_manager_shield_absorb() {
    qe::game::PowerUpManager mgr;

    // No shield - absorb fails
    ASSERT_TRUE(!mgr.absorb_shield_hit());

    // Activate shield (value=3 hits)
    mgr.spawn(qe::game::PowerUpType::Shield, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_TRUE(mgr.get_shield_hits() == 3);

    // Absorb 3 hits
    ASSERT_TRUE(mgr.absorb_shield_hit());
    ASSERT_TRUE(mgr.get_shield_hits() == 2);

    ASSERT_TRUE(mgr.absorb_shield_hit());
    ASSERT_TRUE(mgr.get_shield_hits() == 1);

    ASSERT_TRUE(mgr.absorb_shield_hit());
    // Shield expired after 3rd hit
    ASSERT_TRUE(mgr.get_shield_hits() == 0);
    ASSERT_TRUE(!mgr.absorb_shield_hit());
}

// ── Manager Update Removes Expired ──────────────────────────────────────────

void test_manager_update_removes_expired() {
    qe::game::PowerUpManager mgr;

    // Spawn a pickup with short lifetime
    mgr.spawn(qe::game::PowerUpType::RapidFire, qe::math::Vec3(10, 0, 0));
    ASSERT_TRUE(mgr.pickups().size() == 1);

    // Collect a DamageBoost to create an active effect
    mgr.spawn(qe::game::PowerUpType::DamageBoost, qe::math::Vec3(0, 0, 0));
    mgr.try_collect(qe::math::Vec3(0, 0, 0));
    ASSERT_TRUE(mgr.has_effect(qe::game::PowerUpType::DamageBoost));

    // Update past DamageBoost duration (8s) and RapidFire pickup lifetime (15s)
    mgr.update(16.0f);

    // Dead pickup removed
    ASSERT_TRUE(mgr.pickups().empty());
    // Expired effect removed
    ASSERT_TRUE(!mgr.has_effect(qe::game::PowerUpType::DamageBoost));
    ASSERT_TRUE(mgr.effects().empty());
}

// ── Glow Intensity ──────────────────────────────────────────────────────────

void test_powerup_glow_intensity_oscillates() {
    qe::game::PowerUp p;
    p.type = qe::game::PowerUpType::ScoreMultiplier;
    p.position = qe::math::Vec3(0, 0, 0);

    // At age=0, glow = 0.5 + 0.5*sin(0) = 0.5
    float g0 = p.glow_intensity();
    ASSERT_NEAR(g0, 0.5f, 1e-5f);

    // Advance to a time where sin is positive
    p.update(0.3f);
    float g1 = p.glow_intensity();
    // sin(0.3 * 4) = sin(1.2) > 0
    ASSERT_TRUE(g1 > 0.5f);

    // Glow should always be in [0, 1]
    for (int i = 0; i < 50; ++i) {
        p.update(0.1f);
        float g = p.glow_intensity();
        ASSERT_TRUE(g >= -0.01f && g <= 1.01f);
    }
}

// ── Config Validation ───────────────────────────────────────────────────────

void test_config_values_reasonable() {
    using T = qe::game::PowerUpType;
    T all_types[] = {T::RapidFire, T::TripleShot, T::DamageBoost,
                     T::SlowMotion, T::Shield, T::ScoreMultiplier};

    for (auto t : all_types) {
        auto cfg = qe::game::PowerUpManager::get_config(t);
        // Duration should be positive
        ASSERT_TRUE(cfg.duration > 0.0f);
        // Spawn chance in (0, 1)
        ASSERT_TRUE(cfg.spawn_chance > 0.0f && cfg.spawn_chance < 1.0f);
        // Color components in [0, 1]
        ASSERT_TRUE(cfg.color.x >= 0.0f && cfg.color.x <= 1.0f);
        ASSERT_TRUE(cfg.color.y >= 0.0f && cfg.color.y <= 1.0f);
        ASSERT_TRUE(cfg.color.z >= 0.0f && cfg.color.z <= 1.0f);
        // Value should be positive
        ASSERT_TRUE(cfg.value > 0.0f);
    }

    // Specific expected values
    auto rf = qe::game::PowerUpManager::get_config(T::RapidFire);
    ASSERT_NEAR(rf.duration, 10.0f, 1e-5f);
    ASSERT_NEAR(rf.spawn_chance, 0.15f, 1e-5f);
    ASSERT_NEAR(rf.value, 0.5f, 1e-5f);

    auto sh = qe::game::PowerUpManager::get_config(T::Shield);
    ASSERT_NEAR(sh.duration, 20.0f, 1e-5f);
    ASSERT_NEAR(sh.value, 3.0f, 1e-5f);

    auto sm = qe::game::PowerUpManager::get_config(T::ScoreMultiplier);
    ASSERT_NEAR(sm.duration, 15.0f, 1e-5f);
    ASSERT_NEAR(sm.value, 3.0f, 1e-5f);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== QuatEngine PowerUp System Tests ===" << std::endl;

    std::cout << "\n--- PowerUp Lifecycle ---" << std::endl;
    RUN_TEST(test_powerup_starts_alive_dies_after_lifetime);

    std::cout << "\n--- Quaternion Animation ---" << std::endl;
    RUN_TEST(test_powerup_rotation_changes_over_time);

    std::cout << "\n--- Bob Animation ---" << std::endl;
    RUN_TEST(test_powerup_display_position_includes_bob);

    std::cout << "\n--- Collection ---" << std::endl;
    RUN_TEST(test_powerup_can_collect_within_radius);
    RUN_TEST(test_powerup_can_collect_fails_when_too_far);
    RUN_TEST(test_powerup_collect_marks_not_alive);

    std::cout << "\n--- ActiveEffect ---" << std::endl;
    RUN_TEST(test_active_effect_tracks_duration);
    RUN_TEST(test_active_effect_is_active_and_remaining);

    std::cout << "\n--- PowerUpManager ---" << std::endl;
    RUN_TEST(test_manager_spawn_creates_pickup);
    RUN_TEST(test_manager_try_collect_within_radius);
    RUN_TEST(test_manager_effects_activate_on_collect);
    RUN_TEST(test_manager_multipliers_correct_values);
    RUN_TEST(test_manager_shield_absorb);
    RUN_TEST(test_manager_update_removes_expired);

    std::cout << "\n--- Visual Effects ---" << std::endl;
    RUN_TEST(test_powerup_glow_intensity_oscillates);

    std::cout << "\n--- Config Validation ---" << std::endl;
    RUN_TEST(test_config_values_reasonable);

    return TEST_REPORT();
}
