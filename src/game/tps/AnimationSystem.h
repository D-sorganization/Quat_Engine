// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file AnimationSystem.h
 * @brief Animation state machine for realistic third-person character movement.
 *
 * State-driven animation blending using quaternion SLERP for smooth transitions.
 * Features:
 *   - Locomotion states: Idle, Walk, Run, Sprint with blend trees
 *   - Combat states: MeleeAttack, Shooting, Reloading, ADS
 *   - Aerial states: Jump, Fall, DoubleJump, GroundSlam, Land
 *   - Utility states: Dodge, Parry, Stagger, Death
 *   - Upper/lower body split for simultaneous movement + combat
 *   - Transition blending with configurable durations
 *   - Root motion extraction for melee lunges
 *
 * Orthogonal: animation state reads from game state but does not modify it.
 */

#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>

namespace qe {
namespace game {
namespace tps {

// ── Animation State Enum ─────────────────────────────────────────────────────

enum class AnimState {
    // Locomotion
    Idle,
    Walk,
    Run,
    Sprint,

    // Combat - Upper body
    MeleeLight1,
    MeleeLight2,
    MeleeLight3,
    MeleeHeavy,
    MeleeDodgeAttack,
    MeleeJumpAttack,
    MeleeParry,
    MeleeBackstab,
    Shooting,
    Reloading,
    AimDownSights,

    // Aerial
    JumpStart,
    JumpAscend,
    JumpPeak,
    JumpDescend,
    DoubleJump,
    GroundSlam,
    LandLight,
    LandHeavy,

    // Reactions
    DodgeRoll,
    Stagger,
    KnockBack,
    Death
};

// ── Animation Clip ───────────────────────────────────────────────────────────

struct AnimClip {
    AnimState state;
    float duration;      // Total clip length in seconds
    float speed;         // Playback speed multiplier
    bool looping;
    bool upper_body_only;  // Can play on upper body while legs animate separately

    float effective_duration() const {
        return speed > 0.0f ? duration / speed : duration;
    }
};

inline AnimClip make_anim_clip(AnimState state) {
    AnimClip c{};
    c.state = state;
    c.speed = 1.0f;
    c.looping = false;
    c.upper_body_only = false;

    switch (state) {
        case AnimState::Idle:
            c.duration = 2.0f; c.looping = true; break;
        case AnimState::Walk:
            c.duration = 0.8f; c.looping = true; break;
        case AnimState::Run:
            c.duration = 0.6f; c.looping = true; break;
        case AnimState::Sprint:
            c.duration = 0.5f; c.looping = true; break;

        case AnimState::MeleeLight1:
            c.duration = 0.35f; c.upper_body_only = false; break;
        case AnimState::MeleeLight2:
            c.duration = 0.38f; c.upper_body_only = false; break;
        case AnimState::MeleeLight3:
            c.duration = 0.63f; c.upper_body_only = false; break;
        case AnimState::MeleeHeavy:
            c.duration = 1.15f; c.upper_body_only = false; break;
        case AnimState::MeleeDodgeAttack:
            c.duration = 0.39f; c.upper_body_only = false; break;
        case AnimState::MeleeJumpAttack:
            c.duration = 0.9f; c.upper_body_only = false; break;
        case AnimState::MeleeParry:
            c.duration = 0.5f; c.upper_body_only = false; break;
        case AnimState::MeleeBackstab:
            c.duration = 1.0f; c.upper_body_only = false; break;

        case AnimState::Shooting:
            c.duration = 0.15f; c.looping = true; c.upper_body_only = true; break;
        case AnimState::Reloading:
            c.duration = 2.0f; c.upper_body_only = true; break;
        case AnimState::AimDownSights:
            c.duration = 0.25f; c.upper_body_only = true; break;

        case AnimState::JumpStart:
            c.duration = 0.15f; break;
        case AnimState::JumpAscend:
            c.duration = 0.4f; break;
        case AnimState::JumpPeak:
            c.duration = 0.2f; break;
        case AnimState::JumpDescend:
            c.duration = 0.5f; c.looping = true; break;
        case AnimState::DoubleJump:
            c.duration = 0.35f; break;
        case AnimState::GroundSlam:
            c.duration = 0.6f; break;
        case AnimState::LandLight:
            c.duration = 0.2f; break;
        case AnimState::LandHeavy:
            c.duration = 0.4f; break;

        case AnimState::DodgeRoll:
            c.duration = 0.45f; break;
        case AnimState::Stagger:
            c.duration = 0.5f; break;
        case AnimState::KnockBack:
            c.duration = 0.8f; break;
        case AnimState::Death:
            c.duration = 1.5f; break;
    }

    return c;
}

// ── Transition Config ────────────────────────────────────────────────────────

struct AnimTransition {
    float blend_duration;  // Seconds to crossfade
    bool interruptible;    // Can this transition be interrupted

    static AnimTransition instant() { return {0.0f, true}; }
    static AnimTransition quick() { return {0.1f, true}; }
    static AnimTransition smooth() { return {0.2f, true}; }
    static AnimTransition locked() { return {0.15f, false}; }
};

// ── Animation Layer ──────────────────────────────────────────────────────────

struct AnimLayer {
    AnimClip clip;
    float time = 0.0f;
    float weight = 1.0f;
    bool active = false;

