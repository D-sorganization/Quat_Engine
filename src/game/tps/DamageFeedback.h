#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file DamageFeedback.h
 * @brief Visual feedback state management for combat events.
 *
 * Manages screen-space and world-space feedback effects:
 *   - Screen shake on heavy hits
 *   - Damage direction indicator (red flash)
 *   - Hit markers (white flash for hit, red for kill)
 *   - Floating damage numbers
 *   - Low health vignette pulse
 *   - Critical hit emphasis
 *
 * Pure state management — rendering reads this data each frame.
 * Decoupled from any specific rendering backend.
 */

#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>

namespace qe {
namespace game {
namespace tps {

// ── Damage Number ────────────────────────────────────────────────────────────

struct DamageNumber {
    math::Vec3 world_pos;
    float value;
    bool critical;
    float age = 0.0f;
    float lifetime = 1.2f;
    float y_velocity = 2.0f;

    bool alive() const { return age < lifetime; }
    float alpha() const {
        float t = age / lifetime;
        return t < 0.3f ? 1.0f : 1.0f - ((t - 0.3f) / 0.7f);
    }
    math::Vec3 display_pos() const {
        return math::Vec3(world_pos.x, world_pos.y + y_velocity * age, world_pos.z);
    }
};

// ── Hit Marker ───────────────────────────────────────────────────────────────

struct HitMarker {
    float timer = 0.0f;
    float duration = 0.15f;
    bool is_kill = false;
    bool is_critical = false;

    bool active() const { return timer < duration; }
    float intensity() const { return 1.0f - (timer / duration); }
};

// ── Damage Direction Indicator ───────────────────────────────────────────────

struct DamageIndicator {
    float angle;      // Radians, direction damage came from
    float timer = 0.0f;
    float duration = 0.8f;

    bool active() const { return timer < duration; }
    float intensity() const { return 1.0f - (timer / duration); }
};

// ── Screen Shake ─────────────────────────────────────────────────────────────

struct ScreenShake {
    float intensity = 0.0f;
    float decay_rate = 8.0f;
    float frequency = 25.0f;
    float timer = 0.0f;

    math::Vec3 offset() const {
        if (intensity < 0.001f) return math::Vec3::zero();
        float x = std::sin(timer * frequency) * intensity;
        float y = std::cos(timer * frequency * 1.3f) * intensity * 0.7f;
        return math::Vec3(x, y, 0.0f);
    }
};

// ── Feedback Manager ─────────────────────────────────────────────────────────

class DamageFeedbackSystem {
    std::vector<DamageNumber> damage_numbers_;
    std::vector<DamageIndicator> damage_indicators_;
    HitMarker hit_marker_;
    ScreenShake screen_shake_;
    float low_health_pulse_ = 0.0f;
    float low_health_timer_ = 0.0f;

    static constexpr int MAX_DAMAGE_NUMBERS = 20;
    static constexpr int MAX_INDICATORS = 4;

public:
    // ── Queries ──────────────────────────────────────────────────────

    const std::vector<DamageNumber>& damage_numbers() const { return damage_numbers_; }
    const std::vector<DamageIndicator>& damage_indicators() const { return damage_indicators_; }
    const HitMarker& hit_marker() const { return hit_marker_; }
    const ScreenShake& screen_shake() const { return screen_shake_; }
    float low_health_pulse() const { return low_health_pulse_; }

    math::Vec3 get_shake_offset() const { return screen_shake_.offset(); }
    bool has_active_hit_marker() const { return hit_marker_.active(); }

    // ── Events ───────────────────────────────────────────────────────

    /** Called when player deals damage to an enemy. */
    void on_hit(const math::Vec3& hit_pos, float damage, bool critical, bool killed) {
        // Damage number
        DamageNumber dn;
        dn.world_pos = hit_pos;
        dn.value = damage;
        dn.critical = critical;
        damage_numbers_.push_back(dn);
        if (static_cast<int>(damage_numbers_.size()) > MAX_DAMAGE_NUMBERS) {
            damage_numbers_.erase(damage_numbers_.begin());
        }

        // Hit marker
        hit_marker_.timer = 0.0f;
        hit_marker_.is_kill = killed;
        hit_marker_.is_critical = critical;
        hit_marker_.duration = killed ? 0.3f : (critical ? 0.2f : 0.15f);

        // Screen shake on heavy hits
        if (critical) {
            add_screen_shake(0.08f);
        } else if (killed) {
            add_screen_shake(0.05f);
        }
    }

    /** Called when player takes damage. */
    void on_player_hit(float damage, const math::Vec3& damage_source,
                        const math::Vec3& player_pos, const math::Vec3& player_forward) {
        // Screen shake proportional to damage
        float shake = std::min(0.15f, damage * 0.002f);
        add_screen_shake(shake);

        // Damage direction
        math::Vec3 to_source = damage_source - player_pos;
        float angle = std::atan2(to_source.x, to_source.z);
        float player_yaw = std::atan2(player_forward.x, player_forward.z);
        float relative = angle - player_yaw;

        DamageIndicator di;
        di.angle = relative;
        damage_indicators_.push_back(di);
        if (static_cast<int>(damage_indicators_.size()) > MAX_INDICATORS) {
            damage_indicators_.erase(damage_indicators_.begin());
        }
    }

    /** Called when player performs a parry. */
    void on_parry() {
        add_screen_shake(0.03f);
    }

    /** Called when player lands a ground slam. */
    void on_ground_slam() {
        add_screen_shake(0.12f);
    }

    void add_screen_shake(float intensity) {
        screen_shake_.intensity += intensity;
        if (screen_shake_.intensity > 0.2f) {
            screen_shake_.intensity = 0.2f;
        }
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt, float player_health_fraction) {
        assert(dt >= 0.0f);

        // Damage numbers
        for (auto& dn : damage_numbers_) dn.age += dt;
        damage_numbers_.erase(
            std::remove_if(damage_numbers_.begin(), damage_numbers_.end(),
                [](const DamageNumber& d) { return !d.alive(); }),
            damage_numbers_.end());

        // Damage indicators
        for (auto& di : damage_indicators_) di.timer += dt;
        damage_indicators_.erase(
            std::remove_if(damage_indicators_.begin(), damage_indicators_.end(),
                [](const DamageIndicator& d) { return !d.active(); }),
            damage_indicators_.end());

        // Hit marker
        if (hit_marker_.active()) hit_marker_.timer += dt;

        // Screen shake
        screen_shake_.timer += dt;
        screen_shake_.intensity -= screen_shake_.decay_rate * dt;
        if (screen_shake_.intensity < 0.0f) screen_shake_.intensity = 0.0f;

        // Low health pulse
        if (player_health_fraction < 0.3f) {
            low_health_timer_ += dt;
            low_health_pulse_ = (1.0f - player_health_fraction / 0.3f) *
                (0.3f + 0.2f * std::sin(low_health_timer_ * 4.0f));
        } else {
            low_health_pulse_ = 0.0f;
            low_health_timer_ = 0.0f;
        }
    }

    void clear() {
        damage_numbers_.clear();
        damage_indicators_.clear();
        hit_marker_.timer = hit_marker_.duration;
        screen_shake_.intensity = 0.0f;
        low_health_pulse_ = 0.0f;
    }
};

} // namespace tps
} // namespace game
} // namespace qe
