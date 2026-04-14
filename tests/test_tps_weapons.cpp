/**
 * @file test_tps_weapons.cpp
 * @brief Tests for TPSWeapons — 7 weapon types, firing, reload, ADS, loadout.
 *
 * Validates:
 *   - All 7 weapon configs pass invariants
 *   - Fire rate limiting works
 *   - Ammo consumption and reload
 *   - ADS spread reduction
 *   - Loadout weapon switching
 *   - Damage category assignments
 *   - Invalid inputs raise std::invalid_argument / std::logic_error (Issue #104)
 */

#include "test_framework.h"

#include "../src/game/tps/TPSWeapons.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>


using namespace qe::game::tps;

// ── Config Invariant Tests ───────────────────────────────────────────────────

void test_assault_rifle_config() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.type == TPSWeaponType::AssaultRifle);
    ASSERT_TRUE(cfg.damage_category == DamageCategory::Ballistic);
    ASSERT_TRUE(cfg.magazine_size == 30);
}

void test_combat_shotgun_config() {
    auto cfg = TPSWeaponState::make_combat_shotgun();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.pellet_count == 8);
    ASSERT_TRUE(cfg.spread_angle > 0.1f);
}

void test_plasma_caster_config() {
    auto cfg = TPSWeaponState::make_plasma_caster();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.damage_category == DamageCategory::Energy);
    ASSERT_TRUE(cfg.splash_radius > 0.0f);
}

void test_marksman_rifle_config() {
    auto cfg = TPSWeaponState::make_marksman_rifle();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.piercing == true);
    ASSERT_TRUE(cfg.projectile_speed == 0.0f);  // Hitscan
    ASSERT_TRUE(cfg.ads_zoom > 3.0f);
}

void test_smg_config() {
    auto cfg = TPSWeaponState::make_submachine_gun();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.fire_rate > 10.0f);  // Very fast
    ASSERT_TRUE(cfg.magazine_size == 40);
}

void test_grenade_launcher_config() {
    auto cfg = TPSWeaponState::make_grenade_launcher();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.damage_category == DamageCategory::Explosive);
    ASSERT_TRUE(cfg.splash_radius > 4.0f);
}

void test_tesla_coil_config() {
    auto cfg = TPSWeaponState::make_tesla_coil();
    cfg.check_invariants();
    ASSERT_TRUE(cfg.damage_category == DamageCategory::Energy);
    ASSERT_TRUE(cfg.chain_targets > 0);
}

// ── Firing Tests ─────────────────────────────────────────────────────────────

void test_fire_consumes_ammo() {
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    int initial = weapon.ammo();
    ASSERT_TRUE(weapon.can_fire());
    weapon.fire();
    ASSERT_TRUE(weapon.ammo() == initial - 1);
}

void test_fire_rate_limiting() {
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    weapon.fire();
    ASSERT_TRUE(!weapon.can_fire());  // On cooldown

    // Partial cooldown
    weapon.update(0.05f);
    ASSERT_TRUE(!weapon.can_fire());

    // Full cooldown (1/8 = 0.125s)
    weapon.update(0.1f);
    ASSERT_TRUE(weapon.can_fire());
}

void test_empty_magazine_prevents_fire() {
    // Use a small magazine weapon
    auto cfg = TPSWeaponState::make_grenade_launcher();
    TPSWeaponState weapon(cfg);

    // Fire all 4 rounds
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(weapon.fire());
        weapon.update(2.0f);  // Clear cooldown
    }
    ASSERT_TRUE(!weapon.can_fire());
    ASSERT_TRUE(weapon.ammo() == 0);
}

// ── Reload Tests ─────────────────────────────────────────────────────────────

void test_reload_refills_magazine() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    TPSWeaponState weapon(cfg);

    // Fire 5 rounds
    for (int i = 0; i < 5; ++i) {
        weapon.fire();
        weapon.update(0.2f);
    }
    ASSERT_TRUE(weapon.ammo() == 25);

    // Start reload
    weapon.start_reload();
    ASSERT_TRUE(weapon.is_reloading());
    ASSERT_TRUE(!weapon.can_fire());

    // Complete reload
    weapon.update(cfg.reload_time + 0.1f);
    ASSERT_TRUE(!weapon.is_reloading());
    ASSERT_TRUE(weapon.ammo() == cfg.magazine_size);
}

void test_reload_from_reserve() {
    auto cfg = TPSWeaponState::make_grenade_launcher();
    TPSWeaponState weapon(cfg);

    // Fire all
    for (int i = 0; i < cfg.magazine_size; ++i) {
        weapon.fire();
        weapon.update(2.0f);
    }
    ASSERT_TRUE(weapon.ammo() == 0);

    int prev_reserve = weapon.reserve();
    weapon.start_reload();
    weapon.update(cfg.reload_time + 0.1f);
    ASSERT_TRUE(weapon.ammo() == cfg.magazine_size);
    ASSERT_TRUE(weapon.reserve() == prev_reserve - cfg.magazine_size);
}

// ── ADS Tests ────────────────────────────────────────────────────────────────