    float normalized_time() const {
        return clip.duration > 0.0f ? time / clip.effective_duration() : 0.0f;
    }

    bool is_finished() const {
        return !clip.looping && time >= clip.effective_duration();
    }

    void advance(float dt) {
        if (!active) return;
        time += dt * clip.speed;
        if (clip.looping && time >= clip.effective_duration()) {
            time -= clip.effective_duration();
        }
    }
};

// ── Animation Controller ─────────────────────────────────────────────────────

class AnimationController {
    AnimLayer lower_body_;
    AnimLayer upper_body_;
    AnimLayer full_body_;  // Overrides both when active

    AnimTransition pending_transition_ = AnimTransition::instant();
    float blend_timer_ = 0.0f;
    float blend_weight_ = 1.0f;

    // Root motion
    math::Vec3 root_motion_delta_ = math::Vec3::zero();
    math::Quaternion root_rotation_delta_ = math::Quaternion::identity();

public:
    AnimationController() {
        set_full_body(AnimState::Idle, AnimTransition::instant());
    }

    // ── Queries ──────────────────────────────────────────────────────

    AnimState current_lower_state() const {
        return full_body_.active ? full_body_.clip.state : lower_body_.clip.state;
    }

    AnimState current_upper_state() const {
        return upper_body_.active ? upper_body_.clip.state :
               full_body_.active ? full_body_.clip.state : lower_body_.clip.state;
    }

    float blend_weight() const { return blend_weight_; }
    bool is_blending() const { return blend_timer_ > 0.0f; }

    math::Vec3 consume_root_motion() {
        math::Vec3 delta = root_motion_delta_;
        root_motion_delta_ = math::Vec3::zero();
        return delta;
    }

    math::Quaternion consume_root_rotation() {
        math::Quaternion delta = root_rotation_delta_;
        root_rotation_delta_ = math::Quaternion::identity();
        return delta;
    }

    bool is_full_body_finished() const {
        return full_body_.active && full_body_.is_finished();
    }

    float normalized_time() const {
        if (full_body_.active) return full_body_.normalized_time();
        return lower_body_.normalized_time();
    }

    // ── Commands ─────────────────────────────────────────────────────

    /** Play a full-body animation (locks both layers). */
    void set_full_body(AnimState state, AnimTransition transition) {
        AnimClip clip = make_anim_clip(state);
        full_body_.clip = clip;
        full_body_.time = 0.0f;
        full_body_.weight = 1.0f;
        full_body_.active = true;
        upper_body_.active = false;
        start_blend(transition);
    }

    /** Play a locomotion animation (lower body, or full if no upper override). */
    void set_locomotion(AnimState state, AnimTransition transition) {
        AnimClip clip = make_anim_clip(state);
        assert(clip.looping || state == AnimState::LandLight || state == AnimState::LandHeavy);
        lower_body_.clip = clip;
        lower_body_.time = 0.0f;
        lower_body_.weight = 1.0f;
        lower_body_.active = true;
        full_body_.active = false;
        start_blend(transition);
    }

    /** Play an upper-body override (shooting, reloading while moving). */
    void set_upper_body(AnimState state, AnimTransition transition) {
        AnimClip clip = make_anim_clip(state);
        upper_body_.clip = clip;
        upper_body_.time = 0.0f;
        upper_body_.weight = 1.0f;
        upper_body_.active = true;
        full_body_.active = false;
        start_blend(transition);
    }

    void clear_upper_body() {
        upper_body_.active = false;
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);

        // Advance layers
        lower_body_.advance(dt);
        upper_body_.advance(dt);
        full_body_.advance(dt);

        // Blend timer
        if (blend_timer_ > 0.0f) {
            blend_timer_ -= dt;
            if (blend_timer_ <= 0.0f) {
                blend_timer_ = 0.0f;
                blend_weight_ = 1.0f;
            } else {
                float total = pending_transition_.blend_duration;
                blend_weight_ = 1.0f - (blend_timer_ / total);
            }
        }

        // Compute root motion for melee lunges
        compute_root_motion(dt);
    }

private:
    void start_blend(AnimTransition transition) {
        pending_transition_ = transition;
        blend_timer_ = transition.blend_duration;
        if (transition.blend_duration <= 0.0f) {
            blend_weight_ = 1.0f;
        } else {
            blend_weight_ = 0.0f;
        }
    }

    void compute_root_motion(float dt) {
        // Generate forward lunge for melee attacks
        AnimState active = full_body_.active ? full_body_.clip.state : lower_body_.clip.state;
        float lunge_speed = 0.0f;

        switch (active) {
            case AnimState::MeleeLight1:  lunge_speed = 3.0f; break;
            case AnimState::MeleeLight2:  lunge_speed = 4.0f; break;
            case AnimState::MeleeLight3:  lunge_speed = 5.0f; break;
            case AnimState::MeleeHeavy:   lunge_speed = 6.0f; break;
            case AnimState::DodgeRoll:    lunge_speed = 10.0f; break;
            default: break;
        }

        if (lunge_speed > 0.0f) {
            root_motion_delta_ = math::Vec3(0.0f, 0.0f, -lunge_speed * dt);
        }
    }
};

} // namespace tps
} // namespace game
} // namespace qe
