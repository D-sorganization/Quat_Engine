#pragma once
/**
 * @file MeleeSystem.h
 * @brief Action-game quality melee combat with combos, counters, and aerial moves.
 *
 * Melee system designed to rival dedicated action games. Features:
 *   - 3-hit light attack combo chain with timing windows
 *   - Heavy charged attack with variable damage
 *   - Dodge-attack for counter-offensive play
 *   - Jump-attack aerial slam with AoE
 *   - Parry/counter system with timing-based perfect parry
 *   - Backstab bonus for stealth approaches
 *   - Stagger system that creates openings on enemies
 *
 * Uses quaternion rotation for attack direction and sweep arcs.
 * Orthogonal to weapon system — melee is always available regardless of loadout.
 */

#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>

namespace qe {
namespace game {
namespace tps {

// ── Melee Move Types ─────────────────────────────────────────────────────────

enum class MeleeMove {
    None,
    LightAttack1,   // Quick jab
    LightAttack2,   // Cross strike
    LightAttack3,   // Finishing sweep (wider arc)
    HeavyAttack,    // Charged overhead slam
    DodgeAttack,    // Quick slash during dodge
    JumpAttack,     // Aerial slam with AoE
    Parry,          // Defensive stance, timing window
    Backstab        // High damage from behind
};

// ── Melee Move Config ────────────────────────────────────────────────────────

struct MeleeMoveConfig {
    MeleeMove move;
    float base_damage;
    float arc_angle;         // Sweep arc in radians
    float range;             // Forward reach
    float windup;            // Seconds before hit frame
    float active_frames;     // Seconds where hitbox is active
    float recovery;          // Seconds after active before next action
    float stamina_cost;
    float knockback_force;
    float stagger_power;     // Chance to stagger target
    bool can_chain_from;     // Can follow from previous combo hit
    float chain_window;      // Seconds to input next combo hit
    float splash_radius;     // 0 = single target arc

    float total_duration() const {
        return windup + active_frames + recovery;
    }

    void check_invariants() const {
        assert(base_damage >= 0.0f);
        assert(arc_angle >= 0.0f);
        assert(range >= 0.0f);
        assert(windup >= 0.0f);
        assert(active_frames > 0.0f);
        assert(recovery >= 0.0f);
        assert(stamina_cost >= 0.0f);
    }
};

// ── Melee Move Factories ─────────────────────────────────────────────────────

inline MeleeMoveConfig make_melee_move(MeleeMove move) {
    MeleeMoveConfig c{};
    c.move = move;

    switch (move) {
        case MeleeMove::None:
            c.base_damage = 0.0f;
            c.arc_angle = 0.0f;
            c.range = 0.0f;
            c.windup = 0.0f;
            c.active_frames = 0.01f;
            c.recovery = 0.0f;
            c.stamina_cost = 0.0f;
            c.knockback_force = 0.0f;
            c.stagger_power = 0.0f;
            c.can_chain_from = false;
            c.chain_window = 0.0f;
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::LightAttack1:
            c.base_damage = 20.0f;
            c.arc_angle = 0.8f;      // ~45 degrees
            c.range = 2.0f;
            c.windup = 0.08f;
            c.active_frames = 0.12f;
            c.recovery = 0.15f;
            c.stamina_cost = 8.0f;
            c.knockback_force = 1.0f;
            c.stagger_power = 0.15f;
            c.can_chain_from = false;
            c.chain_window = 0.4f;
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::LightAttack2:
            c.base_damage = 25.0f;
            c.arc_angle = 1.0f;
            c.range = 2.2f;
            c.windup = 0.06f;
            c.active_frames = 0.14f;
            c.recovery = 0.18f;
            c.stamina_cost = 10.0f;
            c.knockback_force = 1.5f;
            c.stagger_power = 0.2f;
            c.can_chain_from = true;
            c.chain_window = 0.35f;
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::LightAttack3:
            c.base_damage = 35.0f;
            c.arc_angle = 1.8f;       // Wide sweep ~100 degrees
            c.range = 2.5f;
            c.windup = 0.1f;
            c.active_frames = 0.18f;
            c.recovery = 0.35f;
            c.stamina_cost = 15.0f;
            c.knockback_force = 3.0f;
            c.stagger_power = 0.4f;
            c.can_chain_from = true;
            c.chain_window = 0.0f;    // End of combo
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::HeavyAttack:
            c.base_damage = 60.0f;
            c.arc_angle = 1.2f;
            c.range = 2.8f;
            c.windup = 0.5f;         // Long telegraph
            c.active_frames = 0.15f;
            c.recovery = 0.5f;
            c.stamina_cost = 25.0f;
            c.knockback_force = 5.0f;
            c.stagger_power = 0.7f;
            c.can_chain_from = false;
            c.chain_window = 0.0f;
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::DodgeAttack:
            c.base_damage = 30.0f;
            c.arc_angle = 0.6f;
            c.range = 2.5f;
            c.windup = 0.04f;       // Very fast
            c.active_frames = 0.1f;
            c.recovery = 0.25f;
            c.stamina_cost = 12.0f;
            c.knockback_force = 2.0f;
            c.stagger_power = 0.25f;
            c.can_chain_from = false;
            c.chain_window = 0.3f;
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::JumpAttack:
            c.base_damage = 50.0f;
            c.arc_angle = 6.28f;     // Full 360 degrees
            c.range = 3.0f;
            c.windup = 0.3f;         // Airtime
            c.active_frames = 0.2f;
            c.recovery = 0.4f;
            c.stamina_cost = 20.0f;
            c.knockback_force = 6.0f;
            c.stagger_power = 0.6f;
            c.can_chain_from = false;
            c.chain_window = 0.0f;
            c.splash_radius = 3.5f;  // AoE slam
            break;

        case MeleeMove::Parry:
            c.base_damage = 0.0f;
            c.arc_angle = 3.14f;     // Front hemisphere
            c.range = 1.5f;
            c.windup = 0.0f;
            c.active_frames = 0.2f;  // Parry window
            c.recovery = 0.3f;
            c.stamina_cost = 5.0f;
            c.knockback_force = 0.0f;
            c.stagger_power = 0.0f;
            c.can_chain_from = false;
            c.chain_window = 0.5f;   // Riposte window after success
            c.splash_radius = 0.0f;
            break;

        case MeleeMove::Backstab:
            c.base_damage = 80.0f;   // Massive damage
            c.arc_angle = 0.5f;
            c.range = 1.8f;
            c.windup = 0.15f;
            c.active_frames = 0.25f;
            c.recovery = 0.6f;
            c.stamina_cost = 15.0f;
            c.knockback_force = 0.0f;
            c.stagger_power = 1.0f;  // Always staggers
            c.can_chain_from = false;
            c.chain_window = 0.0f;
            c.splash_radius = 0.0f;
            break;
    }

    c.check_invariants();
    return c;
}

// ── Melee Combat State Machine ───────────────────────────────────────────────

enum class MeleePhase {
    Idle,
    Windup,
    Active,
    Recovery,
    ChainWindow,
    ParryStance,
    ParrySuccess
};

class MeleeCombatState {
    MeleePhase phase_ = MeleePhase::Idle;
    MeleeMoveConfig current_move_ = make_melee_move(MeleeMove::None);
    float phase_timer_ = 0.0f;
    int combo_count_ = 0;
    float charge_time_ = 0.0f;
    bool parry_succeeded_ = false;

public:
    // ── Queries ──────────────────────────────────────────────────────

