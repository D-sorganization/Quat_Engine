// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file TPSWeapons.h
 * @brief Seven weapon classes for the TPS game with distinct tactical roles.
 *
 * Each weapon interacts differently with character classes and mutant resistances,
 * creating a rich strategy layer. Weapon categories:
 *
 *   - AssaultRifle:    Balanced mid-range, moderate fire rate and damage
 *   - CombatShotgun:   Close-range devastation, wide spread, high burst
 *   - PlasmaCaster:    Slow energy weapon, high damage, splash, energy type
 *   - MarksmanRifle:   Long-range precision, high damage per shot, slow
 *   - Submachine Gun:  Fast fire rate, low per-shot damage, hip-fire bonus
 *   - GrenadeLauncher: Arc projectile, area denial, explosive type
 *   - TeslaCoil:       Chain lightning, hits multiple targets, energy type
 *
 * Orthogonal to CharacterClass and MutantTypes — weapon selection should be
 * driven by encounter composition, not class restrictions.
 */

#include "../../core/Rng.h"
#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace qe {
namespace game {
namespace tps {

enum class TPSWeaponType {
    AssaultRifle,
    CombatShotgun,
    PlasmaCaster,
    MarksmanRifle,
    SubmachineGun,
    GrenadeLauncher,
    TeslaCoil
};

// ── Damage Category (for resistance lookup) ──────────────────────────────────

enum class DamageCategory {
    Ballistic,
    Explosive,
    Energy
};

// ── Weapon Config ────────────────────────────────────────────────────────────

struct TPSWeaponConfig {
    TPSWeaponType type;
    DamageCategory damage_category;
    float damage;
    float fire_rate;          // Shots per second
    float projectile_speed;   // 0 = hitscan
    float spread_angle;       // Radians
    int pellet_count;
    float splash_radius;
    float range;
    bool piercing;
    float recoil;
    int magazine_size;
    int current_ammo;
    int reserve_ammo;
    float reload_time;
    float ads_zoom;           // Aim-down-sights zoom multiplier
    float ads_spread_mult;    // Spread reduction when ADS
    float hip_fire_penalty;   // Spread increase when hip-firing
    int chain_targets;        // For Tesla: number of chain bounces

    void check_invariants() const {
        if (damage <= 0.0f)
            throw std::invalid_argument("TPSWeaponConfig: damage must be > 0");
        if (fire_rate <= 0.0f)
            throw std::invalid_argument("TPSWeaponConfig: fire_rate must be > 0");
        if (spread_angle < 0.0f)
            throw std::invalid_argument("TPSWeaponConfig: spread_angle must be >= 0");
        if (pellet_count < 1)
            throw std::invalid_argument("TPSWeaponConfig: pellet_count must be >= 1");
        if (range <= 0.0f)
            throw std::invalid_argument("TPSWeaponConfig: range must be > 0");
        if (magazine_size <= 0)
            throw std::invalid_argument("TPSWeaponConfig: magazine_size must be > 0");
        if (reload_time <= 0.0f)
            throw std::invalid_argument("TPSWeaponConfig: reload_time must be > 0");
        if (ads_zoom < 1.0f)
            throw std::invalid_argument("TPSWeaponConfig: ads_zoom must be >= 1");
    }
};

// ── Weapon State ─────────────────────────────────────────────────────────────

class TPSWeaponState {
    TPSWeaponConfig config_;
    float fire_cooldown_ = 0.0f;
    float reload_timer_ = 0.0f;
    bool reloading_ = false;
    bool ads_ = false;

public:
    explicit TPSWeaponState(TPSWeaponConfig cfg) : config_(cfg) {
        config_.check_invariants();
    }

    TPSWeaponState() : config_(make_assault_rifle()) {}

    void update(float dt) {
        if (dt < 0.0f)
            throw std::invalid_argument("TPSWeaponState::update: dt must be >= 0");
        if (fire_cooldown_ > 0.0f) {
            fire_cooldown_ -= dt;
            if (fire_cooldown_ < 0.0f) fire_cooldown_ = 0.0f;
        }
        if (reloading_) {
            reload_timer_ -= dt;
            if (reload_timer_ <= 0.0f) {
                int needed = config_.magazine_size - config_.current_ammo;
                int available = (config_.reserve_ammo < needed) ? config_.reserve_ammo : needed;
                config_.current_ammo += available;
                config_.reserve_ammo -= available;
                reloading_ = false;
                reload_timer_ = 0.0f;
            }
        }
    }

    bool can_fire() const {
        return !reloading_ && fire_cooldown_ <= 0.0f && config_.current_ammo > 0;
    }

