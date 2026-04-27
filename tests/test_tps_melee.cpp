// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_tps_melee.cpp
 * @brief Tests for MeleeSystem — combos, parry, backstab, arc detection, state machine.
 *
 * Validates:
 *   - Light attack 3-hit combo chain with timing windows
 *   - Heavy attack properties
 *   - Parry window timing
 *   - Backstab damage values
 *   - Attack arc detection using quaternion forward
 *   - State machine transitions (Idle -> Windup -> Active -> Recovery -> Chain)
 *   - Stamina gating
 */

#include "test_framework.h"

#include "../src/game/tps/MeleeSystem.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <iostream>
#include <string>


using namespace qe::game::tps;
using namespace qe::math;

// ── Move Config Tests ────────────────────────────────────────────────────────

void test_light_attack_configs() {
    auto l1 = make_melee_move(MeleeMove::LightAttack1);
    auto l2 = make_melee_move(MeleeMove::LightAttack2);
    auto l3 = make_melee_move(MeleeMove::LightAttack3);

    // Each successive hit does more damage
    ASSERT_TRUE(l2.base_damage > l1.base_damage);
    ASSERT_TRUE(l3.base_damage > l2.base_damage);

    // L3 has wider arc (sweep finisher)
    ASSERT_TRUE(l3.arc_angle > l1.arc_angle);

    // All have valid invariants
    l1.check_invariants();
    l2.check_invariants();
    l3.check_invariants();
}

void test_heavy_attack_config() {
    auto h = make_melee_move(MeleeMove::HeavyAttack);
    auto l1 = make_melee_move(MeleeMove::LightAttack1);

    // Heavy does more damage but is slower
    ASSERT_TRUE(h.base_damage > l1.base_damage);
    ASSERT_TRUE(h.windup > l1.windup);
    ASSERT_TRUE(h.stamina_cost > l1.stamina_cost);
    h.check_invariants();
}

void test_jump_attack_has_splash() {
    auto ja = make_melee_move(MeleeMove::JumpAttack);
    ASSERT_TRUE(ja.splash_radius > 0.0f);
    ASSERT_TRUE(ja.arc_angle > 6.0f);  // Full 360
    ja.check_invariants();
}

void test_backstab_high_damage() {
    auto bs = make_melee_move(MeleeMove::Backstab);
    auto l3 = make_melee_move(MeleeMove::LightAttack3);
    ASSERT_TRUE(bs.base_damage > l3.base_damage);
    ASSERT_TRUE(bs.stagger_power >= 1.0f);  // Always staggers
}

// ── State Machine Tests ──────────────────────────────────────────────────────

void test_initial_state_is_idle() {
    MeleeCombatState state;
    ASSERT_TRUE(state.is_idle());
    ASSERT_TRUE(state.combo_count() == 0);
    ASSERT_TRUE(state.can_act());
}

void test_light_attack_transitions() {
    MeleeCombatState state;
    float stamina = 100.0f;

    // Start first light attack
    ASSERT_TRUE(state.start_light_attack(stamina));
    ASSERT_TRUE(state.phase() == MeleePhase::Windup);
    ASSERT_TRUE(state.combo_count() == 1);

    // Advance through windup -> active -> recovery
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    state.update(cfg.windup + 0.001f);
    ASSERT_TRUE(state.phase() == MeleePhase::Active);

    state.update(cfg.active_frames + 0.001f);
    ASSERT_TRUE(state.phase() == MeleePhase::Recovery);

    state.update(cfg.recovery + 0.001f);
    ASSERT_TRUE(state.phase() == MeleePhase::ChainWindow);
}

void test_combo_chain_window() {
    MeleeCombatState state;
    float stamina = 100.0f;

    // First attack
    state.start_light_attack(stamina);
    auto cfg1 = make_melee_move(MeleeMove::LightAttack1);

    // Step through phases incrementally to avoid overshooting
    state.update(cfg1.windup + 0.001f);   // Windup -> Active
    state.update(cfg1.active_frames + 0.001f);  // Active -> Recovery
    state.update(cfg1.recovery + 0.001f);  // Recovery -> ChainWindow
    ASSERT_TRUE(state.in_chain_window());

    // Second attack during chain window
    ASSERT_TRUE(state.start_light_attack(stamina));
    ASSERT_TRUE(state.combo_count() == 2);
    ASSERT_TRUE(state.current_move() == MeleeMove::LightAttack2);
}

