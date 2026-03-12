#pragma once
/**
 * @file Weapons.h
 * @brief Weapon system with multiple weapon types and quaternion-based spread.
 *
 * Provides a WeaponManager that handles weapon switching, ammo, reloading,
 * cooldowns, and fire direction computation. The spread system uses quaternion
 * rotation to generate pellet directions within a cone, showcasing practical
 * quaternion usage for gameplay mechanics.
 *
 * Weapon types:
 *   - Pistol:         Reliable sidearm, infinite ammo, moderate stats
 *   - Shotgun:        5-pellet spread, devastating at close range
 *   - RailGun:        Hitscan piercing beam, massive single-shot damage
 *   - RocketLauncher: Slow projectile with splash damage radius
 *   - MiniGun:        Extreme fire rate, slight inaccuracy, huge magazine
 */

#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <cstdint>
#include <vector>

namespace qe {
namespace game {

enum class WeaponType {
    Pistol,          // Default: single shot, moderate damage, moderate fire rate
    Shotgun,         // Spread: fires 5 pellets in a cone, slow fire rate, close range
    RailGun,         // Hitscan only: instant, huge damage, very slow fire rate, piercing
    RocketLauncher,  // Slow projectile, area damage, splash radius
    MiniGun,         // Very fast fire rate, low damage per shot, slight inaccuracy
};

struct WeaponConfig {
    WeaponType type;
    float fire_rate;          // Seconds between shots
    float damage;             // Per projectile
    float projectile_speed;   // 0 = hitscan only
    float spread_angle;       // Radians, for shotgun/minigun spread
    int pellet_count;         // Projectiles per shot (shotgun=5, else 1)
    float splash_radius;      // 0 = no splash
    float range;              // Max effective range
    bool piercing;            // Goes through targets?
    float recoil;             // Camera kick (radians)
    int max_ammo;             // -1 = infinite
    int ammo;                 // Current ammo
    float reload_time;        // Seconds to reload
};

class WeaponManager {
    std::vector<WeaponConfig> weapons_;
    int current_weapon_ = 0;
    float cooldown_ = 0.0f;
    float reload_timer_ = 0.0f;
    bool reloading_ = false;

public:
    WeaponManager() { init_loadout(); }

    void init_loadout() {
        weapons_.clear();
        weapons_.push_back(make_pistol());
        weapons_.push_back(make_shotgun());
        weapons_.push_back(make_railgun());
        weapons_.push_back(make_rocket_launcher());
        weapons_.push_back(make_minigun());
        current_weapon_ = 0;
    }

    void update(float dt) {
        if (cooldown_ > 0.0f) {
            cooldown_ -= dt;
            if (cooldown_ < 0.0f) cooldown_ = 0.0f;
        }
        if (reloading_) {
            reload_timer_ -= dt;
            if (reload_timer_ <= 0.0f) {
                reload_timer_ = 0.0f;
                reloading_ = false;
                weapons_[current_weapon_].ammo = weapons_[current_weapon_].max_ammo;
            }
        }
    }

    bool can_fire() const {
        if (cooldown_ > 0.0f) return false;
        if (reloading_) return false;
        const auto& wpn = weapons_[current_weapon_];
        if (wpn.max_ammo != -1 && wpn.ammo <= 0) return false;
        return true;
    }

    void fire() {
        if (!can_fire()) return;
        auto& wpn = weapons_[current_weapon_];
        if (wpn.max_ammo != -1) {
            wpn.ammo--;
        }
        cooldown_ = wpn.fire_rate;
    }

    void reload() {
        auto& wpn = weapons_[current_weapon_];
        if (wpn.max_ammo == -1) return;           // Infinite ammo, no reload
        if (wpn.ammo == wpn.max_ammo) return;     // Already full
        if (reloading_) return;                    // Already reloading
        reloading_ = true;
        reload_timer_ = wpn.reload_time;
    }

    void switch_weapon(int index) {
        if (index < 0 || index >= static_cast<int>(weapons_.size())) return;
        current_weapon_ = index;
        cooldown_ = 0.0f;
        reloading_ = false;
        reload_timer_ = 0.0f;
    }

    void next_weapon() {
        switch_weapon((current_weapon_ + 1) % static_cast<int>(weapons_.size()));
    }

    void prev_weapon() {
        int count = static_cast<int>(weapons_.size());
        switch_weapon((current_weapon_ - 1 + count) % count);
    }