    MeleePhase phase() const { return phase_; }
    MeleeMove current_move() const { return current_move_.move; }
    int combo_count() const { return combo_count_; }
    bool is_idle() const { return phase_ == MeleePhase::Idle; }
    bool is_attacking() const { return phase_ == MeleePhase::Active; }
    bool in_chain_window() const { return phase_ == MeleePhase::ChainWindow; }
    bool is_parrying() const { return phase_ == MeleePhase::ParryStance; }
    bool parry_succeeded() const { return parry_succeeded_; }
    float phase_timer() const { return phase_timer_; }

    bool can_act() const {
        return phase_ == MeleePhase::Idle
            || phase_ == MeleePhase::ChainWindow
            || phase_ == MeleePhase::ParrySuccess;
    }

    /** Get the active hitbox config (only valid during Active phase). */
    const MeleeMoveConfig& active_move() const { return current_move_; }

    // ── Commands ─────────────────────────────────────────────────────

    /** Start a light attack. Chains automatically if in combo window. */
    bool start_light_attack(float stamina) {
        if (!can_act()) return false;

        MeleeMove next;
        if (phase_ == MeleePhase::ChainWindow && combo_count_ == 1) {
            next = MeleeMove::LightAttack2;
        } else if (phase_ == MeleePhase::ChainWindow && combo_count_ == 2) {
            next = MeleeMove::LightAttack3;
        } else {
            next = MeleeMove::LightAttack1;
            combo_count_ = 0;
        }

        MeleeMoveConfig cfg = make_melee_move(next);
        if (stamina < cfg.stamina_cost) return false;

        begin_move(cfg);
        combo_count_++;
        return true;
    }

    /** Start heavy attack. */
    bool start_heavy_attack(float stamina) {
        if (!can_act()) return false;
        MeleeMoveConfig cfg = make_melee_move(MeleeMove::HeavyAttack);
        if (stamina < cfg.stamina_cost) return false;
        begin_move(cfg);
        combo_count_ = 0;
        return true;
    }

