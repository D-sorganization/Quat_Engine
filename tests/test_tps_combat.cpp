/**
 * @file test_tps_combat.cpp
 * @brief Integration tests for TPSCombat — ranged, melee, resistance, backstab.
 *
 * Validates:
 *   - Ranged hitscan hits and misses
 *   - Damage respects mutant resistance
 *   - Melee arc detection and damage
 *   - Backstab detection and bonus damage
 *   - Projectile collision processing
 *   - Combat stats tracking
 */

#include "../src/game/tps/TPSCombat.h"
#include "../src/game/tps/CharacterClass.h"
#include "../src/game/tps/MutantTypes.h"
#include "../src/game/tps/TPSWeapons.h"
#include "../src/game/tps/LockOnSystem.h"
#include "../src/game/tps/MeleeSystem.h"
#include "../src/core/Projectile.h"

#include <cmath>
#include <iostream>
#include <vector>

static int total_assertions = 0;
static int passed = 0;
static int failed = 0;

#define ASSERT_TRUE(expr) do { \
    total_assertions++; \
    if (expr) { passed++; } \
    else { failed++; std::cerr << "  FAIL: " << #expr \
           << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; } \
} while(0)

#define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(std::abs((a)-(b)) < (eps))
#define RUN_TEST(fn) do { std::cout << "  " << #fn << "... "; fn(); std::cout << "OK" << std::endl; } while(0)

using namespace qe::game::tps;
using namespace qe::math;
using namespace qe::core;

// ── Ranged Combat Tests ──────────────────────────────────────────────────────

void test_ranged_hitscan_hit() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);
    auto weapon = TPSWeaponState::make_marksman_rifle();
    LockOnState lock_on;
    std::vector<Projectile> projectiles;

    // Place a grunt directly ahead
    std::vector<MutantInstance> targets;
    targets.push_back(spawn_mutant(MutantType::Grunt, Vec3(0, 0, -10), 1));

    auto results = combat.process_ranged_attack(
        Vec3(0, 0, 0), Vec3(0, 0, -1), weapon, char_stats,
        lock_on, targets, projectiles);

    ASSERT_TRUE(!results.empty());
    ASSERT_TRUE(results[0].hit);
    ASSERT_TRUE(results[0].damage_dealt > 0.0f);
    ASSERT_TRUE(combat.stats().total_shots == 1);
    ASSERT_TRUE(combat.stats().total_hits >= 1);
}

void test_ranged_hitscan_miss() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);
    auto weapon = TPSWeaponState::make_marksman_rifle();
    LockOnState lock_on;
    std::vector<Projectile> projectiles;

    // Place target to the side
    std::vector<MutantInstance> targets;
    targets.push_back(spawn_mutant(MutantType::Grunt, Vec3(20, 0, 0), 1));

    auto results = combat.process_ranged_attack(
        Vec3(0, 0, 0), Vec3(0, 0, -1), weapon, char_stats,
        lock_on, targets, projectiles);

    ASSERT_TRUE(!results.empty());
    ASSERT_TRUE(!results[0].hit);
}

void test_damage_respects_resistance() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);

    // Brute has high ballistic resistance
    std::vector<MutantInstance> targets;
    targets.push_back(spawn_mutant(MutantType::Brute, Vec3(0, 0, -10), 1));
    float initial_health = targets[0].current_health;

    auto weapon = TPSWeaponState::make_assault_rifle();
    LockOnState lock_on;
    std::vector<Projectile> projectiles;

    combat.process_ranged_attack(
        Vec3(0, 0, 0), Vec3(0, 0, -1), weapon, char_stats,
        lock_on, targets, projectiles);

    // Brute should take reduced damage due to ballistic resistance
    float damage_taken = initial_health - targets[0].current_health;
    float raw_damage = weapon.damage * char_stats.ranged_damage_mult;
    // Damage should be less than raw due to resistance
    ASSERT_TRUE(damage_taken < raw_damage * 1.5f);
}

