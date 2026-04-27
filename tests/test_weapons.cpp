// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_weapons.cpp
 * @brief Tests for Weapon system and Combo/Scoring system.
 *
 * Validates:
 *   - WeaponManager: loadout, switching, ammo, reloading, cooldowns
 *   - Spread directions: quaternion-based cone generation
 *   - ComboState: streaks, multipliers, expiry, combo names
 *   - ScoreTracker: scoring, multipliers, wave bonuses, reset
 */

#include "test_framework.h"

#include "../src/game/Weapons.h"
#include "../src/game/Scoring.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>


// ── Weapon Tests ────────────────────────────────────────────────────────────

void test_weapon_starts_with_pistol() {
    qe::game::WeaponManager mgr;
    ASSERT_TRUE(mgr.current_index() == 0);
    ASSERT_TRUE(mgr.current().type == qe::game::WeaponType::Pistol);
}

void test_weapon_switch() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1);
    ASSERT_TRUE(mgr.current_index() == 1);
    ASSERT_TRUE(mgr.current().type == qe::game::WeaponType::Shotgun);
    mgr.switch_weapon(2);
    ASSERT_TRUE(mgr.current().type == qe::game::WeaponType::RailGun);
    mgr.switch_weapon(3);
    ASSERT_TRUE(mgr.current().type == qe::game::WeaponType::RocketLauncher);
    mgr.switch_weapon(4);
    ASSERT_TRUE(mgr.current().type == qe::game::WeaponType::MiniGun);
}

void test_pistol_infinite_ammo() {
    qe::game::WeaponManager mgr;
    // Pistol should have infinite ammo (max_ammo == -1)
    ASSERT_TRUE(mgr.current().max_ammo == -1);
    ASSERT_TRUE(mgr.current().ammo == -1);
    // Firing should not decrease ammo
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == -1);
}

void test_shotgun_fire_produces_5_directions() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun
    ASSERT_TRUE(mgr.current().pellet_count == 5);

    qe::math::Vec3 forward(0, 0, -1);
    qe::math::Vec3 up(0, 1, 0);
    auto dirs = mgr.compute_fire_directions(forward, up);
    ASSERT_TRUE(static_cast<int>(dirs.size()) == 5);
}

void test_spread_directions_within_angle() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, spread_angle = 0.15

    qe::math::Vec3 forward(0, 0, -1);
    qe::math::Vec3 up(0, 1, 0);
    auto dirs = mgr.compute_fire_directions(forward, up);

    float max_spread = mgr.current().spread_angle;
    for (const auto& d : dirs) {
        // Angle between forward and direction
        float dot = forward.dot(d);
        // Clamp for numerical safety
        if (dot > 1.0f) dot = 1.0f;
        if (dot < -1.0f) dot = -1.0f;
        float angle = std::acos(dot);
        // Each pellet direction should be within a reasonable bound of the spread
        // The combined pitch+yaw can produce slightly larger angles than spread_angle
        // so we allow 2x the spread as a generous bound
        ASSERT_TRUE(angle < max_spread * 2.5f);
    }
}

void test_spread_directions_are_unit_vectors() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun

    qe::math::Vec3 forward(0, 0, -1);
    qe::math::Vec3 up(0, 1, 0);
    auto dirs = mgr.compute_fire_directions(forward, up);

    for (const auto& d : dirs) {
        ASSERT_NEAR(d.length(), 1.0f, 1e-4f);
    }
}

void test_fire_consumes_ammo() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, ammo=8
    ASSERT_TRUE(mgr.current().ammo == 8);
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == 7);
    mgr.update(1.0f); // Clear cooldown
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == 6);
}

void test_reload_restores_ammo() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, max_ammo=8, reload_time=2.0
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == 7);
    mgr.reload();
    ASSERT_TRUE(mgr.is_reloading());
    // Simulate reload completing
    mgr.update(2.1f);
    ASSERT_TRUE(!mgr.is_reloading());
    ASSERT_TRUE(mgr.current().ammo == 8);
}

