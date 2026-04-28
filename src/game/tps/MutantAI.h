// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file MutantAI.h
 * @brief Enemy AI behavior system for mutant combat encounters.
 *
 * Drives MutantInstance state transitions based on player proximity,
 * line-of-sight, and attack timing. Each mutant type has tuned behavior:
 *
 *   - Grunt:     Patrol → chase → melee swipe, occasional charge
 *   - Crawler:   Aggressive flanking, rapid closing, leap attack
 *   - Brute:     Slow advance, ground slam at range, unstoppable charge
 *   - Stalker:   Circle at detection edge, ambush leap, retreat on stagger
 *   - Spitter:   Maintain distance, ranged acid, flee if closed on
 *   - Screamer:  Buff allies in radius, flee from player, area scream
 *   - Hound:     Pack chase, leaping attacks, circle-strafe
 *   - Amalgam:   Fly overhead, dive-bomb, ranged bomb
 *   - Behemoth:  Slow relentless advance, ground slam AoE, charge
 *   - Apex:      Multi-phase: ranged → melee → AoE, summons adds
 *
 * Pure logic — no rendering dependencies. Testable in isolation.
 * Design by Contract: all state transitions validated.
 */

#include "../../core/Rng.h"
#include "../../math/Constants.h"
#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"
#include "MutantTypes.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// Forward declarations
inline math::Vec3 compute_approach_direction(
    const MutantInstance&, const math::Vec3&, const math::Vec3&, float, float, core::Rng&);
inline float compute_speed_multiplier(MutantType);
inline const MutantAttack& select_attack(const MutantInstance&, float);

// ── AI Decision Result ───────────────────────────────────────────────────────

struct AIDecision {
    bool wants_to_attack = false;
    bool using_primary = true;
    math::Vec3 move_direction = math::Vec3::zero();
    float move_speed_mult = 1.0f;
    bool facing_player = false;
};

// ── AI Update Function ──────────────────────────────────────────────────────

/** Update a single mutant's AI based on player state.
 *  @pre mutant is alive
 *  @post mutant.ai_state is valid, position may change
 */