void test_ads_reduces_spread() {
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    float hip = weapon.effective_spread();
    weapon.set_ads(true);
    float ads = weapon.effective_spread();
    ASSERT_TRUE(ads < hip);
}

void test_ads_zoom() {
    TPSWeaponState weapon(TPSWeaponState::make_marksman_rifle());
    ASSERT_NEAR(weapon.effective_zoom(), 1.0f, 0.01f);
    weapon.set_ads(true);
    ASSERT_TRUE(weapon.effective_zoom() > 1.0f);
}

// ── Loadout Tests ────────────────────────────────────────────────────────────

void test_loadout_default() {
    TPSLoadout loadout;
    ASSERT_TRUE(loadout.weapon_count() == 2);  // AR + Shotgun
    ASSERT_TRUE(loadout.current_index() == 0);
}

void test_loadout_switch() {
    TPSLoadout loadout;
    loadout.next_weapon();
    ASSERT_TRUE(loadout.current_index() == 1);
    loadout.prev_weapon();
    ASSERT_TRUE(loadout.current_index() == 0);
}

void test_loadout_add_weapon() {
    TPSLoadout loadout;
    loadout.add_weapon(TPSWeaponState::make_plasma_caster());
    ASSERT_TRUE(loadout.weapon_count() == 3);
    loadout.switch_weapon(2);
    ASSERT_TRUE(loadout.current().config().type == TPSWeaponType::PlasmaCaster);
}

void test_weapon_names() {
    ASSERT_TRUE(std::string(weapon_name(TPSWeaponType::AssaultRifle)) == "M2 Assault Rifle");
    ASSERT_TRUE(std::string(weapon_name(TPSWeaponType::TeslaCoil)) == "Tesla Arc Projector");
}

// ── Validation / Error Handling Tests (Issue #104) ──────────────────────────

void test_config_zero_damage_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.damage = 0.0f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_negative_damage_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.damage = -1.0f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_zero_fire_rate_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.fire_rate = 0.0f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_negative_spread_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.spread_angle = -0.1f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_zero_pellet_count_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.pellet_count = 0;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_zero_magazine_size_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.magazine_size = 0;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_zero_reload_time_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.reload_time = 0.0f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_config_ads_zoom_below_one_throws() {
    auto cfg = TPSWeaponState::make_assault_rifle();
    cfg.ads_zoom = 0.5f;
    ASSERT_THROWS_AS(cfg.check_invariants(), std::invalid_argument);
}

void test_update_negative_dt_throws() {
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    ASSERT_THROWS_AS(weapon.update(-0.1f), std::invalid_argument);
}

void test_add_reserve_negative_throws() {
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    ASSERT_THROWS_AS(weapon.add_reserve_ammo(-1), std::invalid_argument);
}

void test_add_reserve_zero_is_valid() {
    // zero is a valid (no-op) amount — should not throw
    TPSWeaponState weapon(TPSWeaponState::make_assault_rifle());
    int before = weapon.reserve();
    weapon.add_reserve_ammo(0);
    ASSERT_TRUE(weapon.reserve() == before);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Weapons Tests ===" << std::endl;

    std::cout << "\n--- Configs ---" << std::endl;
    RUN_TEST(test_assault_rifle_config);
    RUN_TEST(test_combat_shotgun_config);
    RUN_TEST(test_plasma_caster_config);
    RUN_TEST(test_marksman_rifle_config);
    RUN_TEST(test_smg_config);
    RUN_TEST(test_grenade_launcher_config);
    RUN_TEST(test_tesla_coil_config);

    std::cout << "\n--- Firing ---" << std::endl;
    RUN_TEST(test_fire_consumes_ammo);
    RUN_TEST(test_fire_rate_limiting);
    RUN_TEST(test_empty_magazine_prevents_fire);

    std::cout << "\n--- Reload ---" << std::endl;
    RUN_TEST(test_reload_refills_magazine);
    RUN_TEST(test_reload_from_reserve);

    std::cout << "\n--- ADS ---" << std::endl;
    RUN_TEST(test_ads_reduces_spread);
    RUN_TEST(test_ads_zoom);

    std::cout << "\n--- Loadout ---" << std::endl;
    RUN_TEST(test_loadout_default);
    RUN_TEST(test_loadout_switch);
    RUN_TEST(test_loadout_add_weapon);
    RUN_TEST(test_weapon_names);

    std::cout << "\n--- Validation / Contract Guards (Issue #104) ---" << std::endl;
    RUN_TEST(test_config_zero_damage_throws);
    RUN_TEST(test_config_negative_damage_throws);
    RUN_TEST(test_config_zero_fire_rate_throws);
    RUN_TEST(test_config_negative_spread_throws);
    RUN_TEST(test_config_zero_pellet_count_throws);
    RUN_TEST(test_config_zero_magazine_size_throws);
    RUN_TEST(test_config_zero_reload_time_throws);
    RUN_TEST(test_config_ads_zoom_below_one_throws);
    RUN_TEST(test_update_negative_dt_throws);
    RUN_TEST(test_add_reserve_negative_throws);
    RUN_TEST(test_add_reserve_zero_is_valid);

    return TEST_REPORT();
}