void test_cooldown_prevents_rapid_fire() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, fire_rate=0.8
    ASSERT_TRUE(mgr.can_fire());
    mgr.fire();
    ASSERT_TRUE(!mgr.can_fire()); // Cooldown active
    mgr.update(0.5f);
    ASSERT_TRUE(!mgr.can_fire()); // Still cooling down
    mgr.update(0.4f);
    ASSERT_TRUE(mgr.can_fire());  // Cooldown expired
}

void test_compute_fire_directions_uses_quaternion() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun with spread

    qe::math::Vec3 forward(0, 0, -1);
    qe::math::Vec3 up(0, 1, 0);
    auto dirs = mgr.compute_fire_directions(forward, up);

    // At least some directions should differ from forward due to spread
    int different = 0;
    for (const auto& d : dirs) {
        if (!d.approx_equal(forward, 1e-3f)) {
            different++;
        }
    }
    ASSERT_TRUE(different > 0);
}

void test_next_prev_weapon_cycle() {
    qe::game::WeaponManager mgr;
    ASSERT_TRUE(mgr.current_index() == 0);
    ASSERT_TRUE(mgr.weapon_count() == 5);

    mgr.next_weapon();
    ASSERT_TRUE(mgr.current_index() == 1);
    mgr.next_weapon();
    ASSERT_TRUE(mgr.current_index() == 2);

    // Cycle forward past last
    mgr.switch_weapon(4);
    mgr.next_weapon();
    ASSERT_TRUE(mgr.current_index() == 0); // Wraps around

    // Cycle backward past first
    mgr.switch_weapon(0);
    mgr.prev_weapon();
    ASSERT_TRUE(mgr.current_index() == 4); // Wraps around

    mgr.prev_weapon();
    ASSERT_TRUE(mgr.current_index() == 3);
}

// ── Weapon Negative / Error-Path Tests ─────────────────────────────────────
//
// These verify the documented no-op / clamp contracts in Weapons.h. The
// API never throws — instead, invalid operations are silently ignored so
// callers can fire the input layer blindly without guarding every call.
// Each test below pins one of those error paths.

void test_weapons_fire_with_empty_ammo_silently_fails() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, max_ammo=8
    // Drain the magazine deterministically — allow enough time between
    // shots to clear the cooldown each round.
    for (int i = 0; i < 8; ++i) {
        ASSERT_TRUE(mgr.can_fire());
        mgr.fire();
        mgr.update(10.0f); // Clear cooldown only — we never call reload().
    }
    ASSERT_TRUE(mgr.current().ammo == 0);
    ASSERT_TRUE(!mgr.can_fire());        // Contract: empty ammo disables firing.
    ASSERT_TRUE(!mgr.is_reloading());    // Contract: fire() does NOT auto-reload.

    // Calling fire() with an empty magazine must be a silent no-op: no
    // exception, no crash, no negative ammo, no cooldown kicked.
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == 0);
    ASSERT_NEAR(mgr.cooldown_progress(), 0.0f, 1e-6f);
    ASSERT_TRUE(!mgr.is_reloading());
}

void test_weapons_reload_while_reloading_is_noop() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, reload_time=2.0
    mgr.fire();
    mgr.update(10.0f); // Clear cooldown so reload() is the only state change.

    mgr.reload();
    ASSERT_TRUE(mgr.is_reloading());

    // Advance part-way through the reload, then request a second reload.
    mgr.update(0.5f);
    float progress_before = mgr.reload_progress();
    ASSERT_TRUE(progress_before > 0.0f);
    ASSERT_TRUE(progress_before < 1.0f);

    mgr.reload(); // Contract: must be a no-op — do NOT restart the timer.
    ASSERT_TRUE(mgr.is_reloading());
    // Progress should be unchanged (no second reload_timer_ reset).
    ASSERT_NEAR(mgr.reload_progress(), progress_before, 1e-5f);

    // And the reload should still complete on the original schedule.
    mgr.update(1.6f);
    ASSERT_TRUE(!mgr.is_reloading());
    ASSERT_TRUE(mgr.current().ammo == mgr.current().max_ammo);
}