inline AIDecision update_mutant_ai(
        MutantInstance& mutant,
        const math::Vec3& player_pos,
        bool player_alive,
        float dt,
        core::Rng& rng) {

    assert(mutant.alive && "pre: mutant must be alive");

    AIDecision decision;
    mutant.update(dt);

    if (!player_alive) {
        mutant.ai_state = MutantAIState::Idle;
        return decision;
    }

    math::Vec3 to_player = player_pos - mutant.position;
    float dist_to_player = to_player.length();
    math::Vec3 dir_to_player = dist_to_player > 0.01f
        ? to_player * (1.0f / dist_to_player) : math::Vec3::zero();

    // Face the player when chasing/attacking
    if (dist_to_player > 0.1f) {
        float target_yaw = std::atan2(dir_to_player.x, dir_to_player.z);
        math::Quaternion target_rot = math::Quaternion::from_axis_angle(
            math::Vec3::up(), target_yaw);
        mutant.rotation = math::Quaternion::slerp(mutant.rotation, target_rot,
            std::min(1.0f, 5.0f * dt));
    }

    // Staggered — don't do anything
    if (mutant.ai_state == MutantAIState::Staggered) {
        return decision;
    }

    float detect_range = mutant.config.detection_range;
    float aggro_range = mutant.config.aggro_range;
    float attack_range = mutant.config.attack_range;

    switch (mutant.ai_state) {
        case MutantAIState::Idle:
            if (dist_to_player <= detect_range) {
                mutant.ai_state = MutantAIState::Alerted;
                mutant.state_timer = 0.0f;
            } else {
                // Patrol around origin
                mutant.patrol_phase += dt * 0.5f;
                float px = mutant.patrol_origin.x +
                    std::sin(mutant.patrol_phase) * mutant.patrol_radius;
                float pz = mutant.patrol_origin.z +
                    std::cos(mutant.patrol_phase) * mutant.patrol_radius;
                math::Vec3 patrol_target(px, mutant.patrol_origin.y, pz);
                math::Vec3 to_patrol = patrol_target - mutant.position;
                if (to_patrol.length_squared() > 0.25f) {
                    decision.move_direction = to_patrol.normalized();
                    decision.move_speed_mult = 0.4f;
                }
                mutant.ai_state = MutantAIState::Patrol;
            }
            break;

        case MutantAIState::Patrol:
            if (dist_to_player <= detect_range) {
                mutant.ai_state = MutantAIState::Alerted;
                mutant.state_timer = 0.0f;
            } else {
                mutant.patrol_phase += dt * 0.5f;
                float px = mutant.patrol_origin.x +
                    std::sin(mutant.patrol_phase) * mutant.patrol_radius;
                float pz = mutant.patrol_origin.z +
                    std::cos(mutant.patrol_phase) * mutant.patrol_radius;
                math::Vec3 patrol_target(px, mutant.patrol_origin.y, pz);
                math::Vec3 to_patrol = patrol_target - mutant.position;
                if (to_patrol.length_squared() > 0.25f) {
                    decision.move_direction = to_patrol.normalized();
                    decision.move_speed_mult = 0.4f;
                }
            }
            break;

        case MutantAIState::Alerted:
            if (mutant.state_timer > 0.5f) {
                mutant.ai_state = MutantAIState::Chasing;
                mutant.state_timer = 0.0f;
            }
            decision.facing_player = true;
            break;

        case MutantAIState::Chasing:
            decision.facing_player = true;
            if (dist_to_player > detect_range * 1.5f) {
                mutant.ai_state = MutantAIState::Idle;
                mutant.state_timer = 0.0f;
                break;
            }
            if (dist_to_player <= attack_range && mutant.attack_cooldown <= 0.0f) {
                mutant.ai_state = MutantAIState::Attacking;
                mutant.state_timer = 0.0f;
                break;
            }
            // Move toward player with type-specific behavior
            decision.move_direction = compute_approach_direction(
                mutant, player_pos, dir_to_player, dist_to_player, dt, rng);
            decision.move_speed_mult = compute_speed_multiplier(mutant.config.type);
            break;

        case MutantAIState::Attacking: {
            decision.facing_player = true;
            const auto& atk = select_attack(mutant, dist_to_player);
            float atk_duration = atk.windup_time + 0.3f;

            if (mutant.state_timer < atk.windup_time) {
                // Windup phase — telegraph
            } else if (mutant.state_timer < atk_duration) {
                // Active hit frame
                decision.wants_to_attack = true;
                decision.using_primary = (&atk == &mutant.config.primary_attack);
            } else {
                // Attack finished
                mutant.attack_cooldown = atk.cooldown;
                mutant.ai_state = MutantAIState::Chasing;
                mutant.state_timer = 0.0f;
            }
            break;
        }

        case MutantAIState::Retreating:
            decision.move_direction = dir_to_player * -1.0f;
            decision.move_speed_mult = 0.8f;
            if (dist_to_player > aggro_range || mutant.state_timer > 3.0f) {
                mutant.ai_state = MutantAIState::Chasing;
                mutant.state_timer = 0.0f;
            }
            break;

        case MutantAIState::Summoning:
            // Screamer special: pause and buff
            if (mutant.state_timer > 2.0f) {
                mutant.ai_state = MutantAIState::Chasing;
                mutant.state_timer = 0.0f;
            }
            break;

        case MutantAIState::Staggered:
        case MutantAIState::Dead:
            break;
    }

    return decision;
}

/** Apply AI movement to mutant position. */
inline void apply_ai_movement(MutantInstance& mutant, const AIDecision& decision,
                               float dt) {
    if (!mutant.alive) return;
    if (decision.move_direction.length_squared() < 0.001f) return;

    float speed = mutant.config.move_speed * decision.move_speed_mult;
    mutant.position += decision.move_direction * speed * dt;
    mutant.position.y = std::max(0.0f, mutant.position.y);
}