    const WeaponConfig& current() const { return weapons_[current_weapon_]; }
    int current_index() const { return current_weapon_; }
    int weapon_count() const { return static_cast<int>(weapons_.size()); }
    bool is_reloading() const { return reloading_; }

    float reload_progress() const {
        if (!reloading_) return 0.0f;
        float total = weapons_[current_weapon_].reload_time;
        if (total <= 0.0f) return 0.0f;
        return 1.0f - (reload_timer_ / total);
    }

    float cooldown_progress() const {
        if (cooldown_ <= 0.0f) return 0.0f;
        float total = weapons_[current_weapon_].fire_rate;
        if (total <= 0.0f) return 0.0f;
        return cooldown_ / total;
    }

    /** Generate spread directions using quaternion rotation.
     *  Returns a vector of directions, one per pellet.
     */
    std::vector<math::Vec3> compute_fire_directions(
            const math::Vec3& forward, const math::Vec3& up) {
        const auto& wpn = current();
        std::vector<math::Vec3> dirs;

        for (int i = 0; i < wpn.pellet_count; ++i) {
            if (wpn.spread_angle <= 0.0f && wpn.pellet_count == 1) {
                dirs.push_back(forward);
            } else {
                // Generate random direction within cone using quaternion rotation
                float pitch = random_float(-wpn.spread_angle, wpn.spread_angle);
                float yaw = random_float(-wpn.spread_angle, wpn.spread_angle);
                math::Vec3 right = forward.cross(up).normalized();
                math::Quaternion q_pitch = math::Quaternion::from_axis_angle(right, pitch);
                math::Quaternion q_yaw = math::Quaternion::from_axis_angle(up, yaw);
                dirs.push_back((q_yaw * q_pitch).normalized().rotate(forward).normalized());
            }
        }
        return dirs;
    }

private:
    static WeaponConfig make_pistol() {
        return {
            WeaponType::Pistol,
            0.2f,    // fire_rate
            25.0f,   // damage
            50.0f,   // projectile_speed
            0.0f,    // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            100.0f,  // range
            false,   // piercing
            0.02f,   // recoil
            -1,      // max_ammo (infinite)
            -1,      // ammo (infinite)
            0.0f     // reload_time
        };
    }

    static WeaponConfig make_shotgun() {
        return {
            WeaponType::Shotgun,
            0.8f,    // fire_rate
            15.0f,   // damage
            40.0f,   // projectile_speed
            0.15f,   // spread_angle
            5,       // pellet_count
            0.0f,    // splash_radius
            30.0f,   // range
            false,   // piercing
            0.08f,   // recoil
            8,       // max_ammo
            8,       // ammo
            2.0f     // reload_time
        };
    }

    static WeaponConfig make_railgun() {
        return {
            WeaponType::RailGun,
            1.5f,    // fire_rate
            150.0f,  // damage
            0.0f,    // projectile_speed (hitscan)
            0.0f,    // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            200.0f,  // range
            true,    // piercing
            0.12f,   // recoil
            5,       // max_ammo
            5,       // ammo
            2.5f     // reload_time
        };
    }

    static WeaponConfig make_rocket_launcher() {
        return {
            WeaponType::RocketLauncher,
            1.0f,    // fire_rate
            80.0f,   // damage
            20.0f,   // projectile_speed
            0.0f,    // spread_angle
            1,       // pellet_count
            4.0f,    // splash_radius
            80.0f,   // range
            false,   // piercing
            0.06f,   // recoil
            6,       // max_ammo
            6,       // ammo
            3.0f     // reload_time
        };
    }

    static WeaponConfig make_minigun() {
        return {
            WeaponType::MiniGun,
            0.05f,   // fire_rate
            8.0f,    // damage
            60.0f,   // projectile_speed
            0.05f,   // spread_angle
            1,       // pellet_count
            0.0f,    // splash_radius
            60.0f,   // range
            false,   // piercing
            0.01f,   // recoil
            100,     // max_ammo
            100,     // ammo
            4.0f     // reload_time
        };
    }

    mutable uint32_t rng_ = 99999;

    float random_float(float min, float max) {
        // Simple xorshift32 PRNG
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        float t = static_cast<float>(rng_ & 0xFFFF) / 65535.0f;
        return min + t * (max - min);
    }
};

} // namespace game
} // namespace qe