void test_weapons_reload_at_full_ammo_is_noop() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun, starts full.
    ASSERT_TRUE(mgr.current().ammo == mgr.current().max_ammo);

    mgr.reload(); // Contract: reloading a full magazine is a no-op.
    ASSERT_TRUE(!mgr.is_reloading());
    ASSERT_NEAR(mgr.reload_progress(), 0.0f, 1e-6f);
}

void test_weapons_reload_infinite_ammo_is_noop() {
    qe::game::WeaponManager mgr;
    // Pistol has max_ammo == -1 (infinite); reload() must early-return.
    ASSERT_TRUE(mgr.current().max_ammo == -1);
    mgr.reload();
    ASSERT_TRUE(!mgr.is_reloading());
}

void test_weapons_switch_to_invalid_index_is_ignored() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(2); // RailGun
    ASSERT_TRUE(mgr.current_index() == 2);

    // Out-of-range indices must be silently ignored — no throw, no change.
    mgr.switch_weapon(-1);
    ASSERT_TRUE(mgr.current_index() == 2);

    mgr.switch_weapon(99);
    ASSERT_TRUE(mgr.current_index() == 2);

    mgr.switch_weapon(mgr.weapon_count());
    ASSERT_TRUE(mgr.current_index() == 2);

    // Sanity: a valid index still works after rejected calls.
    mgr.switch_weapon(0);
    ASSERT_TRUE(mgr.current_index() == 0);
}

void test_weapons_can_fire_returns_false_during_reload() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun
    mgr.fire();
    mgr.update(10.0f); // Clear cooldown so can_fire()==true before reload.
    ASSERT_TRUE(mgr.can_fire());

    mgr.reload();
    ASSERT_TRUE(mgr.is_reloading());
    ASSERT_TRUE(!mgr.can_fire());            // Contract: cannot fire mid-reload.

    // Firing during reload must be a silent no-op — ammo must not tick down
    // and the reload must continue unaffected.
    int ammo_before = mgr.current().ammo;
    mgr.fire();
    ASSERT_TRUE(mgr.current().ammo == ammo_before);
    ASSERT_TRUE(mgr.is_reloading());
}

void test_weapons_can_fire_returns_false_during_cooldown() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(2); // RailGun, fire_rate=1.5
    ASSERT_TRUE(mgr.can_fire());
    mgr.fire();
    ASSERT_TRUE(!mgr.can_fire());            // Contract: cooldown blocks fire.

    // Cooldown progress should be (close to) 1.0 immediately after firing
    // and must fall monotonically to 0 as we advance time.
    float start = mgr.cooldown_progress();
    ASSERT_TRUE(start > 0.0f);
    mgr.update(0.75f);
    ASSERT_TRUE(mgr.cooldown_progress() < start);
    ASSERT_TRUE(!mgr.can_fire());
    mgr.update(1.0f);
    ASSERT_NEAR(mgr.cooldown_progress(), 0.0f, 1e-5f);
    ASSERT_TRUE(mgr.can_fire());
}

void test_weapons_switch_weapon_cancels_reload() {
    qe::game::WeaponManager mgr;
    mgr.switch_weapon(1); // Shotgun
    mgr.fire();
    mgr.update(10.0f);
    mgr.reload();
    ASSERT_TRUE(mgr.is_reloading());

    // Documented behaviour in switch_weapon: reload state is cleared on
    // weapon change (reloading_ = false; reload_timer_ = 0).
    mgr.switch_weapon(2);
    ASSERT_TRUE(!mgr.is_reloading());
    ASSERT_NEAR(mgr.reload_progress(), 0.0f, 1e-6f);
}