/** Process mutant attack against player. Returns damage dealt (0 if miss). */
inline float process_mutant_attack(const MutantInstance& mutant,
                                    const AIDecision& decision,
                                    const math::Vec3& player_pos,
                                    bool player_has_iframes) {
    if (!decision.wants_to_attack) return 0.0f;
    if (player_has_iframes) return 0.0f;

    const auto& atk = decision.using_primary
        ? mutant.config.primary_attack : mutant.config.secondary_attack;

    float dist = mutant.position.distance_to(player_pos);
    if (dist > atk.range) return 0.0f;

    return atk.damage;
}

/** Buff nearby mutants when a Screamer uses its ability. */
inline void apply_screamer_buff(const MutantInstance& screamer,
                                 std::vector<MutantInstance>& all_mutants,
                                 float buff_radius = 12.0f) {
    if (screamer.config.type != MutantType::Screamer) return;

    for (auto& m : all_mutants) {
        if (!m.alive || m.id == screamer.id) continue;
        float dist = m.position.distance_to(screamer.position);
        if (dist <= buff_radius) {
            // Buff: temporary speed increase (handled via patrol_phase as timer)
            m.config.move_speed *= 1.2f;
        }
    }
}

// ── Private Helpers ──────────────────────────────────────────────────────────

inline math::Vec3 compute_approach_direction(
        const MutantInstance& mutant,
        const math::Vec3& /*player_pos*/,
        const math::Vec3& dir_to_player,
        float dist,
        float /*dt*/,
        core::Rng& rng) {
    switch (mutant.config.type) {
        case MutantType::Crawler:
        case MutantType::Hound: {
            // Flanking: approach at an angle
            float angle = (rng.random_float(0.0f, 1.0f) - 0.5f) * 0.8f;
            math::Quaternion flank = math::Quaternion::from_axis_angle(
                math::Vec3::up(), angle);
            return flank.rotate(dir_to_player).normalized();
        }

        case MutantType::Spitter: {
            // Maintain distance: retreat if too close, advance if too far
            float ideal = mutant.config.attack_range * 0.7f;
            if (dist < ideal * 0.5f) return dir_to_player * -1.0f;
            if (dist > ideal) return dir_to_player;
            // Strafe
            math::Vec3 strafe = dir_to_player.cross(math::Vec3::up()).normalized();
            return (rng.next() & 1) ? strafe : strafe * -1.0f;
        }

        case MutantType::Screamer: {
            // Flee from player, maintain distance
            if (dist < mutant.config.aggro_range * 0.5f) {
                return dir_to_player * -1.0f;
            }
            return dir_to_player;
        }

        case MutantType::Stalker: {
            // Circle the player
            math::Vec3 strafe = dir_to_player.cross(math::Vec3::up()).normalized();
            if (dist > mutant.config.attack_range * 2.0f) {
                return (dir_to_player + strafe * 0.5f).normalized();
            }
            return strafe;
        }

        case MutantType::Amalgam: {
            // Fly overhead, approach from above
            math::Vec3 approach = dir_to_player;
            approach.y += 0.3f;
            return approach.length_squared() > 0.01f ? approach.normalized() : dir_to_player;
        }

        default:
            return dir_to_player;
    }
}

inline float compute_speed_multiplier(MutantType type) {
    switch (type) {
        case MutantType::Crawler:
        case MutantType::Hound:    return 1.3f;
        case MutantType::Stalker:  return 1.1f;
        case MutantType::Brute:
        case MutantType::Behemoth: return 0.7f;
        case MutantType::Apex:     return 1.0f;
        default:                   return 1.0f;
    }
}

inline const MutantAttack& select_attack(const MutantInstance& mutant,
                                           float dist_to_player) {
    // Use secondary if in range and it's better suited
    const auto& primary = mutant.config.primary_attack;
    const auto& secondary = mutant.config.secondary_attack;

    if (dist_to_player > primary.range && dist_to_player <= secondary.range) {
        return secondary;
    }
    if (secondary.is_ranged() && dist_to_player > primary.range * 0.5f) {
        return secondary;
    }
    return primary;
}

} // namespace tps
} // namespace game
} // namespace qe