    /** Start dodge attack (called during a dodge). */
    bool start_dodge_attack(float stamina) {
        MeleeMoveConfig cfg = make_melee_move(MeleeMove::DodgeAttack);
        if (stamina < cfg.stamina_cost) return false;
        begin_move(cfg);
        combo_count_ = 0;
        return true;
    }

    /** Start jump attack (called while airborne). */
    bool start_jump_attack(float stamina) {
        MeleeMoveConfig cfg = make_melee_move(MeleeMove::JumpAttack);
        if (stamina < cfg.stamina_cost) return false;
        begin_move(cfg);
        combo_count_ = 0;
        return true;
    }

    /** Initiate parry stance. */
    bool start_parry(float stamina) {
        if (!can_act()) return false;
        MeleeMoveConfig cfg = make_melee_move(MeleeMove::Parry);
        if (stamina < cfg.stamina_cost) return false;
        current_move_ = cfg;
        phase_ = MeleePhase::ParryStance;
        phase_timer_ = 0.0f;
        parry_succeeded_ = false;
        combo_count_ = 0;
        return true;
    }

    /** Call when an incoming attack is detected during parry window.
     *  Returns true if parry was successful (timing window). */
    bool try_parry_incoming() {
        if (phase_ != MeleePhase::ParryStance) return false;
        if (phase_timer_ <= current_move_.active_frames) {
            parry_succeeded_ = true;
            phase_ = MeleePhase::ParrySuccess;
            phase_timer_ = 0.0f;
            return true;
        }
        return false;
    }

    /** Start backstab (requires being behind target). */
    bool start_backstab(float stamina) {
        if (phase_ != MeleePhase::Idle) return false;
        MeleeMoveConfig cfg = make_melee_move(MeleeMove::Backstab);
        if (stamina < cfg.stamina_cost) return false;
        begin_move(cfg);
        combo_count_ = 0;
        return true;
    }

    /** Cancel current melee action (e.g., interrupted by damage). */
    void interrupt() {
        phase_ = MeleePhase::Idle;
        phase_timer_ = 0.0f;
        combo_count_ = 0;
    }

    /** Check if a target position is within the current attack arc.
     *  Uses quaternion rotation to define the sweep region. */
    bool is_in_attack_arc(const math::Vec3& attacker_pos,
                          const math::Quaternion& attacker_rot,
                          const math::Vec3& target_pos) const {
        if (phase_ != MeleePhase::Active) return false;

        math::Vec3 to_target = target_pos - attacker_pos;
        float dist = to_target.length();
        if (dist > current_move_.range || dist < 0.01f) return false;

        math::Vec3 forward = attacker_rot.rotate(math::Vec3::forward());
        math::Vec3 dir = to_target.normalized();
        float dot = forward.dot(dir);
        float half_arc = current_move_.arc_angle * 0.5f;
        float threshold = std::cos(half_arc);

        return dot >= threshold;
    }

    /** Compute damage for the current active hit, with class multiplier. */
    float compute_damage(float class_melee_mult) const {
        assert(class_melee_mult > 0.0f);
        return current_move_.base_damage * class_melee_mult;
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);
        phase_timer_ += dt;

        switch (phase_) {
            case MeleePhase::Idle:
                break;

            case MeleePhase::Windup:
                if (phase_timer_ >= current_move_.windup) {
                    phase_ = MeleePhase::Active;
                    phase_timer_ = 0.0f;
                }
                break;

            case MeleePhase::Active:
                if (phase_timer_ >= current_move_.active_frames) {
                    phase_ = MeleePhase::Recovery;
                    phase_timer_ = 0.0f;
                }
                break;

            case MeleePhase::Recovery:
                if (phase_timer_ >= current_move_.recovery) {
                    if (current_move_.chain_window > 0.0f) {
                        phase_ = MeleePhase::ChainWindow;
                    } else {
                        phase_ = MeleePhase::Idle;
                        combo_count_ = 0;
                    }
                    phase_timer_ = 0.0f;
                }
                break;

            case MeleePhase::ChainWindow:
                if (phase_timer_ >= current_move_.chain_window) {
                    phase_ = MeleePhase::Idle;
                    combo_count_ = 0;
                    phase_timer_ = 0.0f;
                }
                break;

            case MeleePhase::ParryStance:
                if (phase_timer_ >= current_move_.active_frames + current_move_.recovery) {
                    phase_ = MeleePhase::Idle;
                    phase_timer_ = 0.0f;
                }
                break;

            case MeleePhase::ParrySuccess:
                if (phase_timer_ >= current_move_.chain_window) {
                    phase_ = MeleePhase::Idle;
                    phase_timer_ = 0.0f;
                }
                break;
        }
    }

private:
    void begin_move(const MeleeMoveConfig& cfg) {
        current_move_ = cfg;
        phase_ = MeleePhase::Windup;
        phase_timer_ = 0.0f;
        parry_succeeded_ = false;
    }
};

} // namespace tps
} // namespace game
} // namespace qe