    /** Fire the weapon. Returns true if shot was fired. */
    bool fire() {
        if (!can_fire()) return false;
        config_.current_ammo--;
        fire_cooldown_ = 1.0f / config_.fire_rate;
        return true;
    }

    void start_reload() {
        if (reloading_) return;
        if (config_.current_ammo == config_.magazine_size) return;
        if (config_.reserve_ammo <= 0) return;
        reloading_ = true;
        reload_timer_ = config_.reload_time;
    }

    void set_ads(bool ads) { ads_ = ads; }
    bool is_ads() const { return ads_; }
    bool is_reloading() const { return reloading_; }

    float effective_spread() const {
        float base = config_.spread_angle;
        if (ads_) return base * config_.ads_spread_mult;
        return base + config_.hip_fire_penalty;
    }

    float effective_zoom() const {
        return ads_ ? config_.ads_zoom : 1.0f;
    }

    /** Generate fire directions using quaternion spread. */
    std::vector<math::Vec3> compute_directions(
            const math::Vec3& forward, const math::Vec3& up, core::Rng& rng) const {
        std::vector<math::Vec3> dirs;
        float spread = effective_spread();

        for (int i = 0; i < config_.pellet_count; ++i) {
            if (spread <= 0.0f && config_.pellet_count == 1) {
                dirs.push_back(forward);
            } else {
                float pitch = random_range(rng, -spread, spread);
                float yaw = random_range(rng, -spread, spread);
                math::Vec3 right = forward.cross(up).normalized();
                math::Quaternion q_p = math::Quaternion::from_axis_angle(right, pitch);
                math::Quaternion q_y = math::Quaternion::from_axis_angle(up, yaw);
                dirs.push_back((q_y * q_p).normalized().rotate(forward).normalized());
            }
        }
        return dirs;
    }

    const TPSWeaponConfig& config() const { return config_; }
    int ammo() const { return config_.current_ammo; }
    int reserve() const { return config_.reserve_ammo; }

    float reload_progress() const {
        if (!reloading_) return 0.0f;
        return 1.0f - (reload_timer_ / config_.reload_time);
    }

    void add_reserve_ammo(int amount) {
        if (amount < 0)
            throw std::invalid_argument("TPSWeaponState::add_reserve_ammo: amount must be >= 0");
        config_.reserve_ammo += amount;
    }

private:
    static float random_range(core::Rng& rng, float lo, float hi) {
        return rng.random_float(lo, hi);
    }