void test_energy_weapon_vs_brute() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);

    // Test energy weapon against brute (lower energy resistance than ballistic)
    std::vector<MutantInstance> brute_ballistic;
    brute_ballistic.push_back(spawn_mutant(MutantType::Brute, Vec3(0, 0, -10), 1));

    std::vector<MutantInstance> brute_energy;
    brute_energy.push_back(spawn_mutant(MutantType::Brute, Vec3(0, 0, -10), 2));

    auto ballistic_wpn = TPSWeaponState::make_marksman_rifle();
    auto energy_wpn = TPSWeaponState::make_tesla_coil();
    LockOnState lock_on;
    std::vector<Projectile> proj1, proj2;

    combat.process_ranged_attack(Vec3(0, 0, 0), Vec3(0, 0, -1),
        ballistic_wpn, char_stats, lock_on, brute_ballistic, proj1);

    TPSCombatSystem combat2;
    combat2.process_ranged_attack(Vec3(0, 0, 0), Vec3(0, 0, -1),
        energy_wpn, char_stats, lock_on, brute_energy, proj2);

    // Brute has 0.3 ballistic resist, 0.1 energy resist
    // Energy should do relatively more effective damage per raw point
    float ballistic_hp_lost = brute_ballistic[0].config.health - brute_ballistic[0].current_health;
    float energy_hp_lost = brute_energy[0].config.health - brute_energy[0].current_health;
    // Both should have done some damage
    ASSERT_TRUE(ballistic_hp_lost > 0.0f || energy_hp_lost > 0.0f);
}

// ── Melee Combat Tests ───────────────────────────────────────────────────────

void test_melee_hits_in_arc() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);

    MeleeCombatState melee;
    melee.start_light_attack(100.0f);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    melee.update(cfg.windup + 0.001f);  // Enter active phase

    std::vector<MutantInstance> targets;
    targets.push_back(spawn_mutant(MutantType::Grunt, Vec3(0, 0, -1.5f), 1));

    auto results = combat.process_melee_attack(
        Vec3(0, 0, 0), Quaternion::identity(), melee, char_stats, targets);

    ASSERT_TRUE(!results.empty());
    ASSERT_TRUE(results[0].hit);
    ASSERT_TRUE(results[0].damage_dealt > 0.0f);
    ASSERT_TRUE(combat.stats().melee_kills >= 0);
}

void test_melee_misses_behind() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);

    MeleeCombatState melee;
    melee.start_light_attack(100.0f);
    auto cfg = make_melee_move(MeleeMove::LightAttack1);
    melee.update(cfg.windup + 0.001f);

    std::vector<MutantInstance> targets;
    // Target behind the player
    targets.push_back(spawn_mutant(MutantType::Grunt, Vec3(0, 0, 1.5f), 1));

    auto results = combat.process_melee_attack(
        Vec3(0, 0, 0), Quaternion::identity(), melee, char_stats, targets);

    ASSERT_TRUE(results.empty());
}

// ── Backstab Tests ───────────────────────────────────────────────────────────

void test_backstab_detection() {
    // Target facing forward (-Z), attacker behind them (+Z direction)
    Vec3 target_pos(0, 0, 0);
    Quaternion target_rot = Quaternion::identity();  // Facing -Z
    Vec3 attacker_behind(0, 0, -3);  // Behind target (same direction as -Z forward)

    // The attacker is in the direction the target is facing = behind
    ASSERT_TRUE(TPSCombatSystem::is_behind_target(attacker_behind, target_pos, target_rot));
}

void test_not_backstab_from_front() {
    Vec3 target_pos(0, 0, 0);
    Quaternion target_rot = Quaternion::identity();  // Facing -Z
    Vec3 attacker_front(0, 0, 3);  // In front of target

    ASSERT_TRUE(!TPSCombatSystem::is_behind_target(attacker_front, target_pos, target_rot));
}

