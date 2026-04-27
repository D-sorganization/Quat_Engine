#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file PlayerController.h
 * @brief Player state machine integrating all TPS combat and movement systems.
 *
 * Coordinates: CharacterClass, MeleeSystem, JumpSystem, LockOnSystem,
 * TPSWeapons, and AnimationSystem into a unified player entity.
 *
 * Player states:
 *   - Idle, Moving, Sprinting
 *   - Dodging (invincibility frames)
 *   - MeleeAttacking, Shooting, Reloading
 *   - Airborne (jump, double-jump, slam)
 *   - Staggered, Dead
 *
 * Design by Contract: all state transitions validated via assert.
 * Orthogonal: each subsystem is updated independently, controller
 * only reads outputs and coordinates transitions.
 */

#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"
#include "CharacterClass.h"
#include "MeleeSystem.h"
#include "JumpSystem.h"
#include "LockOnSystem.h"
#include "TPSWeapons.h"
#include "AnimationSystem.h"

#include <cassert>
#include <cmath>
#include <algorithm>

namespace qe {
namespace game {
namespace tps {

// ── Player State ─────────────────────────────────────────────────────────────

enum class PlayerState {
    Idle,
    Moving,
    Sprinting,
    Dodging,
    MeleeAttacking,
    Shooting,
    Reloading,
    Airborne,
    Staggered,
    Dead
};

// ── Dodge State ──────────────────────────────────────────────────────────────

struct DodgeState {
    bool active = false;
    float timer = 0.0f;
    float duration = 0.4f;
    float iframes_duration = 0.25f;  // Invincibility window
    math::Vec3 direction;
    float distance = 3.0f;
    float stamina_cost = 20.0f;

    bool has_iframes() const { return active && timer < iframes_duration; }
    bool is_finished() const { return timer >= duration; }

    void start(const math::Vec3& dir, float dist, float cost) {
        active = true;
        timer = 0.0f;
        direction = dir;
        distance = dist;
        stamina_cost = cost;
    }

    void update(float dt) {
        if (!active) return;
        timer += dt;
        if (timer >= duration) {
            active = false;
        }
    }
};

// ── Player Controller ────────────────────────────────────────────────────────

class PlayerController {
    // Identity
    CharacterClassType class_type_;
    CharacterStats stats_;

    // Transform
    math::Vec3 position_;
    math::Quaternion rotation_ = math::Quaternion::identity();

    // Health & Stamina
    float health_;
    float stamina_;

    // State
    PlayerState state_ = PlayerState::Idle;
    float state_timer_ = 0.0f;
    float stagger_duration_ = 0.0f;

    // Subsystems
    MeleeCombatState melee_;
    JumpState jump_;
    LockOnState lock_on_;
    TPSLoadout loadout_;
    AnimationController anim_;
    DodgeState dodge_;

    // Movement
    math::Vec3 velocity_ = math::Vec3::zero();
    math::Vec3 move_input_ = math::Vec3::zero();
    bool sprint_held_ = false;

public:
    PlayerController()
        : class_type_(CharacterClassType::Vanguard),
          stats_(make_character_stats(CharacterClassType::Vanguard)),
          health_(150.0f), stamina_(100.0f) {}

    explicit PlayerController(CharacterClassType type)
        : class_type_(type), stats_(make_character_stats(type)),
          health_(make_character_stats(type).max_health),
          stamina_(make_character_stats(type).stamina_max) {
        int max_jumps = (type == CharacterClassType::Recon) ? 2 : 1;
        jump_ = JumpState(make_jump_config(stats_.jump_height, max_jumps));
    }

    // ── Queries ──────────────────────────────────────────────────────

    PlayerState state() const { return state_; }
    const math::Vec3& position() const { return position_; }
    const math::Quaternion& rotation() const { return rotation_; }
    float health() const { return health_; }
    float max_health() const { return stats_.max_health; }
    float health_fraction() const { return health_ / stats_.max_health; }
    float stamina() const { return stamina_; }
    float stamina_fraction() const { return stamina_ / stats_.stamina_max; }
    bool is_alive() const { return state_ != PlayerState::Dead; }
    bool is_dodging() const { return state_ == PlayerState::Dodging; }
    bool has_iframes() const { return dodge_.has_iframes(); }
    CharacterClassType class_type() const { return class_type_; }
    const CharacterStats& stats() const { return stats_; }

    const MeleeCombatState& melee() const { return melee_; }
    const JumpState& jump() const { return jump_; }
    const LockOnState& lock_on() const { return lock_on_; }
    const TPSLoadout& loadout() const { return loadout_; }
    TPSLoadout& loadout_mut() { return loadout_; }
    const AnimationController& animation() const { return anim_; }

    bool can_act() const {
        return state_ != PlayerState::Dead
            && state_ != PlayerState::Staggered
            && !dodge_.active;
    }

    // ── Movement Input ───────────────────────────────────────────────

    void set_move_input(const math::Vec3& input) { move_input_ = input; }
    void set_sprint(bool held) { sprint_held_ = held; }

