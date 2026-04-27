#pragma once
/**
 * @file PowerUp.h
 * @brief Power-up system with quaternion-animated collectibles.
 *
 * Provides floating, spinning power-up pickups that use quaternion
 * composition for smooth rotation animation. Includes a manager that
 * handles spawning, collection, and active effect tracking.
 *
 * Power-up types: RapidFire, TripleShot, DamageBoost, SlowMotion,
 * Shield, and ScoreMultiplier.
 */

#include "../core/Rng.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <random>

namespace qe {
namespace game {

// ── Power-Up Type ───────────────────────────────────────────────────────────

enum class PowerUpType {
    RapidFire,       // Doubles fire rate for 10s
    TripleShot,      // Fires 3 projectiles in a spread for 10s
    DamageBoost,     // 2x damage for 8s
    SlowMotion,      // Slows enemies to 50% speed for 6s
    Shield,          // Absorbs next 3 hits
    ScoreMultiplier  // 3x score for 15s
};

// ── Power-Up Config ─────────────────────────────────────────────────────────

struct PowerUpConfig {
    float duration;       // How long the effect lasts
    float spawn_chance;   // Probability of spawning (0-1)
    math::Vec3 color;     // Visual color for the pickup
    float value;          // Multiplier or count
};

// ── Power-Up (World Pickup) ─────────────────────────────────────────────────

struct PowerUp {
    PowerUpType type;
    math::Vec3 position;
    math::Quaternion rotation = math::Quaternion::identity();
    float bob_phase = 0.0f;    // For floating bob animation
    float spin_speed = 2.0f;   // Radians/sec around Y
    float bob_speed = 3.0f;
    float bob_amplitude = 0.3f;
    float lifetime = 15.0f;    // Despawns after this time
    float age = 0.0f;
    bool collected = false;
    bool alive = true;
    float radius = 0.8f;       // Collection radius

    /** Quaternion-driven animation update. */
    void update(float dt) {
        if (!alive) return;
        age += dt;
        if (age >= lifetime) { alive = false; return; }

        // Spinning rotation using quaternion composition
        // Spin around Y axis with a slight tilt for visual interest
        math::Quaternion y_spin = math::Quaternion::from_axis_angle(
            math::Vec3::up(), age * spin_speed);
        math::Quaternion tilt = math::Quaternion::from_axis_angle(
            math::Vec3(1, 0, 0.5f).normalized(), std::sin(age * 1.5f) * 0.3f);
        rotation = (y_spin * tilt).normalized();

        bob_phase = age * bob_speed;
    }

    /** Get the display position (base + bob offset). */
    math::Vec3 display_position() const {
        return math::Vec3(position.x,
                          position.y + std::sin(bob_phase) * bob_amplitude,
                          position.z);
    }

    /** Check if player is within collection radius. */
    bool can_collect(const math::Vec3& player_pos) const {
        return alive && !collected &&
               player_pos.distance_to(position) < radius;
    }

    /** Mark as collected. */
    void collect() { collected = true; alive = false; }

    /** Visual: pulsing glow based on age (for shader). */
    float glow_intensity() const {
        return 0.5f + 0.5f * std::sin(age * 4.0f);
    }

    /** Flash faster when about to despawn (last 3 seconds). */
    float flash_rate() const {
        float remaining = lifetime - age;
        if (remaining < 3.0f) return 8.0f;
        return 0.0f;
    }
};

// ── Active Effect ───────────────────────────────────────────────────────────

struct ActiveEffect {
    PowerUpType type;
    float duration;
    float elapsed = 0.0f;
    float value;   // multiplier or count

    bool is_active() const { return elapsed < duration; }
    float remaining() const { return std::max(0.0f, duration - elapsed); }
    float progress() const { return duration > 0 ? elapsed / duration : 1.0f; }
    void update(float dt) { elapsed += dt; }
};

// ── Power-Up Manager ────────────────────────────────────────────────────────

class PowerUpManager {
    std::vector<PowerUp> pickups_;
    std::vector<ActiveEffect> effects_;
    std::random_device rd_;
    qe::core::Rng rng_{rd_()};

public:
    // --- Config per type ---

    static PowerUpConfig get_config(PowerUpType type) {
        switch (type) {
            case PowerUpType::RapidFire:
                return {10.0f, 0.15f, math::Vec3(1.0f, 1.0f, 0.2f), 0.5f};
            case PowerUpType::TripleShot:
                return {10.0f, 0.10f, math::Vec3(0.2f, 0.8f, 1.0f), 3.0f};
            case PowerUpType::DamageBoost:
                return {8.0f, 0.12f, math::Vec3(1.0f, 0.3f, 0.2f), 2.0f};
            case PowerUpType::SlowMotion:
                return {6.0f, 0.08f, math::Vec3(0.5f, 0.2f, 1.0f), 0.5f};
            case PowerUpType::Shield:
                return {20.0f, 0.10f, math::Vec3(0.3f, 1.0f, 0.3f), 3.0f};
            case PowerUpType::ScoreMultiplier:
                return {15.0f, 0.12f, math::Vec3(1.0f, 0.8f, 0.2f), 3.0f};
        }
        return {10.0f, 0.1f, math::Vec3(1.0f, 1.0f, 1.0f), 1.0f};
    }