void test_weapons_reload_progress_is_zero_when_not_reloading() {
    qe::game::WeaponManager mgr;
    ASSERT_NEAR(mgr.reload_progress(), 0.0f, 1e-6f);
    mgr.switch_weapon(1);
    ASSERT_NEAR(mgr.reload_progress(), 0.0f, 1e-6f);
}

// ── Scoring Tests ───────────────────────────────────────────────────────────

void test_initial_score_is_zero() {
    qe::game::ScoreTracker tracker;
    ASSERT_TRUE(tracker.score() == 0);
    ASSERT_TRUE(tracker.high_score() == 0);
}

void test_record_kill_adds_score() {
    qe::game::ScoreTracker tracker;
    auto event = tracker.record_kill(100);
    ASSERT_TRUE(tracker.score() > 0);
    ASSERT_TRUE(event.base_points == 100);
}

void test_combo_multiplier_increases() {
    qe::game::ComboState combo;
    ASSERT_NEAR(combo.multiplier(), 1.0f, 1e-5f); // streak=0

    combo.register_hit(); // streak=1
    ASSERT_NEAR(combo.multiplier(), 1.0f, 1e-5f);

    combo.register_hit(); // streak=2
    ASSERT_NEAR(combo.multiplier(), 1.5f, 1e-5f);

    for (int i = 0; i < 3; ++i) combo.register_hit(); // streak=5
    ASSERT_NEAR(combo.multiplier(), 2.0f, 1e-5f);

    for (int i = 0; i < 5; ++i) combo.register_hit(); // streak=10
    ASSERT_NEAR(combo.multiplier(), 3.0f, 1e-5f);

    for (int i = 0; i < 10; ++i) combo.register_hit(); // streak=20
    ASSERT_NEAR(combo.multiplier(), 5.0f, 1e-5f);
}

void test_combo_expires_after_window() {
    qe::game::ComboState combo;
    combo.register_hit();
    combo.register_hit();
    ASSERT_TRUE(combo.streak == 2);

    combo.update(2.1f); // Exceeds combo_window of 2.0
    ASSERT_TRUE(combo.streak == 0);
}

void test_miss_resets_combo() {
    qe::game::ComboState combo;
    combo.register_hit();
    combo.register_hit();
    combo.register_hit();
    ASSERT_TRUE(combo.streak == 3);

    combo.register_miss();
    ASSERT_TRUE(combo.streak == 0);
}

void test_combo_name_returns_correct_strings() {
    qe::game::ComboState combo;
    ASSERT_TRUE(std::strcmp(combo.combo_name(), "") == 0);

    combo.register_hit();
    combo.register_hit(); // streak=2
    ASSERT_TRUE(std::strcmp(combo.combo_name(), "COMBO x2!") == 0);

    for (int i = 0; i < 3; ++i) combo.register_hit(); // streak=5
    ASSERT_TRUE(std::strcmp(combo.combo_name(), "MEGA COMBO!") == 0);

    for (int i = 0; i < 5; ++i) combo.register_hit(); // streak=10
    ASSERT_TRUE(std::strcmp(combo.combo_name(), "ULTRA COMBO!") == 0);

    for (int i = 0; i < 10; ++i) combo.register_hit(); // streak=20
    ASSERT_TRUE(std::strcmp(combo.combo_name(), "GODLIKE!") == 0);
}

void test_score_event_total_with_multipliers() {
    qe::game::ScoreEvent event;
    event.base_points = 100;
    event.combo_multiplier = 2.0f;
    event.powerup_multiplier = 1.5f;
    event.wave_bonus = 50;
    // total = int(100 * 2.0 * 1.5) + 50 = 300 + 50 = 350
    ASSERT_TRUE(event.total() == 350);
}