    void set_position(const math::Vec3& pos) { position_ = pos; }
    void set_rotation(const math::Quaternion& rot) { rotation_ = rot; }

    // ── Actions ──────────────────────────────────────────────────────

    /** Take damage. Returns true if player died.
     *  @pre damage >= 0
     */
    bool take_damage(float damage, bool is_explosion = false) {
        assert(damage >= 0.0f);
        if (state_ == PlayerState::Dead) return false;
        if (dodge_.has_iframes()) return false;

        float effective = stats_.apply_damage_reduction(damage, is_explosion);
        health_ -= effective;

        if (health_ <= 0.0f) {
            health_ = 0.0f;
            transition_to(PlayerState::Dead);
            anim_.set_full_body(AnimState::Death, AnimTransition::locked());
            return true;
        }

        // Stagger if hit is hard enough
        if (effective > stats_.max_health * 0.3f) {
            stagger(0.5f);
        }
        return false;
    }

    /** Heal the player. */
    void heal(float amount) {
        assert(amount >= 0.0f);
        health_ = std::min(health_ + amount, stats_.max_health);
    }

    /** Start a dodge roll in the given direction. */
    bool dodge(const math::Vec3& direction) {
        if (!can_act()) return false;
        if (stamina_ < dodge_.stamina_cost) return false;
        if (jump_.is_airborne()) return false;

        stamina_ -= dodge_.stamina_cost;
        math::Vec3 dir = direction.length_squared() > 0.01f
            ? direction.normalized() : rotation_.rotate(math::Vec3::forward());
        dodge_.start(dir, stats_.dodge_distance, dodge_.stamina_cost);
        transition_to(PlayerState::Dodging);
        anim_.set_full_body(AnimState::DodgeRoll, AnimTransition::quick());
        return true;
    }

