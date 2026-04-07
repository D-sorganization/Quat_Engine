#pragma once
/**
 * @file LockOnSystem.h
 * @brief Lock-on targeting system with aim wobble for third-person combat.
 *
 * Left trigger locks onto the nearest valid target. While locked:
 *   - Camera orbits around the player, keeping target visible
 *   - Aim reticle wobbles using Perlin-like noise driven by quaternion rotation
 *   - Wobble intensity varies by character class (lock_on_stability)
 *   - Wobble decreases with ADS (aim down sights)
 *   - Target switching via right stick flick
 *
 * The wobble system uses quaternion axis-angle rotation to create
 * organic, non-repetitive aim drift that rewards skill while keeping
 * combat dynamic and preventing trivial target acquisition.
 */

#include "../../core/Rng.h"
#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace qe {
namespace game {
namespace tps {

// ── Lock-On Config ───────────────────────────────────────────────────────────

struct LockOnConfig {
    float max_lock_range;       // Max distance to lock onto target
    float lock_angle;           // Max angle from center to acquire target (radians)
    float wobble_frequency;     // Base oscillation speed
    float wobble_amplitude;     // Base wobble magnitude (radians)
    float ads_wobble_reduction; // Multiplier when ADS (0.3 = 70% less wobble)
    float stability_influence;  // How much class stability affects wobble
    float target_switch_cooldown;
    float lock_break_distance;  // Distance at which lock breaks

    void check_invariants() const {
        assert(max_lock_range > 0.0f);
        assert(lock_angle > 0.0f);
        assert(wobble_frequency > 0.0f);
        assert(wobble_amplitude > 0.0f);
        assert(ads_wobble_reduction > 0.0f && ads_wobble_reduction <= 1.0f);
    }
};

inline LockOnConfig default_lock_on_config() {
    return {
        40.0f,   // max_lock_range
        0.8f,    // lock_angle (~45 degrees)
        2.5f,    // wobble_frequency
        0.04f,   // wobble_amplitude
        0.3f,    // ads_wobble_reduction
        1.5f,    // stability_influence
        0.3f,    // target_switch_cooldown
        50.0f    // lock_break_distance
    };
}

// ── Lock-On Target ───────────────────────────────────────────────────────────

struct LockOnTarget {
    int entity_id;
    math::Vec3 position;
    bool alive;
    float priority;  // Lower = better target (based on distance + angle)
};

// ── Lock-On State ────────────────────────────────────────────────────────────

class LockOnState {
    LockOnConfig config_;
    bool locked_ = false;
    int locked_target_id_ = -1;
    math::Vec3 locked_target_pos_;
    float wobble_time_ = 0.0f;
    float switch_cooldown_ = 0.0f;
    qe::core::Rng rng_{77777};

    // Wobble state — two independent oscillators for organic motion
    float wobble_phase_x_ = 0.0f;
    float wobble_phase_y_ = 0.0f;
    float wobble_drift_x_ = 0.0f;
    float wobble_drift_y_ = 0.0f;

public:
    explicit LockOnState(LockOnConfig cfg) : config_(cfg) {
        config_.check_invariants();
    }

    LockOnState() : config_(default_lock_on_config()) {}

    // ── Queries ──────────────────────────────────────────────────────

    bool is_locked() const { return locked_; }
    int locked_target_id() const { return locked_target_id_; }
    const math::Vec3& locked_target_position() const { return locked_target_pos_; }

    /** Get the current aim direction with wobble applied.
     *  @param base_direction  The perfect aim direction (player -> target)
     *  @param player_stability  Character class lock_on_stability (0-1, lower = less wobble)
     *  @param is_ads  Whether player is aiming down sights
     *  @return  Wobble-adjusted aim direction
     */
    math::Vec3 get_wobbled_aim(const math::Vec3& base_direction,
                                float player_stability,
                                bool is_ads) const {
        if (!locked_) return base_direction;

        float amp = config_.wobble_amplitude * (1.0f + player_stability * config_.stability_influence);
        if (is_ads) amp *= config_.ads_wobble_reduction;

        // Apply wobble via quaternion rotation around perpendicular axes
        math::Vec3 right = base_direction.cross(math::Vec3::up());
        if (right.length_squared() < 1e-6f) {
            right = math::Vec3::right();
        }
        right = right.normalized();
        math::Vec3 up = right.cross(base_direction).normalized();

        math::Quaternion q_x = math::Quaternion::from_axis_angle(right, wobble_drift_x_ * amp);
        math::Quaternion q_y = math::Quaternion::from_axis_angle(up, wobble_drift_y_ * amp);
        math::Quaternion wobble = (q_x * q_y).normalized();

        return wobble.rotate(base_direction).normalized();
    }