void test_combo_expires() {
    MeleeCombatState state;
    float stamina = 100.0f;

    state.start_light_attack(stamina);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);

    // Step through each phase individually
    state.update(cfg.windup + 0.001f);         // -> Active
    state.update(cfg.active_frames + 0.001f);  // -> Recovery
    state.update(cfg.recovery + 0.001f);       // -> ChainWindow
    state.update(cfg.chain_window + 0.1f);     // -> Idle (chain expired)
    ASSERT_TRUE(state.is_idle());
    ASSERT_TRUE(state.combo_count() == 0);
}

void test_insufficient_stamina_prevents_attack() {
    MeleeCombatState state;
    float low_stamina = 1.0f;  // Less than any attack cost
    ASSERT_TRUE(!state.start_light_attack(low_stamina));
    ASSERT_TRUE(!state.start_heavy_attack(low_stamina));
    ASSERT_TRUE(state.is_idle());
}

// ── Parry Tests ──────────────────────────────────────────────────────────────

void test_parry_window() {
    MeleeCombatState state;
    float stamina = 100.0f;

    ASSERT_TRUE(state.start_parry(stamina));
    ASSERT_TRUE(state.is_parrying());

    // Within parry window -> success
    ASSERT_TRUE(state.try_parry_incoming());
    ASSERT_TRUE(state.parry_succeeded());
}

void test_late_parry_fails() {
    MeleeCombatState state;
    float stamina = 100.0f;

    state.start_parry(stamina);
    auto cfg = make_melee_move(MeleeMove::Parry);
    // Wait past the active window
    state.update(cfg.active_frames + 0.1f);
    ASSERT_TRUE(!state.try_parry_incoming());
}

// ── Arc Detection Tests ──────────────────────────────────────────────────────

void test_target_in_front_arc() {
    MeleeCombatState state;
    float stamina = 100.0f;
    state.start_light_attack(stamina);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    state.update(cfg.windup + 0.001f);  // Enter Active phase

    Vec3 attacker_pos(0, 0, 0);
    Quaternion attacker_rot = Quaternion::identity();  // Facing -Z
    Vec3 target_in_front(0, 0, -1.5f);  // In front, within range
    Vec3 target_behind(0, 0, 1.5f);     // Behind

    ASSERT_TRUE(state.is_in_attack_arc(attacker_pos, attacker_rot, target_in_front));
    ASSERT_TRUE(!state.is_in_attack_arc(attacker_pos, attacker_rot, target_behind));
}

void test_target_out_of_range() {
    MeleeCombatState state;
    float stamina = 100.0f;
    state.start_light_attack(stamina);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    state.update(cfg.windup + 0.001f);

    Vec3 attacker_pos(0, 0, 0);
    Quaternion attacker_rot = Quaternion::identity();
    Vec3 far_target(0, 0, -50.0f);  // Way too far

    ASSERT_TRUE(!state.is_in_attack_arc(attacker_pos, attacker_rot, far_target));
}

void test_interrupt_resets_state() {
    MeleeCombatState state;
    float stamina = 100.0f;
    state.start_light_attack(stamina);
    state.interrupt();
    ASSERT_TRUE(state.is_idle());
    ASSERT_TRUE(state.combo_count() == 0);
}

// ── Damage Computation ───────────────────────────────────────────────────────

void test_damage_with_class_mult() {
    MeleeCombatState state;
    float stamina = 100.0f;
    state.start_light_attack(stamina);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    state.update(cfg.windup + 0.001f);

    float dmg = state.compute_damage(1.4f);  // Vanguard melee mult
    ASSERT_NEAR(dmg, cfg.base_damage * 1.4f, 0.01f);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Melee System Tests ===" << std::endl;

    std::cout << "\n--- Move Configs ---" << std::endl;
    RUN_TEST(test_light_attack_configs);
    RUN_TEST(test_heavy_attack_config);
    RUN_TEST(test_jump_attack_has_splash);
    RUN_TEST(test_backstab_high_damage);

    std::cout << "\n--- State Machine ---" << std::endl;
    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_light_attack_transitions);
    RUN_TEST(test_combo_chain_window);
    RUN_TEST(test_combo_expires);
    RUN_TEST(test_insufficient_stamina_prevents_attack);

    std::cout << "\n--- Parry ---" << std::endl;
    RUN_TEST(test_parry_window);
    RUN_TEST(test_late_parry_fails);

    std::cout << "\n--- Arc Detection ---" << std::endl;
    RUN_TEST(test_target_in_front_arc);
    RUN_TEST(test_target_out_of_range);
    RUN_TEST(test_interrupt_resets_state);

    std::cout << "\n--- Damage ---" << std::endl;
    RUN_TEST(test_damage_with_class_mult);

    return TEST_REPORT();
}