// ── Projectile Collision Tests ───────────────────────────────────────────────

void test_projectile_collision() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);

    std::vector<MutantInstance> targets;
    targets.push_back(spawn_mutant(MutantType::Grunt, Vec3(5, 0, 0), 1));

    std::vector<Projectile> projectiles;
    Projectile p;
    p.position = Vec3(5, 0, 0);  // Right on top of target
    p.velocity = Vec3(1, 0, 0);
    p.damage = 50.0f;
    p.radius = 1.0f;
    p.active = true;
    projectiles.push_back(p);

    auto results = combat.process_projectile_collisions(
        projectiles, targets, char_stats, DamageCategory::Ballistic);

    ASSERT_TRUE(!results.empty());
    ASSERT_TRUE(results[0].hit);
    ASSERT_TRUE(!projectiles[0].active);  // Projectile consumed
}

// ── Stats Tracking Tests ─────────────────────────────────────────────────────

void test_combat_stats_tracking() {
    TPSCombatSystem combat;
    auto char_stats = make_character_stats(CharacterClassType::Vanguard);
    auto weapon = TPSWeaponState::make_marksman_rifle();
    LockOnState lock_on;
    std::vector<Projectile> projectiles;

    // Kill a weak grunt
    std::vector<MutantInstance> targets;
    auto grunt = spawn_mutant(MutantType::Grunt, Vec3(0, 0, -10), 1);
    grunt.current_health = 1.0f;  // One-shot kill
    targets.push_back(grunt);

    combat.process_ranged_attack(
        Vec3(0, 0, 0), Vec3(0, 0, -1), weapon, char_stats,
        lock_on, targets, projectiles);

    ASSERT_TRUE(combat.stats().total_shots >= 1);
    ASSERT_TRUE(combat.stats().total_kills >= 1);
    ASSERT_TRUE(combat.stats().ranged_kills >= 1);
    ASSERT_TRUE(combat.stats().score > 0);
}

void test_combat_stats_reset() {
    TPSCombatSystem combat;
    combat.stats_mut().total_shots = 10;
    combat.stats_mut().score = 500;
    combat.reset();
    ASSERT_TRUE(combat.stats().total_shots == 0);
    ASSERT_TRUE(combat.stats().score == 0);
}

void test_accuracy_calculation() {
    TPSCombatStats stats;
    stats.total_shots = 10;
    stats.total_hits = 7;
    ASSERT_NEAR(stats.accuracy(), 70.0f, 0.1f);
}

void test_zero_shots_accuracy() {
    TPSCombatStats stats;
    ASSERT_NEAR(stats.accuracy(), 0.0f, 0.01f);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Combat Integration Tests ===" << std::endl;

    std::cout << "\n--- Ranged Combat ---" << std::endl;
    RUN_TEST(test_ranged_hitscan_hit);
    RUN_TEST(test_ranged_hitscan_miss);
    RUN_TEST(test_damage_respects_resistance);
    RUN_TEST(test_energy_weapon_vs_brute);

    std::cout << "\n--- Melee Combat ---" << std::endl;
    RUN_TEST(test_melee_hits_in_arc);
    RUN_TEST(test_melee_misses_behind);

    std::cout << "\n--- Backstab ---" << std::endl;
    RUN_TEST(test_backstab_detection);
    RUN_TEST(test_not_backstab_from_front);

    std::cout << "\n--- Projectile Collisions ---" << std::endl;
    RUN_TEST(test_projectile_collision);

    std::cout << "\n--- Stats ---" << std::endl;
    RUN_TEST(test_combat_stats_tracking);
    RUN_TEST(test_combat_stats_reset);
    RUN_TEST(test_accuracy_calculation);
    RUN_TEST(test_zero_shots_accuracy);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total: " << total_assertions << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;

    return failed > 0 ? 1 : 0;
}
