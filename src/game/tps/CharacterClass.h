#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file CharacterClass.h
 * @brief Character class system with distinct strengths, weaknesses, and weapon affinities.
 *
 * Four classes designed for strategic diversity:
 *   - Vanguard:  Balanced frontline fighter, melee bonus, high HP
 *   - Recon:     Agile flanker, critical hits, dodge mastery
 *   - Heavy:     Slow juggernaut, explosive resistance, suppression
 *   - Phantom:   Stealth operative, backstab bonus, silent movement
 *
 * Each class modifies combat stats multiplicatively, allowing orthogonal
 * interaction with weapon classes and power-ups without tight coupling.
 *
 * Design by Contract: all multipliers are strictly positive, health > 0.
 */

#include <cassert>
#include <cmath>

namespace qe {
namespace game {
namespace tps {

// ── Character Class Enum ─────────────────────────────────────────────────────

enum class CharacterClassType {
    Vanguard,
    Recon,
    Heavy,
    Phantom
};

// ── Stat Modifiers ───────────────────────────────────────────────────────────

struct CharacterStats {
    float max_health;
    float move_speed;
    float sprint_multiplier;
    float melee_damage_mult;
    float ranged_damage_mult;
    float critical_chance;       // 0.0 - 1.0
    float critical_multiplier;
    float dodge_speed;
    float dodge_distance;
    float armor;                 // Damage reduction fraction 0.0 - 0.8
    float explosion_resistance;  // 0.0 - 1.0
    float stamina_max;
    float stamina_regen;
    float jump_height;
    float lock_on_stability;     // Lower = less wobble (0.0 - 1.0)
    float stealth_modifier;      // Lower = harder to detect (0.0 - 1.0)

    /** @pre All multipliers must be positive. */
    void check_invariants() const {
        assert(max_health > 0.0f && "invariant: max_health must be positive");
        assert(move_speed > 0.0f && "invariant: move_speed must be positive");
        assert(sprint_multiplier >= 1.0f && "invariant: sprint_multiplier >= 1.0");
        assert(melee_damage_mult > 0.0f && "invariant: melee_damage_mult must be positive");
        assert(ranged_damage_mult > 0.0f && "invariant: ranged_damage_mult must be positive");
        assert(critical_chance >= 0.0f && critical_chance <= 1.0f);
        assert(critical_multiplier >= 1.0f);
        assert(dodge_speed > 0.0f);
        assert(dodge_distance > 0.0f);
        assert(armor >= 0.0f && armor <= 0.8f);
        assert(explosion_resistance >= 0.0f && explosion_resistance <= 1.0f);
        assert(stamina_max > 0.0f);
        assert(stamina_regen > 0.0f);
        assert(jump_height > 0.0f);
        assert(lock_on_stability >= 0.0f && lock_on_stability <= 1.0f);
        assert(stealth_modifier >= 0.0f && stealth_modifier <= 1.0f);
    }

    /** Apply armor and resistance to incoming damage. */
    float apply_damage_reduction(float raw_damage, bool is_explosion) const {
        assert(raw_damage >= 0.0f && "pre: damage must be non-negative");
        float reduced = raw_damage * (1.0f - armor);
        if (is_explosion) {
            reduced *= (1.0f - explosion_resistance);
        }
        assert(reduced >= 0.0f && "post: reduced damage must be non-negative");
        return reduced;
    }

    /** Compute effective melee damage for this class. */
    float effective_melee_damage(float base_damage) const {
        assert(base_damage >= 0.0f);
        return base_damage * melee_damage_mult;
    }

