#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file MutantTypes.h
 * @brief Post-apocalyptic mutant enemy types with distinct combat behaviors.
 *
 * Set in an alternate post-WW2 universe where radiation and experimental
 * weapons spawned a mutant plague. Each mutant type has unique stats,
 * attack patterns, and weaknesses that force strategic weapon/class choices.
 *
 * Mutant categories:
 *   - Grunt:       Basic irradiated soldier, slow, predictable
 *   - Crawler:     Fast quadrupedal, melee-only, flanker
 *   - Brute:       Armored tank, charges, splash resistant
 *   - Stalker:     Ambush predator, partial invisibility
 *   - Spitter:     Ranged acid attacker, area denial
 *   - Screamer:    Summons reinforcements, buffs nearby mutants
 *   - Hound:       Fast pack hunter, leaping attacks
 *   - Amalgam:     Flying mutant, dive-bomb attacks
 *   - Behemoth:    Mini-boss, massive HP, devastating attacks
 *   - Apex:        Final boss, multi-phase, all attack types
 */

#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace qe {
namespace game {
namespace tps {

enum class MutantType {
    Grunt,
    Crawler,
    Brute,
    Stalker,
    Spitter,
    Screamer,
    Hound,
    Amalgam,
    Behemoth,
    Apex
};

// ── Mutant Attack Pattern ────────────────────────────────────────────────────

enum class MutantAttackType {
    MeleeSwipe,
    MeleeCharge,
    MeleeLeap,
    RangedSpit,
    RangedBomb,
    AreaScream,
    DiveBomb,
    GroundSlam,
    MultiPhase
};

struct MutantAttack {
    MutantAttackType type;
    float damage;
    float range;
    float cooldown;
    float windup_time;    // Telegraph duration before strike
    float splash_radius;  // 0 = single target

    bool is_melee() const {
        return type == MutantAttackType::MeleeSwipe
            || type == MutantAttackType::MeleeCharge
            || type == MutantAttackType::MeleeLeap
            || type == MutantAttackType::GroundSlam;
    }

    bool is_ranged() const {
        return type == MutantAttackType::RangedSpit
            || type == MutantAttackType::RangedBomb
            || type == MutantAttackType::DiveBomb;
    }
};

// ── Mutant Weakness / Resistance ─────────────────────────────────────────────

struct MutantResistance {
    float ballistic;     // 0.0 = full damage, 1.0 = immune
    float explosive;
    float energy;
    float melee;
    float fire;

    float apply(float damage, float resist) const {
        assert(resist >= 0.0f && resist <= 1.0f);
        return damage * (1.0f - resist);
    }
};

// ── Mutant Config ────────────────────────────────────────────────────────────

struct MutantConfig {
    MutantType type;
    float health;
    float move_speed;
    float detection_range;
    float aggro_range;
    float attack_range;
    int score_value;
    bool can_be_staggered;
    bool can_be_knocked_back;
    float stagger_threshold;  // Damage in single hit to stagger
    MutantResistance resistance;
    MutantAttack primary_attack;
    MutantAttack secondary_attack;

    void check_invariants() const {
        assert(health > 0.0f && "invariant: mutant health must be positive");
        assert(move_speed >= 0.0f && "invariant: move_speed must be non-negative");
        assert(detection_range > 0.0f);
        assert(score_value > 0);
    }
};

// ── Mutant AI State ──────────────────────────────────────────────────────────

enum class MutantAIState {
    Idle,
    Patrol,
    Alerted,
    Chasing,
    Attacking,
    Staggered,
    Retreating,
    Summoning,
    Dead
};

struct MutantInstance {
    MutantConfig config;
    MutantAIState ai_state = MutantAIState::Idle;
    math::Vec3 position;
    math::Quaternion rotation = math::Quaternion::identity();
    float current_health;
    float attack_cooldown = 0.0f;
    float state_timer = 0.0f;
    float stagger_timer = 0.0f;
    bool alive = true;
    int id = 0;

    // Patrol data
    math::Vec3 patrol_origin;
    float patrol_radius = 8.0f;
    float patrol_phase = 0.0f;

    /** Apply damage with resistance. Returns true if killed. */
    bool take_damage(float raw_damage, float resist_value) {
        assert(raw_damage >= 0.0f);
        if (!alive) return false;

        float effective = raw_damage * (1.0f - resist_value);
        current_health -= effective;

        if (config.can_be_staggered && effective >= config.stagger_threshold) {
            ai_state = MutantAIState::Staggered;
            stagger_timer = 0.8f;
        }

        if (current_health <= 0.0f) {
            current_health = 0.0f;
            alive = false;
            ai_state = MutantAIState::Dead;
            return true;
        }
        return false;
    }