    // ── Weapon Factories ─────────────────────────────────────────────────

public:
    static TPSWeaponConfig make_assault_rifle() {
        return {
            TPSWeaponType::AssaultRifle,
            DamageCategory::Ballistic,
            22.0f,   // damage
            8.0f,    // fire_rate (shots/sec)
            80.0f,   // projectile_speed
            0.03f,   // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            80.0f,   // range
            false,   // piercing
            0.025f,  // recoil
            30,      // magazine_size
            30,      // current_ammo
            180,     // reserve_ammo
            2.0f,    // reload_time
            2.0f,    // ads_zoom
            0.3f,    // ads_spread_mult
            0.02f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_combat_shotgun() {
        return {
            TPSWeaponType::CombatShotgun,
            DamageCategory::Ballistic,
            12.0f,   // damage per pellet
            1.5f,    // fire_rate
            50.0f,   // projectile_speed
            0.18f,   // spread_angle
            8,       // pellet_count
            0.0f,    // splash_radius
            25.0f,   // range
            false,   // piercing
            0.10f,   // recoil
            6,       // magazine_size
            6,       // current_ammo
            36,      // reserve_ammo
            2.5f,    // reload_time
            1.5f,    // ads_zoom
            0.6f,    // ads_spread_mult
            0.04f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_plasma_caster() {
        return {
            TPSWeaponType::PlasmaCaster,
            DamageCategory::Energy,
            65.0f,   // damage
            1.2f,    // fire_rate
            30.0f,   // projectile_speed
            0.02f,   // spread_angle
            1,       // pellet_count
            3.5f,    // splash_radius
            60.0f,   // range
            false,   // piercing
            0.06f,   // recoil
            8,       // magazine_size
            8,       // current_ammo
            32,      // reserve_ammo
            3.0f,    // reload_time
            2.5f,    // ads_zoom
            0.4f,    // ads_spread_mult
            0.01f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_marksman_rifle() {
        return {
            TPSWeaponType::MarksmanRifle,
            DamageCategory::Ballistic,
            85.0f,   // damage
            1.0f,    // fire_rate
            0.0f,    // hitscan
            0.005f,  // spread_angle (very accurate)
            1,       // pellet_count
            0.0f,    // splash_radius
            150.0f,  // range
            true,    // piercing
            0.08f,   // recoil
            5,       // magazine_size
            5,       // current_ammo
            30,      // reserve_ammo
            2.8f,    // reload_time
            4.0f,    // ads_zoom
            0.1f,    // ads_spread_mult
            0.06f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_submachine_gun() {
        return {
            TPSWeaponType::SubmachineGun,
            DamageCategory::Ballistic,
            12.0f,   // damage
            14.0f,   // fire_rate
            70.0f,   // projectile_speed
            0.06f,   // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            45.0f,   // range
            false,   // piercing
            0.015f,  // recoil
            40,      // magazine_size
            40,      // current_ammo
            200,     // reserve_ammo
            1.8f,    // reload_time
            1.5f,    // ads_zoom
            0.5f,    // ads_spread_mult
            0.01f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_grenade_launcher() {
        return {
            TPSWeaponType::GrenadeLauncher,
            DamageCategory::Explosive,
            70.0f,   // damage
            0.8f,    // fire_rate
            25.0f,   // projectile_speed
            0.01f,   // spread_angle
            1,       // pellet_count
            5.0f,    // splash_radius
            50.0f,   // range
            false,   // piercing
            0.05f,   // recoil
            4,       // magazine_size
            4,       // current_ammo
            16,      // reserve_ammo
            3.5f,    // reload_time
            1.8f,    // ads_zoom
            0.5f,    // ads_spread_mult
            0.02f,   // hip_fire_penalty
            0        // chain_targets
        };
    }

    static TPSWeaponConfig make_tesla_coil() {
        return {
            TPSWeaponType::TeslaCoil,
            DamageCategory::Energy,
            30.0f,   // damage
            3.0f,    // fire_rate
            0.0f,    // hitscan (instant beam)
            0.0f,    // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            35.0f,   // range
            false,   // piercing
            0.02f,   // recoil
            20,      // magazine_size (charge units)
            20,      // current_ammo
            60,      // reserve_ammo
            2.2f,    // reload_time
            2.0f,    // ads_zoom
            0.3f,    // ads_spread_mult
            0.0f,    // hip_fire_penalty
            3        // chain_targets
        };
    }
};

// ── Weapon Loadout Manager ───────────────────────────────────────────────────

class TPSLoadout {
    std::vector<TPSWeaponState> weapons_;
    int current_ = 0;

public:
    TPSLoadout() {
        weapons_.emplace_back(TPSWeaponState::make_assault_rifle());
        weapons_.emplace_back(TPSWeaponState::make_combat_shotgun());
    }

    void add_weapon(TPSWeaponConfig cfg) {
        weapons_.emplace_back(cfg);
    }

    void update(float dt) {
        for (auto& w : weapons_) w.update(dt);
    }

    TPSWeaponState& current() {
        if (weapons_.empty())
            throw std::logic_error("TPSLoadout::current: loadout is empty");
        return weapons_[current_];
    }

    const TPSWeaponState& current() const {
        if (weapons_.empty())
            throw std::logic_error("TPSLoadout::current: loadout is empty");
        return weapons_[current_];
    }

    void switch_weapon(int index) {
        if (index >= 0 && index < static_cast<int>(weapons_.size())) {
            current_ = index;
        }
    }

    void next_weapon() {
        if (weapons_.empty()) return;
        current_ = (current_ + 1) % static_cast<int>(weapons_.size());
    }

    void prev_weapon() {
        if (weapons_.empty()) return;
        int n = static_cast<int>(weapons_.size());
        current_ = (current_ - 1 + n) % n;
    }

    int weapon_count() const { return static_cast<int>(weapons_.size()); }
    int current_index() const { return current_; }
};

inline const char* weapon_name(TPSWeaponType type) {
    switch (type) {
        case TPSWeaponType::AssaultRifle:    return "M2 Assault Rifle";
        case TPSWeaponType::CombatShotgun:   return "Trench Sweeper";
        case TPSWeaponType::PlasmaCaster:    return "Plasma Caster Mk.IV";
        case TPSWeaponType::MarksmanRifle:   return "Deadshot Carbine";
        case TPSWeaponType::SubmachineGun:   return "Iron Hornet SMG";
        case TPSWeaponType::GrenadeLauncher: return "Thumper GL-40";
        case TPSWeaponType::TeslaCoil:       return "Tesla Arc Projector";
    }
    return "Unknown";
}

} // namespace tps
} // namespace game
} // namespace qe