    /** Light melee attack (combo-aware). */
    bool melee_light() {
        if (!can_act() && !melee_.in_chain_window()) return false;
        if (melee_.start_light_attack(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            transition_to(PlayerState::MeleeAttacking);
            AnimState anim_state = AnimState::MeleeLight1;
            if (melee_.combo_count() == 2) anim_state = AnimState::MeleeLight2;
            if (melee_.combo_count() == 3) anim_state = AnimState::MeleeLight3;
            anim_.set_full_body(anim_state, AnimTransition::quick());
            return true;
        }
        return false;
    }

    /** Heavy melee attack. */
    bool melee_heavy() {
        if (!can_act()) return false;
        if (melee_.start_heavy_attack(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            transition_to(PlayerState::MeleeAttacking);
            anim_.set_full_body(AnimState::MeleeHeavy, AnimTransition::quick());
            return true;
        }
        return false;
    }

    /** Jump attack from airborne state. */
    bool melee_jump_attack() {
        if (!jump_.is_airborne()) return false;
        if (melee_.start_jump_attack(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            jump_.start_ground_slam();
            anim_.set_full_body(AnimState::MeleeJumpAttack, AnimTransition::quick());
            return true;
        }
        return false;
    }

    /** Dodge attack (during dodge). */
    bool melee_dodge_attack() {
        if (!dodge_.active) return false;
        if (melee_.start_dodge_attack(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            anim_.set_full_body(AnimState::MeleeDodgeAttack, AnimTransition::instant());
            return true;
        }
        return false;
    }

    /** Parry. */
    bool parry() {
        if (!can_act()) return false;
        if (melee_.start_parry(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            anim_.set_full_body(AnimState::MeleeParry, AnimTransition::instant());
            return true;
        }
        return false;
    }

    /** Backstab (must be behind target). */
    bool backstab() {
        if (!can_act()) return false;
        if (melee_.start_backstab(stamina_)) {
            stamina_ -= melee_.active_move().stamina_cost;
            transition_to(PlayerState::MeleeAttacking);
            anim_.set_full_body(AnimState::MeleeBackstab, AnimTransition::locked());
            return true;
        }
        return false;
    }

    /** Fire current weapon. */
    bool shoot() {
        if (!can_act()) return false;
        if (loadout_.current().fire()) {
            if (state_ != PlayerState::Shooting) {
                transition_to(PlayerState::Shooting);
            }
            anim_.set_upper_body(AnimState::Shooting, AnimTransition::instant());
            return true;
        }
        return false;
    }

    /** Reload current weapon. */
    void reload() {
        if (!can_act()) return;
        loadout_.current().start_reload();
        transition_to(PlayerState::Reloading);
        anim_.set_upper_body(AnimState::Reloading, AnimTransition::smooth());
    }

    /** Jump. */
    bool request_jump() {
        if (state_ == PlayerState::Dead || state_ == PlayerState::Staggered) return false;
        if (jump_.request_jump()) {
            transition_to(PlayerState::Airborne);
            anim_.set_full_body(
                jump_.jumps_remaining() < (class_type_ == CharacterClassType::Recon ? 1 : 0)
                    ? AnimState::DoubleJump : AnimState::JumpStart,
                AnimTransition::quick());
            return true;
        }
        return false;
    }

    /** Release jump (variable height). */
    void release_jump() { jump_.release_jump(); }

    /** Toggle lock-on. */
    bool toggle_lock_on(const std::vector<LockOnTarget>& targets) {
        if (lock_on_.is_locked()) {
            lock_on_.release_lock();
            return false;
        }
        math::Vec3 fwd = rotation_.rotate(math::Vec3::forward());
        return lock_on_.try_lock_on(position_, fwd, targets);
    }

    /** Switch locked target. */
    bool switch_lock_target(const math::Vec3& dir, const std::vector<LockOnTarget>& targets) {
        return lock_on_.switch_target(position_, dir, targets);
    }

    /** Stagger the player. */
    void stagger(float duration) {
        if (state_ == PlayerState::Dead) return;
        transition_to(PlayerState::Staggered);
        stagger_duration_ = duration;
        state_timer_ = 0.0f;
        melee_.interrupt();
        anim_.set_full_body(AnimState::Stagger, AnimTransition::instant());
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);
        if (state_ == PlayerState::Dead) return;

        state_timer_ += dt;

        // Regenerate stamina
        if (!sprint_held_ && !dodge_.active) {
            stamina_ = std::min(stamina_ + stats_.stamina_regen * dt, stats_.stamina_max);
        }

        // Update subsystems
        melee_.update(dt);
        jump_.update(dt);
        lock_on_.update(dt);
        loadout_.update(dt);
        anim_.update(dt);
        dodge_.update(dt);

        // State transitions
        switch (state_) {
            case PlayerState::Staggered:
                if (state_timer_ >= stagger_duration_) {
                    transition_to(PlayerState::Idle);
                    anim_.set_locomotion(AnimState::Idle, AnimTransition::smooth());
                }
                break;

            case PlayerState::Dodging:
                if (!dodge_.active) {
                    transition_to(PlayerState::Idle);
                }
                // Apply dodge movement
                position_ += dodge_.direction * (stats_.dodge_speed * dt);
                break;

            case PlayerState::MeleeAttacking:
                if (melee_.is_idle()) {
                    transition_to(PlayerState::Idle);
                    anim_.set_locomotion(AnimState::Idle, AnimTransition::smooth());
                }
                break;

            case PlayerState::Airborne:
                if (jump_.is_grounded()) {
                    transition_to(PlayerState::Idle);
                    anim_.set_locomotion(
                        jump_.just_slam_landed() ? AnimState::LandHeavy : AnimState::LandLight,
                        AnimTransition::quick());
                }
                break;

            case PlayerState::Shooting:
            case PlayerState::Reloading:
                if (!loadout_.current().is_reloading() && state_ == PlayerState::Reloading) {
                    transition_to(PlayerState::Idle);
                    anim_.clear_upper_body();
                }
                break;

            default:
                break;
        }

        // Movement
        apply_movement(dt);

        // Update animation based on movement
        if (state_ == PlayerState::Idle || state_ == PlayerState::Moving || state_ == PlayerState::Sprinting) {
            update_locomotion_anim();
        }
    }

private:
    void transition_to(PlayerState next) {
        state_ = next;
        state_timer_ = 0.0f;
    }

    void apply_movement(float dt) {
        if (state_ == PlayerState::Dead || state_ == PlayerState::Staggered) return;
        if (state_ == PlayerState::Dodging) return;  // Dodge handles its own movement

        float speed = stats_.move_speed;
        if (sprint_held_ && stamina_ > 0.0f && move_input_.length_squared() > 0.01f) {
            speed *= stats_.sprint_multiplier;
            stamina_ -= 15.0f * dt;
            if (stamina_ < 0.0f) stamina_ = 0.0f;
        }

        if (jump_.is_airborne()) {
            speed *= jump_.air_control_factor();
        }

        math::Vec3 move = rotation_.rotate(move_input_) * speed;
        position_ += move * dt;

        // Root motion from melee
        math::Vec3 root = anim_.consume_root_motion();
        if (root.length_squared() > 0.001f) {
            position_ += rotation_.rotate(root);
        }

        // Update state based on movement
        if (state_ == PlayerState::Idle && move_input_.length_squared() > 0.01f) {
            if (sprint_held_ && stamina_ > 0.0f) {
                transition_to(PlayerState::Sprinting);
            } else {
                transition_to(PlayerState::Moving);
            }
        } else if ((state_ == PlayerState::Moving || state_ == PlayerState::Sprinting)
                   && move_input_.length_squared() < 0.01f) {
            transition_to(PlayerState::Idle);
        }
    }

    void update_locomotion_anim() {
        float input_mag = move_input_.length();
        AnimState target;
        if (input_mag < 0.1f) {
            target = AnimState::Idle;
        } else if (sprint_held_ && stamina_ > 0.0f) {
            target = AnimState::Sprint;
        } else if (input_mag > 0.5f) {
            target = AnimState::Run;
        } else {
            target = AnimState::Walk;
        }

        if (anim_.current_lower_state() != target) {
            anim_.set_locomotion(target, AnimTransition::smooth());
        }
    }
};

} // namespace tps
} // namespace game
} // namespace qe
