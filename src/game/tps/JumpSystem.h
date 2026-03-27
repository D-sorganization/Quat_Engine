#pragma once
/**
 * @file JumpSystem.h
 * @brief Jump dynamics, gravity, and aerial state for TPS combat.
 *
 * Features:
 *   - Variable-height jumping (hold to jump higher)
 *   - Double jump for Recon class
 *   - Aerial momentum preservation
 *   - Ground slam from airborne state
 *   - Coyote time (grace period after leaving ledge)
 *   - Jump buffer (input registered before landing)
 *   - Gravity with terminal velocity
 *
 * Orthogonal to melee and weapon systems — aerial state modifies
 * available actions without coupling to specific implementations.
 */

#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>
#include <algorithm>

namespace qe {
namespace game {
namespace tps {

// ── Jump Config ──────────────────────────────────────────────────────────────

struct JumpConfig {
    float jump_force;           // Initial upward velocity
    float gravity;              // Downward acceleration
    float terminal_velocity;    // Max fall speed
    float air_control;          // Fraction of ground control while airborne (0-1)
    float coyote_time;          // Grace period after walking off ledge
    float jump_buffer_time;     // Pre-landing input buffer
    int max_jumps;              // 1 = single, 2 = double jump
    float double_jump_force;    // Force for second jump
    float ground_slam_speed;    // Downward speed for slam
    float ground_slam_radius;   // AoE radius on landing
    float ground_slam_damage;   // Damage dealt on slam

    void check_invariants() const {
        assert(jump_force > 0.0f);
        assert(gravity > 0.0f);
        assert(terminal_velocity > 0.0f);
        assert(air_control >= 0.0f && air_control <= 1.0f);
        assert(coyote_time >= 0.0f);
        assert(max_jumps >= 1);
    }
};

inline JumpConfig make_jump_config(float jump_height, int max_jumps = 1) {
    assert(jump_height > 0.0f);
    JumpConfig c{};
    // v = sqrt(2 * g * h) for desired peak height
    c.gravity = 18.0f;
    c.jump_force = std::sqrt(2.0f * c.gravity * jump_height);
    c.terminal_velocity = 30.0f;
    c.air_control = 0.6f;
    c.coyote_time = 0.12f;
    c.jump_buffer_time = 0.1f;
    c.max_jumps = max_jumps;
    c.double_jump_force = c.jump_force * 0.8f;
    c.ground_slam_speed = 25.0f;
    c.ground_slam_radius = 3.5f;
    c.ground_slam_damage = 50.0f;
    c.check_invariants();
    return c;
}

// ── Aerial State ─────────────────────────────────────────────────────────────

enum class AerialState {
    Grounded,
    Rising,
    Falling,
    DoubleJump,
    GroundSlam
};

// ── Jump State Machine ───────────────────────────────────────────────────────

class JumpState {
    JumpConfig config_;
    AerialState state_ = AerialState::Grounded;
    float vertical_velocity_ = 0.0f;
    float height_ = 0.0f;
    int jumps_used_ = 0;
    float coyote_timer_ = 0.0f;
    float jump_buffer_timer_ = 0.0f;
    bool jump_held_ = false;
    bool slam_landing_ = false;

public:
    explicit JumpState(JumpConfig cfg) : config_(cfg) {
        config_.check_invariants();
    }

    JumpState() : config_(make_jump_config(3.0f)) {}

    // ── Queries ──────────────────────────────────────────────────────

    AerialState state() const { return state_; }
    float height() const { return height_; }
    float vertical_velocity() const { return vertical_velocity_; }
    bool is_grounded() const { return state_ == AerialState::Grounded; }
    bool is_airborne() const { return state_ != AerialState::Grounded; }
    bool is_slamming() const { return state_ == AerialState::GroundSlam; }
    bool just_slam_landed() const { return slam_landing_; }
    int jumps_remaining() const { return config_.max_jumps - jumps_used_; }

    float air_control_factor() const {
        return is_airborne() ? config_.air_control : 1.0f;
    }

    const JumpConfig& config() const { return config_; }

    // ── Commands ─────────────────────────────────────────────────────

    /** Request a jump. Uses coyote time and jump buffer. */
    bool request_jump() {
        // Buffer the jump input
        jump_buffer_timer_ = config_.jump_buffer_time;

        if (can_jump()) {
            execute_jump();
            return true;
        }
        return false;
    }

    /** Release jump button (for variable height). */
    void release_jump() {
        jump_held_ = false;
        // Cut vertical velocity for short hops
        if (state_ == AerialState::Rising && vertical_velocity_ > 0.0f) {
            vertical_velocity_ *= 0.5f;
        }
    }

    /** Initiate ground slam from airborne state. */
    bool start_ground_slam() {
        if (!is_airborne()) return false;
        if (state_ == AerialState::GroundSlam) return false;
        state_ = AerialState::GroundSlam;
        vertical_velocity_ = -config_.ground_slam_speed;
        return true;
    }

    /** Notify that the character has landed on ground. */
    void land() {
        slam_landing_ = (state_ == AerialState::GroundSlam);
        state_ = AerialState::Grounded;
        vertical_velocity_ = 0.0f;
        height_ = 0.0f;
        jumps_used_ = 0;
        coyote_timer_ = config_.coyote_time;

        // Check jump buffer
        if (jump_buffer_timer_ > 0.0f) {
            execute_jump();
        }
    }

    /** Notify that the character walked off a ledge (no jump). */
    void leave_ground() {
        if (state_ != AerialState::Grounded) return;
        state_ = AerialState::Falling;
        coyote_timer_ = config_.coyote_time;
        // Don't count as a jump — coyote time allows first jump
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);
        slam_landing_ = false;

        // Tick timers
        if (coyote_timer_ > 0.0f) {
            coyote_timer_ -= dt;
            if (coyote_timer_ < 0.0f) coyote_timer_ = 0.0f;
        }
        if (jump_buffer_timer_ > 0.0f) {
            jump_buffer_timer_ -= dt;
            if (jump_buffer_timer_ < 0.0f) jump_buffer_timer_ = 0.0f;
        }

        if (state_ == AerialState::Grounded) return;

        // Apply gravity
        if (state_ != AerialState::GroundSlam) {
            vertical_velocity_ -= config_.gravity * dt;
        }

        // Terminal velocity
        if (vertical_velocity_ < -config_.terminal_velocity) {
            vertical_velocity_ = -config_.terminal_velocity;
        }

        // Update height
        height_ += vertical_velocity_ * dt;

        // Transition Rising -> Falling
        if (state_ == AerialState::Rising && vertical_velocity_ <= 0.0f) {
            state_ = AerialState::Falling;
        }
        if (state_ == AerialState::DoubleJump && vertical_velocity_ <= 0.0f) {
            state_ = AerialState::Falling;
        }

        // Ground check
        if (height_ <= 0.0f && vertical_velocity_ <= 0.0f) {
            land();
        }
    }

private:
    bool can_jump() const {
        if (state_ == AerialState::Grounded) return true;
        if (coyote_timer_ > 0.0f && jumps_used_ == 0) return true;
        if (jumps_used_ < config_.max_jumps) return true;
        return false;
    }

    void execute_jump() {
        jump_held_ = true;
        jump_buffer_timer_ = 0.0f;

        if (jumps_used_ == 0) {
            vertical_velocity_ = config_.jump_force;
            state_ = AerialState::Rising;
        } else {
            vertical_velocity_ = config_.double_jump_force;
            state_ = AerialState::DoubleJump;
        }
        jumps_used_++;
        coyote_timer_ = 0.0f;
    }
};

} // namespace tps
} // namespace game
} // namespace qe