void test_record_kill_with_wave_bonus() {
    qe::game::ScoreTracker tracker;
    auto event = tracker.record_kill(100, 1.0f, 25);
    // First kill: streak=1, multiplier=1.0
    // total = int(100 * 1.0 * 1.0) + 25 = 125
    ASSERT_TRUE(event.total() == 125);
    ASSERT_TRUE(tracker.score() == 125);
}

void test_max_streak_tracks_correctly() {
    qe::game::ComboState combo;
    combo.register_hit();
    combo.register_hit();
    combo.register_hit(); // streak=3
    ASSERT_TRUE(combo.max_streak == 3);

    combo.register_miss(); // streak=0, max_streak still 3
    ASSERT_TRUE(combo.max_streak == 3);
    ASSERT_TRUE(combo.streak == 0);

    combo.register_hit(); // streak=1
    ASSERT_TRUE(combo.max_streak == 3); // Still 3

    for (int i = 0; i < 4; ++i) combo.register_hit(); // streak=5
    ASSERT_TRUE(combo.max_streak == 5); // Updated
}

void test_reset_clears_everything() {
    qe::game::ScoreTracker tracker;
    tracker.record_kill(100);
    tracker.record_kill(100);
    tracker.record_kill(100);
    ASSERT_TRUE(tracker.score() > 0);
    ASSERT_TRUE(tracker.combo().streak > 0);

    tracker.reset();
    ASSERT_TRUE(tracker.score() == 0);
    ASSERT_TRUE(tracker.combo().streak == 0);
    ASSERT_TRUE(tracker.combo().max_streak == 0);
    ASSERT_TRUE(tracker.combo().total_kills == 0);
    ASSERT_TRUE(tracker.recent_events().empty());
}

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== QuatEngine Weapon & Scoring Tests ===" << std::endl;

    std::cout << "\n--- Weapons ---" << std::endl;
    RUN_TEST(test_weapon_starts_with_pistol);
    RUN_TEST(test_weapon_switch);
    RUN_TEST(test_pistol_infinite_ammo);
    RUN_TEST(test_shotgun_fire_produces_5_directions);
    RUN_TEST(test_spread_directions_within_angle);
    RUN_TEST(test_spread_directions_are_unit_vectors);
    RUN_TEST(test_fire_consumes_ammo);
    RUN_TEST(test_reload_restores_ammo);
    RUN_TEST(test_cooldown_prevents_rapid_fire);
    RUN_TEST(test_compute_fire_directions_uses_quaternion);
    RUN_TEST(test_next_prev_weapon_cycle);

    std::cout << "\n--- Weapon Negative Paths ---" << std::endl;
    RUN_TEST(test_weapons_fire_with_empty_ammo_silently_fails);
    RUN_TEST(test_weapons_reload_while_reloading_is_noop);
    RUN_TEST(test_weapons_reload_at_full_ammo_is_noop);
    RUN_TEST(test_weapons_reload_infinite_ammo_is_noop);
    RUN_TEST(test_weapons_switch_to_invalid_index_is_ignored);
    RUN_TEST(test_weapons_can_fire_returns_false_during_reload);
    RUN_TEST(test_weapons_can_fire_returns_false_during_cooldown);
    RUN_TEST(test_weapons_switch_weapon_cancels_reload);
    RUN_TEST(test_weapons_reload_progress_is_zero_when_not_reloading);

    std::cout << "\n--- Scoring ---" << std::endl;
    RUN_TEST(test_initial_score_is_zero);
    RUN_TEST(test_record_kill_adds_score);
    RUN_TEST(test_combo_multiplier_increases);
    RUN_TEST(test_combo_expires_after_window);
    RUN_TEST(test_miss_resets_combo);
    RUN_TEST(test_combo_name_returns_correct_strings);
    RUN_TEST(test_score_event_total_with_multipliers);
    RUN_TEST(test_record_kill_with_wave_bonus);
    RUN_TEST(test_max_streak_tracks_correctly);
    RUN_TEST(test_reset_clears_everything);

    return TEST_REPORT();
}