    /** Compute effective ranged damage for this class. */
    float effective_ranged_damage(float base_damage) const {
        assert(base_damage >= 0.0f);
        return base_damage * ranged_damage_mult;
    }
};

// ── Character Class Factory ──────────────────────────────────────────────────

/** Generate stats for a given class type.
 *  @pre type is a valid CharacterClassType
 *  @post returned stats pass check_invariants()
 */
inline CharacterStats make_character_stats(CharacterClassType type) {
    CharacterStats s{};

    switch (type) {
        case CharacterClassType::Vanguard:
            s.max_health          = 150.0f;
            s.move_speed          = 5.0f;
            s.sprint_multiplier   = 1.5f;
            s.melee_damage_mult   = 1.4f;
            s.ranged_damage_mult  = 1.0f;
            s.critical_chance     = 0.08f;
            s.critical_multiplier = 1.5f;
            s.dodge_speed         = 8.0f;
            s.dodge_distance      = 2.5f;
            s.armor               = 0.25f;
            s.explosion_resistance = 0.15f;
            s.stamina_max         = 100.0f;
            s.stamina_regen       = 12.0f;
            s.jump_height         = 3.0f;
            s.lock_on_stability   = 0.3f;
            s.stealth_modifier    = 0.7f;
            break;

        case CharacterClassType::Recon:
            s.max_health          = 90.0f;
            s.move_speed          = 7.0f;
            s.sprint_multiplier   = 1.8f;
            s.melee_damage_mult   = 1.0f;
            s.ranged_damage_mult  = 1.15f;
            s.critical_chance     = 0.25f;
            s.critical_multiplier = 2.5f;
            s.dodge_speed         = 12.0f;
            s.dodge_distance      = 4.0f;
            s.armor               = 0.08f;
            s.explosion_resistance = 0.05f;
            s.stamina_max         = 120.0f;
            s.stamina_regen       = 18.0f;
            s.jump_height         = 4.5f;
            s.lock_on_stability   = 0.15f;
            s.stealth_modifier    = 0.4f;
            break;

        case CharacterClassType::Heavy:
            s.max_health          = 250.0f;
            s.move_speed          = 3.5f;
            s.sprint_multiplier   = 1.25f;
            s.melee_damage_mult   = 1.2f;
            s.ranged_damage_mult  = 1.3f;
            s.critical_chance     = 0.04f;
            s.critical_multiplier = 1.3f;
            s.dodge_speed         = 5.0f;
            s.dodge_distance      = 1.5f;
            s.armor               = 0.4f;
            s.explosion_resistance = 0.5f;
            s.stamina_max         = 80.0f;
            s.stamina_regen       = 8.0f;
            s.jump_height         = 2.0f;
            s.lock_on_stability   = 0.5f;
            s.stealth_modifier    = 0.9f;
            break;

        case CharacterClassType::Phantom:
            s.max_health          = 100.0f;
            s.move_speed          = 6.0f;
            s.sprint_multiplier   = 1.6f;
            s.melee_damage_mult   = 1.8f;  // Backstab specialist
            s.ranged_damage_mult  = 0.9f;
            s.critical_chance     = 0.2f;
            s.critical_multiplier = 3.0f;  // Devastating crits
            s.dodge_speed         = 11.0f;
            s.dodge_distance      = 3.5f;
            s.armor               = 0.1f;
            s.explosion_resistance = 0.1f;
            s.stamina_max         = 110.0f;
            s.stamina_regen       = 15.0f;
            s.jump_height         = 3.8f;
            s.lock_on_stability   = 0.2f;
            s.stealth_modifier    = 0.15f; // Very stealthy
            break;
    }

    s.check_invariants();
    return s;
}

// ── Class Name Utility ───────────────────────────────────────────────────────

inline const char* class_name(CharacterClassType type) {
    switch (type) {
        case CharacterClassType::Vanguard: return "Vanguard";
        case CharacterClassType::Recon:    return "Recon";
        case CharacterClassType::Heavy:    return "Heavy";
        case CharacterClassType::Phantom:  return "Phantom";
    }
    return "Unknown";
}

inline const char* class_description(CharacterClassType type) {
    switch (type) {
        case CharacterClassType::Vanguard:
            return "Frontline warrior. High health, strong melee, balanced ranged.";
        case CharacterClassType::Recon:
            return "Swift striker. Fast movement, high crits, low armor.";
        case CharacterClassType::Heavy:
            return "Walking fortress. Massive health, explosion resistant, slow.";
        case CharacterClassType::Phantom:
            return "Shadow operative. Lethal backstabs, stealth master, fragile.";
    }
    return "";
}

} // namespace tps
} // namespace game
} // namespace qe
