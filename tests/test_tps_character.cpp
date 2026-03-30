/**
 * @file test_tps_character.cpp
 * @brief Tests for CharacterClass system — stats, damage reduction, class balance.
 *
 * Validates:
 *   - All four classes produce valid stats (invariants pass)
 *   - Damage reduction applies armor correctly
 *   - Explosion resistance stacks with armor
 *   - Melee/ranged multipliers modify damage correctly
 *   - Class balance: no class dominates all categories
 */

#include "test_framework.h"

#include "../src/game/tps/CharacterClass.h"

#include <cmath>
#include <iostream>
#include <string>


using namespace qe::game::tps;

// ── Invariant Tests ──────────────────────────────────────────────────────────

void test_vanguard_stats_valid() {
    auto s = make_character_stats(CharacterClassType::Vanguard);
    ASSERT_TRUE(s.max_health > 0.0f);
    ASSERT_TRUE(s.move_speed > 0.0f);
    ASSERT_TRUE(s.armor >= 0.0f && s.armor <= 0.8f);
    ASSERT_TRUE(s.critical_chance >= 0.0f && s.critical_chance <= 1.0f);
    s.check_invariants();  // Should not assert-fail
}

void test_recon_stats_valid() {
    auto s = make_character_stats(CharacterClassType::Recon);
    ASSERT_TRUE(s.max_health > 0.0f);
    ASSERT_TRUE(s.move_speed > 0.0f);
    s.check_invariants();
}

void test_heavy_stats_valid() {
    auto s = make_character_stats(CharacterClassType::Heavy);
    ASSERT_TRUE(s.max_health > 0.0f);
    s.check_invariants();
}

void test_phantom_stats_valid() {
    auto s = make_character_stats(CharacterClassType::Phantom);
    ASSERT_TRUE(s.max_health > 0.0f);
    s.check_invariants();
}

// ── Damage Reduction Tests ───────────────────────────────────────────────────

void test_armor_reduces_damage() {
    auto s = make_character_stats(CharacterClassType::Heavy);
    float raw = 100.0f;
    float reduced = s.apply_damage_reduction(raw, false);
    // Heavy has 0.4 armor -> 60% damage
    ASSERT_NEAR(reduced, raw * (1.0f - s.armor), 0.01f);
    ASSERT_TRUE(reduced < raw);
}

void test_explosion_resistance_stacks() {
    auto s = make_character_stats(CharacterClassType::Heavy);
    float raw = 100.0f;
    float normal = s.apply_damage_reduction(raw, false);
    float explosive = s.apply_damage_reduction(raw, true);
    // Explosive should be further reduced by explosion_resistance
    ASSERT_TRUE(explosive < normal);
    ASSERT_NEAR(explosive, raw * (1.0f - s.armor) * (1.0f - s.explosion_resistance), 0.01f);
}

void test_zero_damage_stays_zero() {
    auto s = make_character_stats(CharacterClassType::Vanguard);
    ASSERT_NEAR(s.apply_damage_reduction(0.0f, false), 0.0f, 1e-6f);
    ASSERT_NEAR(s.apply_damage_reduction(0.0f, true), 0.0f, 1e-6f);
}

// ── Multiplier Tests ─────────────────────────────────────────────────────────

void test_melee_damage_multiplier() {
    auto vanguard = make_character_stats(CharacterClassType::Vanguard);
    auto recon = make_character_stats(CharacterClassType::Recon);
    float base = 50.0f;
    // Vanguard has higher melee mult than Recon
    ASSERT_TRUE(vanguard.effective_melee_damage(base) > recon.effective_melee_damage(base));
}

void test_ranged_damage_multiplier() {
    auto heavy = make_character_stats(CharacterClassType::Heavy);
    float base = 50.0f;
    float effective = heavy.effective_ranged_damage(base);
    ASSERT_NEAR(effective, base * heavy.ranged_damage_mult, 0.01f);
}

void test_phantom_backstab_bonus() {
    auto phantom = make_character_stats(CharacterClassType::Phantom);
    // Phantom has 1.8x melee mult (backstab specialist)
    ASSERT_TRUE(phantom.melee_damage_mult > 1.5f);
    float base = 80.0f;  // Backstab base
    float effective = phantom.effective_melee_damage(base);
    ASSERT_NEAR(effective, base * phantom.melee_damage_mult, 0.01f);
}

// ── Balance Tests ────────────────────────────────────────────────────────────

void test_no_class_dominates() {
    auto v = make_character_stats(CharacterClassType::Vanguard);
    auto r = make_character_stats(CharacterClassType::Recon);
    auto h = make_character_stats(CharacterClassType::Heavy);
    auto p = make_character_stats(CharacterClassType::Phantom);

    // Heavy has most HP
    ASSERT_TRUE(h.max_health > v.max_health);
    ASSERT_TRUE(h.max_health > r.max_health);
    ASSERT_TRUE(h.max_health > p.max_health);

    // Recon is fastest
    ASSERT_TRUE(r.move_speed > v.move_speed);
    ASSERT_TRUE(r.move_speed > h.move_speed);
    ASSERT_TRUE(r.move_speed > p.move_speed);

    // Phantom has best stealth
    ASSERT_TRUE(p.stealth_modifier < v.stealth_modifier);
    ASSERT_TRUE(p.stealth_modifier < h.stealth_modifier);
    ASSERT_TRUE(p.stealth_modifier < r.stealth_modifier);

    // Recon has best lock-on stability
    ASSERT_TRUE(r.lock_on_stability < v.lock_on_stability);
    ASSERT_TRUE(r.lock_on_stability < h.lock_on_stability);
}

void test_class_names() {
    ASSERT_TRUE(std::string(class_name(CharacterClassType::Vanguard)) == "Vanguard");
    ASSERT_TRUE(std::string(class_name(CharacterClassType::Recon)) == "Recon");
    ASSERT_TRUE(std::string(class_name(CharacterClassType::Heavy)) == "Heavy");
    ASSERT_TRUE(std::string(class_name(CharacterClassType::Phantom)) == "Phantom");
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Character Class Tests ===" << std::endl;

    std::cout << "\n--- Invariants ---" << std::endl;
    RUN_TEST(test_vanguard_stats_valid);
    RUN_TEST(test_recon_stats_valid);
    RUN_TEST(test_heavy_stats_valid);
    RUN_TEST(test_phantom_stats_valid);

    std::cout << "\n--- Damage Reduction ---" << std::endl;
    RUN_TEST(test_armor_reduces_damage);
    RUN_TEST(test_explosion_resistance_stacks);
    RUN_TEST(test_zero_damage_stays_zero);

    std::cout << "\n--- Multipliers ---" << std::endl;
    RUN_TEST(test_melee_damage_multiplier);
    RUN_TEST(test_ranged_damage_multiplier);
    RUN_TEST(test_phantom_backstab_bonus);

    std::cout << "\n--- Balance ---" << std::endl;
    RUN_TEST(test_no_class_dominates);
    RUN_TEST(test_class_names);

    return TEST_REPORT();
}