    // --- Spawning ---

    /** Spawn a power-up at a position. */
    void spawn(PowerUpType type, const math::Vec3& position) {
        PowerUp p;
        p.type = type;
        p.position = position;
        pickups_.push_back(p);
    }

    /** Randomly spawn a power-up (called on enemy kill, uses spawn_chance). */
    void try_spawn_random(const math::Vec3& position) {
        // Roll against total spawn chance of all types
        static const PowerUpType all_types[] = {
            PowerUpType::RapidFire,
            PowerUpType::TripleShot,
            PowerUpType::DamageBoost,
            PowerUpType::SlowMotion,
            PowerUpType::Shield,
            PowerUpType::ScoreMultiplier
        };

        float roll = random_float(0.0f, 1.0f);
        float cumulative = 0.0f;

        for (auto t : all_types) {
            cumulative += get_config(t).spawn_chance;
            if (roll < cumulative) {
                spawn(t, position);
                return;
            }
        }
        // No spawn if roll exceeds cumulative probability
    }

    // --- Update ---

    /** Update all pickups and effects, removing expired ones. */
    void update(float dt) {
        // Update pickups
        for (auto& p : pickups_) {
            p.update(dt);
        }

        // Remove dead pickups
        pickups_.erase(
            std::remove_if(pickups_.begin(), pickups_.end(),
                           [](const PowerUp& p) { return !p.alive; }),
            pickups_.end());

        // Update effects
        for (auto& e : effects_) {
            e.update(dt);
        }

        // Remove expired effects
        effects_.erase(
            std::remove_if(effects_.begin(), effects_.end(),
                           [](const ActiveEffect& e) { return !e.is_active(); }),
            effects_.end());
    }

    // --- Collection ---

    /**
     * Check collection against player position.
     * Returns PowerUpType as int, or -1 if nothing collected.
     */
    int try_collect(const math::Vec3& player_pos) {
        for (auto& p : pickups_) {
            if (p.can_collect(player_pos)) {
                PowerUpType type = p.type;
                p.collect();
                activate_effect(type);
                return static_cast<int>(type);
            }
        }
        return -1;
    }

    // --- Query Active Effects ---

    bool has_effect(PowerUpType type) const {
        for (const auto& e : effects_) {
            if (e.type == type && e.is_active()) return true;
        }
        return false;
    }

    /**
     * Return the value of the first active effect matching @p type,
     * or @p default_val if no such effect is active.
     *
     * Eliminates duplicated iteration in get_fire_rate_multiplier(),
     * get_damage_multiplier(), get_score_multiplier(), and
     * get_enemy_speed_multiplier(). See D-sorganization/QuatEngine#93.
     */
    float get_effect_value(PowerUpType type, float default_val = 1.0f) const {
        for (const auto& e : effects_) {
            if (e.type == type && e.is_active())
                return e.value;
        }
        return default_val;
    }

    /** 1.0 normally, 0.5 with RapidFire (halved fire interval = double rate). */
    float get_fire_rate_multiplier() const {
        return get_effect_value(PowerUpType::RapidFire);
    }

    /** 1.0 normally, 2.0 with DamageBoost. */
    float get_damage_multiplier() const {
        return get_effect_value(PowerUpType::DamageBoost);
    }

    /** 1.0 normally, 3.0 with ScoreMultiplier. */
    float get_score_multiplier() const {
        return get_effect_value(PowerUpType::ScoreMultiplier);
    }

    /** 1.0 normally, 0.5 with SlowMotion. */
    float get_enemy_speed_multiplier() const {
        return get_effect_value(PowerUpType::SlowMotion);
    }

    /** 1 normally, 3 with TripleShot. */
    int get_triple_shot_count() const {
        for (const auto& e : effects_) {
            if (e.type == PowerUpType::TripleShot && e.is_active())
                return static_cast<int>(e.value);
        }
        return 1;
    }

    /** 0 normally, remaining hits with Shield. */
    int get_shield_hits() const {
        for (const auto& e : effects_) {
            if (e.type == PowerUpType::Shield && e.is_active())
                return static_cast<int>(e.value);
        }
        return 0;
    }

    /** Remove a shield hit. Returns true if shield absorbed it. */
    bool absorb_shield_hit() {
        for (auto& e : effects_) {
            if (e.type == PowerUpType::Shield && e.is_active()) {
                e.value -= 1.0f;
                if (e.value <= 0.0f) {
                    // Expire the shield immediately
                    e.elapsed = e.duration;
                }
                return true;
            }
        }
        return false;
    }

    // --- Accessors ---

    const std::vector<PowerUp>& pickups() const { return pickups_; }
    const std::vector<ActiveEffect>& effects() const { return effects_; }

    void clear() { pickups_.clear(); effects_.clear(); }

private:
    /** Random float in [min, max]. */
    float random_float(float min, float max) {
        return rng_.random_float(min, max);
    }

    /** Activate a power-up effect on the player. */
    void activate_effect(PowerUpType type) {
        auto cfg = get_config(type);
        ActiveEffect effect;
        effect.type = type;
        effect.duration = cfg.duration;
        effect.elapsed = 0.0f;
        effect.value = cfg.value;
        effects_.push_back(effect);
    }
};

} // namespace game
} // namespace qe