    /** Get wobble offset in screen-space units (for reticle rendering). */
    math::Vec3 get_wobble_offset() const {
        return math::Vec3(wobble_drift_x_, wobble_drift_y_, 0.0f);
    }

    // ── Commands ─────────────────────────────────────────────────────

    /** Attempt to lock onto the best target from the candidate list.
     *  @param player_pos  Player world position
     *  @param player_forward  Player facing direction
     *  @param candidates  Available targets
     *  @return true if a target was locked
     */
    bool try_lock_on(const math::Vec3& player_pos,
                     const math::Vec3& player_forward,
                     const std::vector<LockOnTarget>& candidates) {
        if (candidates.empty()) return false;

        float best_priority = 999999.0f;
        int best_id = -1;
        math::Vec3 best_pos;

        for (const auto& t : candidates) {
            if (!t.alive) continue;

            math::Vec3 to_target = t.position - player_pos;
            float dist = to_target.length();
            if (dist > config_.max_lock_range || dist < 0.1f) continue;

            float dot = player_forward.dot(to_target.normalized());
            float angle = std::acos(std::min(1.0f, std::max(-1.0f, dot)));
            if (angle > config_.lock_angle) continue;

            float priority = dist * 0.5f + angle * 20.0f;
            if (priority < best_priority) {
                best_priority = priority;
                best_id = t.entity_id;
                best_pos = t.position;
            }
        }

        if (best_id >= 0) {
            locked_ = true;
            locked_target_id_ = best_id;
            locked_target_pos_ = best_pos;
            wobble_time_ = 0.0f;
            return true;
        }
        return false;
    }

    /** Release lock-on. */
    void release_lock() {
        locked_ = false;
        locked_target_id_ = -1;
        wobble_drift_x_ = 0.0f;
        wobble_drift_y_ = 0.0f;
    }

    /** Switch to next valid target. */
    bool switch_target(const math::Vec3& player_pos,
                       const math::Vec3& switch_direction,
                       const std::vector<LockOnTarget>& candidates) {
        if (!locked_ || switch_cooldown_ > 0.0f) return false;

        float best_score = -999.0f;
        int best_id = -1;
        math::Vec3 best_pos;

        for (const auto& t : candidates) {
            if (!t.alive || t.entity_id == locked_target_id_) continue;

            math::Vec3 to_target = t.position - player_pos;
            float dist = to_target.length();
            if (dist > config_.max_lock_range) continue;

            float directional = switch_direction.dot(to_target.normalized());
            if (directional <= 0.0f) continue;

            float score = directional / (dist * 0.1f + 1.0f);
            if (score > best_score) {
                best_score = score;
                best_id = t.entity_id;
                best_pos = t.position;
            }
        }

        if (best_id >= 0) {
            locked_target_id_ = best_id;
            locked_target_pos_ = best_pos;
            switch_cooldown_ = config_.target_switch_cooldown;
            return true;
        }
        return false;
    }

    /** Update the locked target position (call each frame). */
    void update_target_position(const math::Vec3& new_pos) {
        locked_target_pos_ = new_pos;
    }

    /** Check if lock should break (target died, out of range, etc). */
    bool should_break_lock(const math::Vec3& player_pos, bool target_alive) const {
        if (!locked_) return false;
        if (!target_alive) return true;
        float dist = player_pos.distance_to(locked_target_pos_);
        return dist > config_.lock_break_distance;
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);

        if (switch_cooldown_ > 0.0f) {
            switch_cooldown_ -= dt;
            if (switch_cooldown_ < 0.0f) switch_cooldown_ = 0.0f;
        }

        if (!locked_) return;

        wobble_time_ += dt;

        // Dual-frequency wobble for organic motion
        float freq = config_.wobble_frequency;
        wobble_phase_x_ += dt * freq * 1.0f;
        wobble_phase_y_ += dt * freq * 1.37f;  // Irrational ratio avoids repetition

        // Layered sinusoidal wobble (pseudo-Perlin)
        wobble_drift_x_ = std::sin(wobble_phase_x_)
                        + 0.5f * std::sin(wobble_phase_x_ * 2.3f + 1.7f)
                        + 0.25f * std::sin(wobble_phase_x_ * 4.1f + 3.2f);

        wobble_drift_y_ = std::sin(wobble_phase_y_)
                        + 0.5f * std::sin(wobble_phase_y_ * 1.9f + 0.8f)
                        + 0.25f * std::sin(wobble_phase_y_ * 3.7f + 2.1f);

        // Normalize to [-1, 1] range
        wobble_drift_x_ /= 1.75f;
        wobble_drift_y_ /= 1.75f;
    }

    const LockOnConfig& config() const { return config_; }
};

} // namespace tps
} // namespace game
} // namespace qe