    /** Update AI timers. */
    void update(float dt) {
        if (!alive) return;

        state_timer += dt;
        if (attack_cooldown > 0.0f) {
            attack_cooldown -= dt;
            if (attack_cooldown < 0.0f) attack_cooldown = 0.0f;
        }
        if (stagger_timer > 0.0f) {
            stagger_timer -= dt;
            if (stagger_timer <= 0.0f) {
                stagger_timer = 0.0f;
                if (ai_state == MutantAIState::Staggered) {
                    ai_state = MutantAIState::Chasing;
                }
            }
        }
    }

    /** Check if target is within detection range. */
    bool can_detect(const math::Vec3& target_pos) const {
        float dist = position.distance_to(target_pos);
        return dist <= config.detection_range;
    }

    /** Check if target is within attack range. */
    bool in_attack_range(const math::Vec3& target_pos) const {
        float dist = position.distance_to(target_pos);
        return dist <= config.attack_range;
    }

    /** Get the resistance value for a given damage category. */
    float get_resistance(bool is_explosive, bool is_energy, bool is_melee) const {
        if (is_melee) return config.resistance.melee;
        if (is_explosive) return config.resistance.explosive;
        if (is_energy) return config.resistance.energy;
        return config.resistance.ballistic;
    }

    float health_fraction() const {
        return config.health > 0.0f ? current_health / config.health : 0.0f;
    }
};

// ── Mutant Factory ───────────────────────────────────────────────────────────

inline MutantConfig make_mutant_config(MutantType type) {
    MutantConfig c{};
    c.type = type;

    switch (type) {
        case MutantType::Grunt:
            c.health = 80.0f;
            c.move_speed = 3.0f;
            c.detection_range = 20.0f;
            c.aggro_range = 15.0f;
            c.attack_range = 2.0f;
            c.score_value = 50;
            c.can_be_staggered = true;
            c.can_be_knocked_back = true;
            c.stagger_threshold = 25.0f;
            c.resistance = {0.0f, 0.0f, 0.1f, 0.0f, 0.3f};
            c.primary_attack = {MutantAttackType::MeleeSwipe, 15.0f, 2.0f, 1.2f, 0.4f, 0.0f};
            c.secondary_attack = {MutantAttackType::MeleeCharge, 25.0f, 8.0f, 4.0f, 0.8f, 0.0f};
            break;

        case MutantType::Crawler:
            c.health = 50.0f;
            c.move_speed = 8.0f;
            c.detection_range = 25.0f;
            c.aggro_range = 20.0f;
            c.attack_range = 2.5f;
            c.score_value = 75;
            c.can_be_staggered = true;
            c.can_be_knocked_back = true;
            c.stagger_threshold = 15.0f;
            c.resistance = {0.0f, 0.1f, 0.0f, 0.2f, 0.0f};
            c.primary_attack = {MutantAttackType::MeleeSwipe, 20.0f, 2.5f, 0.8f, 0.25f, 0.0f};
            c.secondary_attack = {MutantAttackType::MeleeLeap, 35.0f, 10.0f, 3.0f, 0.5f, 0.0f};
            break;

        case MutantType::Brute:
            c.health = 300.0f;
            c.move_speed = 2.5f;
            c.detection_range = 18.0f;
            c.aggro_range = 15.0f;
            c.attack_range = 3.0f;
            c.score_value = 200;
            c.can_be_staggered = false;
            c.can_be_knocked_back = false;
            c.stagger_threshold = 100.0f;
            c.resistance = {0.3f, 0.5f, 0.1f, 0.4f, 0.2f};
            c.primary_attack = {MutantAttackType::MeleeSwipe, 40.0f, 3.0f, 2.0f, 0.6f, 0.0f};
            c.secondary_attack = {MutantAttackType::GroundSlam, 60.0f, 5.0f, 5.0f, 1.2f, 4.0f};
            break;

        case MutantType::Stalker:
            c.health = 70.0f;
            c.move_speed = 6.0f;
            c.detection_range = 30.0f;
            c.aggro_range = 25.0f;
            c.attack_range = 2.5f;
            c.score_value = 150;
            c.can_be_staggered = true;
            c.can_be_knocked_back = true;
            c.stagger_threshold = 20.0f;
            c.resistance = {0.0f, 0.0f, 0.3f, 0.0f, 0.5f};
            c.primary_attack = {MutantAttackType::MeleeSwipe, 30.0f, 2.5f, 1.0f, 0.2f, 0.0f};
            c.secondary_attack = {MutantAttackType::MeleeLeap, 45.0f, 8.0f, 4.0f, 0.3f, 0.0f};
            break;

        case MutantType::Spitter:
            c.health = 60.0f;
            c.move_speed = 3.5f;
            c.detection_range = 35.0f;
            c.aggro_range = 30.0f;
            c.attack_range = 25.0f;
            c.score_value = 100;
            c.can_be_staggered = true;
            c.can_be_knocked_back = true;
            c.stagger_threshold = 20.0f;
            c.resistance = {0.0f, 0.2f, 0.0f, 0.0f, 0.8f};
            c.primary_attack = {MutantAttackType::RangedSpit, 20.0f, 25.0f, 2.5f, 0.6f, 2.0f};
            c.secondary_attack = {MutantAttackType::MeleeSwipe, 12.0f, 2.0f, 1.5f, 0.4f, 0.0f};
            break;

        case MutantType::Screamer:
            c.health = 100.0f;
            c.move_speed = 4.0f;
            c.detection_range = 25.0f;
            c.aggro_range = 20.0f;
            c.attack_range = 15.0f;
            c.score_value = 175;
            c.can_be_staggered = true;
            c.can_be_knocked_back = false;
            c.stagger_threshold = 30.0f;
            c.resistance = {0.1f, 0.1f, 0.0f, 0.1f, 0.1f};
            c.primary_attack = {MutantAttackType::AreaScream, 10.0f, 15.0f, 6.0f, 1.5f, 12.0f};
            c.secondary_attack = {MutantAttackType::MeleeSwipe, 15.0f, 2.0f, 1.5f, 0.4f, 0.0f};
            break;

        case MutantType::Hound:
            c.health = 40.0f;
            c.move_speed = 10.0f;
            c.detection_range = 30.0f;
            c.aggro_range = 25.0f;
            c.attack_range = 3.0f;
            c.score_value = 60;
            c.can_be_staggered = true;
            c.can_be_knocked_back = true;
            c.stagger_threshold = 10.0f;
            c.resistance = {0.0f, 0.0f, 0.0f, 0.1f, 0.0f};
            c.primary_attack = {MutantAttackType::MeleeSwipe, 18.0f, 3.0f, 0.6f, 0.15f, 0.0f};
            c.secondary_attack = {MutantAttackType::MeleeLeap, 30.0f, 12.0f, 3.0f, 0.4f, 0.0f};
            break;

        case MutantType::Amalgam:
            c.health = 120.0f;
            c.move_speed = 7.0f;
            c.detection_range = 40.0f;
            c.aggro_range = 35.0f;
            c.attack_range = 20.0f;
            c.score_value = 250;
            c.can_be_staggered = true;
            c.can_be_knocked_back = false;
            c.stagger_threshold = 40.0f;
            c.resistance = {0.1f, 0.3f, 0.0f, 0.5f, 0.0f};
            c.primary_attack = {MutantAttackType::DiveBomb, 35.0f, 20.0f, 4.0f, 0.8f, 3.0f};
            c.secondary_attack = {MutantAttackType::RangedBomb, 25.0f, 15.0f, 3.0f, 0.5f, 3.0f};
            break;

        case MutantType::Behemoth:
            c.health = 800.0f;
            c.move_speed = 2.0f;
            c.detection_range = 25.0f;
            c.aggro_range = 20.0f;
            c.attack_range = 5.0f;
            c.score_value = 500;
            c.can_be_staggered = false;
            c.can_be_knocked_back = false;
            c.stagger_threshold = 200.0f;
            c.resistance = {0.4f, 0.3f, 0.2f, 0.5f, 0.3f};
            c.primary_attack = {MutantAttackType::GroundSlam, 80.0f, 5.0f, 3.0f, 1.5f, 6.0f};
            c.secondary_attack = {MutantAttackType::MeleeCharge, 100.0f, 15.0f, 8.0f, 2.0f, 0.0f};
            break;

        case MutantType::Apex:
            c.health = 2000.0f;
            c.move_speed = 5.0f;
            c.detection_range = 50.0f;
            c.aggro_range = 50.0f;
            c.attack_range = 20.0f;
            c.score_value = 2000;
            c.can_be_staggered = false;
            c.can_be_knocked_back = false;
            c.stagger_threshold = 500.0f;
            c.resistance = {0.3f, 0.3f, 0.3f, 0.3f, 0.3f};
            c.primary_attack = {MutantAttackType::MultiPhase, 50.0f, 20.0f, 2.0f, 1.0f, 8.0f};
            c.secondary_attack = {MutantAttackType::GroundSlam, 120.0f, 8.0f, 6.0f, 2.0f, 10.0f};
            break;
    }

    c.check_invariants();
    return c;
}

inline MutantInstance spawn_mutant(MutantType type, const math::Vec3& pos, int id) {
    MutantInstance m;
    m.config = make_mutant_config(type);
    m.position = pos;
    m.patrol_origin = pos;
    m.current_health = m.config.health;
    m.id = id;
    return m;
}

inline const char* mutant_name(MutantType type) {
    switch (type) {
        case MutantType::Grunt:    return "Irradiated Grunt";
        case MutantType::Crawler:  return "Feral Crawler";
        case MutantType::Brute:    return "Armored Brute";
        case MutantType::Stalker:  return "Shadow Stalker";
        case MutantType::Spitter:  return "Acid Spitter";
        case MutantType::Screamer: return "Mutant Screamer";
        case MutantType::Hound:    return "Rad-Hound";
        case MutantType::Amalgam:  return "Flesh Amalgam";
        case MutantType::Behemoth: return "Behemoth";
        case MutantType::Apex:     return "The Apex";
    }
    return "Unknown";
}

} // namespace tps
} // namespace game
} // namespace qe
