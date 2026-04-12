#pragma once
/**
 * @file TPSCombat.h
 * @brief Combined ranged + melee combat pipeline for TPS game.
 *
 * Orchestrates the shoot/melee -> hit detection -> damage -> scoring pipeline.
 * Integrates: TPSWeapons, MeleeSystem, MutantTypes, LockOnSystem, CharacterClass.
 *
 * Responsibilities:
 *   - Ranged combat: fire directions (with lock-on wobble), hitscan, projectiles
 *   - Melee combat: arc hit detection, backstab check, combo damage
 *   - Damage resolution: weapon category vs mutant resistance
 *   - Kill processing: score, power-up drops
 *   - Critical hit calculation based on character class
 */

#include "../../core/AABB.h"
#include "../../core/Projectile.h"
#include "../../core/Rng.h"
#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"
#include "CharacterClass.h"
#include "MutantTypes.h"
#include "TPSWeapons.h"
#include "LockOnSystem.h"
#include "MeleeSystem.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace qe {
namespace game {
namespace tps {

// ── Combat Stats ─────────────────────────────────────────────────────────────

struct TPSCombatStats {
    int score = 0;
    int total_shots = 0;
    int total_hits = 0;
    int total_kills = 0;
    int melee_kills = 0;
    int ranged_kills = 0;
    int backstab_kills = 0;
    int critical_hits = 0;
    int parries_landed = 0;
    int highest_combo = 0;

    float accuracy() const {
        return total_shots > 0
            ? static_cast<float>(total_hits) / static_cast<float>(total_shots) * 100.0f
            : 0.0f;
    }

    void reset() {
        score = 0; total_shots = 0; total_hits = 0; total_kills = 0;
        melee_kills = 0; ranged_kills = 0; backstab_kills = 0;
        critical_hits = 0; parries_landed = 0; highest_combo = 0;
    }
};

// ── Hit Result ───────────────────────────────────────────────────────────────

struct HitResult {
    bool hit = false;
    bool killed = false;
    bool critical = false;
    float damage_dealt = 0.0f;
    int target_id = -1;
    int score_earned = 0;
};

// ── Combat System ────────────────────────────────────────────────────────────

class TPSCombatSystem {
    TPSCombatStats stats_;
    qe::core::Rng rng_{12345};

public:
    const TPSCombatStats& stats() const { return stats_; }
    TPSCombatStats& stats_mut() { return stats_; }

    /** Process a ranged shot against mutant targets.
     *  @pre weapon can fire
     */
    std::vector<HitResult> process_ranged_attack(
            const math::Vec3& origin,
            const math::Vec3& aim_direction,
            const TPSWeaponConfig& weapon,
            const CharacterStats& char_stats,
            const LockOnState& lock_on,
            std::vector<MutantInstance>& targets,
            std::vector<core::Projectile>& projectiles) {

        std::vector<HitResult> results;
        stats_.total_shots++;

        // Compute fire directions with lock-on wobble
        math::Vec3 fire_dir = aim_direction;
        if (lock_on.is_locked()) {
            fire_dir = lock_on.get_wobbled_aim(
                aim_direction, char_stats.lock_on_stability,
                false);
        }

        // Generate spread directions
        math::Vec3 up = math::Vec3::up();
        std::vector<math::Vec3> directions;
        float spread = weapon.spread_angle;

        for (int i = 0; i < weapon.pellet_count; ++i) {
            if (spread <= 0.0f && weapon.pellet_count == 1) {
                directions.push_back(fire_dir);
            } else {
                float pitch = random_range(-spread, spread);
                float yaw = random_range(-spread, spread);
                math::Vec3 right = fire_dir.cross(up);
                if (right.length_squared() < 1e-6f) right = math::Vec3::right();
                right = right.normalized();
                math::Quaternion qp = math::Quaternion::from_axis_angle(right, pitch);
                math::Quaternion qy = math::Quaternion::from_axis_angle(up, yaw);
                directions.push_back((qy * qp).normalized().rotate(fire_dir).normalized());
            }
        }

        // Process each pellet/ray
        for (const auto& dir : directions) {
            // Spawn projectile if not hitscan
            if (weapon.projectile_speed > 0.0f) {
                core::Projectile proj;
                proj.position = origin + dir * 0.5f;
                proj.velocity = dir * weapon.projectile_speed;
                proj.lifetime = weapon.range / weapon.projectile_speed;
                proj.radius = 0.1f;
                proj.damage = weapon.damage * char_stats.ranged_damage_mult;
                projectiles.push_back(proj);
            }

            // Hitscan check
            HitResult hr = hitscan(origin, dir, weapon, char_stats, targets);
            if (hr.hit) {
                stats_.total_hits++;
                if (hr.critical) stats_.critical_hits++;
                if (hr.killed) {
                    stats_.total_kills++;
                    stats_.ranged_kills++;
                    stats_.score += hr.score_earned;
                }
            }
            results.push_back(hr);
        }

        return results;
    }

    /** Process a melee hit against targets in arc.
     *  @pre melee state is in Active phase
     */
    std::vector<HitResult> process_melee_attack(
            const math::Vec3& attacker_pos,
            const math::Quaternion& attacker_rot,
            const MeleeCombatState& melee,
            const CharacterStats& char_stats,
            std::vector<MutantInstance>& targets,
            bool is_backstab = false) {

        std::vector<HitResult> results;
        const auto& move = melee.active_move();

        for (auto& target : targets) {
            if (!target.alive) continue;
            if (!is_in_melee_arc(attacker_pos, attacker_rot, target.position, move))
                continue;

            HitResult hr = apply_melee_damage(
                target, move, char_stats, is_backstab);

            results.push_back(hr);

            // Splash damage for jump attack
            if (move.splash_radius > 0.0f) {
                apply_splash_damage(
                    results, targets, target, hr.damage_dealt,
                    move.splash_radius);
                break;  // Splash only from first target hit
            }
        }

        return results;
    }

    /** Check projectile-mutant collisions and resolve damage. */
    std::vector<HitResult> process_projectile_collisions(
            std::vector<core::Projectile>& projectiles,
            std::vector<MutantInstance>& targets,
            const CharacterStats& char_stats,
            DamageCategory damage_cat) {

        std::vector<HitResult> results;
        (void)char_stats;  // Used for future damage scaling

        for (auto& proj : projectiles) {
            if (!proj.active) continue;
            core::AABB pb = proj.bounds();

            for (auto& target : targets) {
                if (!target.alive) continue;
                core::AABB tb = core::AABB::from_center(target.position, 1.0f);

                if (pb.intersects(tb)) {
                    bool is_explosive = (damage_cat == DamageCategory::Explosive);
                    bool is_energy = (damage_cat == DamageCategory::Energy);
                    float resist = target.get_resistance(is_explosive, is_energy, false);

                    float dmg = proj.damage * (1.0f - resist);
                    bool killed = target.take_damage(dmg, resist);

                    HitResult hr;
                    hr.hit = true;
                    hr.damage_dealt = dmg;
                    hr.target_id = target.id;
                    hr.killed = killed;
                    if (killed) {
                        hr.score_earned = target.config.score_value;
                        stats_.total_kills++;
                        stats_.ranged_kills++;
                        stats_.score += hr.score_earned;
                    }
                    results.push_back(hr);
                    stats_.total_hits++;

                    proj.active = false;
                    break;
                }
            }
        }

        return results;
    }

    /** Check if attacker is behind target (for backstab). */
    static bool is_behind_target(const math::Vec3& attacker_pos,
                                  const math::Vec3& target_pos,
                                  const math::Quaternion& target_rot) {
        math::Vec3 target_forward = target_rot.rotate(math::Vec3::forward());
        math::Vec3 to_attacker = (attacker_pos - target_pos);
        if (to_attacker.length_squared() < 0.01f) return false;
        to_attacker = to_attacker.normalized();

        float dot = target_forward.dot(to_attacker);
        return dot > 0.3f;
    }

    void reset() { stats_.reset(); }

private:
    /** Check if a target position is within the melee arc. */
    static bool is_in_melee_arc(const math::Vec3& attacker_pos,
                                 const math::Quaternion& attacker_rot,
                                 const math::Vec3& target_pos,
                                 const MeleeMoveConfig& move) {
        math::Vec3 to_target = target_pos - attacker_pos;
        float dist = to_target.length();
        if (dist > move.range) return false;

        if (dist > 0.01f) {
            math::Vec3 forward = attacker_rot.rotate(math::Vec3::forward());
            float dot = forward.dot(to_target.normalized());
            float half_arc = move.arc_angle * 0.5f;
            if (dot < std::cos(half_arc)) return false;
        }
        return true;
    }

    /** Compute and apply melee damage to a single target, updating stats. */
    HitResult apply_melee_damage(MutantInstance& target,
                                  const MeleeMoveConfig& move,
                                  const CharacterStats& char_stats,
                                  bool is_backstab) {
        float base_dmg = move.base_damage * char_stats.melee_damage_mult;
        if (is_backstab) {
            base_dmg = make_melee_move(MeleeMove::Backstab).base_damage
                     * char_stats.melee_damage_mult;
        }

        bool crit = roll_critical(char_stats.critical_chance);
        if (crit) base_dmg *= char_stats.critical_multiplier;

        float resist = target.get_resistance(false, false, true);
        float final_dmg = base_dmg * (1.0f - resist);

        HitResult hr;
        hr.hit = true;
        hr.critical = crit;
        hr.damage_dealt = final_dmg;
        hr.target_id = target.id;

        bool killed = target.take_damage(final_dmg, resist);
        hr.killed = killed;

        if (killed) {
            hr.score_earned = target.config.score_value;
            if (is_backstab) hr.score_earned = static_cast<int>(hr.score_earned * 1.5f);
            stats_.total_kills++;
            stats_.melee_kills++;
            if (is_backstab) stats_.backstab_kills++;
            stats_.score += hr.score_earned;
            if (crit) stats_.critical_hits++;
        }

        return hr;
    }

    /** Apply splash damage from an impact point to nearby targets. */
    void apply_splash_damage(std::vector<HitResult>& results,
                              std::vector<MutantInstance>& targets,
                              const MutantInstance& origin_target,
                              float base_damage,
                              float splash_radius) {
        for (auto& other : targets) {
            if (!other.alive || other.id == origin_target.id) continue;
            float splash_dist = other.position.distance_to(origin_target.position);
            if (splash_dist <= splash_radius) {
                float falloff = 1.0f - (splash_dist / splash_radius);
                float splash_dmg = base_damage * 0.5f * falloff;
                float splash_resist = other.get_resistance(false, false, true);
                bool splash_killed = other.take_damage(splash_dmg, splash_resist);

                HitResult shr;
                shr.hit = true;
                shr.damage_dealt = splash_dmg * (1.0f - splash_resist);
                shr.target_id = other.id;
                shr.killed = splash_killed;
                if (splash_killed) {
                    shr.score_earned = other.config.score_value;
                    stats_.total_kills++;
                    stats_.melee_kills++;
                    stats_.score += shr.score_earned;
                }
                results.push_back(shr);
            }
        }
    }

    HitResult hitscan(const math::Vec3& origin, const math::Vec3& dir,
                      const TPSWeaponConfig& weapon,
                      const CharacterStats& char_stats,
                      std::vector<MutantInstance>& targets) {
        float closest_t = weapon.range;
        int closest_idx = -1;

        for (size_t i = 0; i < targets.size(); ++i) {
            if (!targets[i].alive) continue;
            core::AABB bounds = core::AABB::from_center(targets[i].position, 1.0f);
            float t = 0;
            if (bounds.ray_intersect(origin, dir, t) && t < closest_t) {
                closest_t = t;
                closest_idx = static_cast<int>(i);
            }
        }

        HitResult hr;
        if (closest_idx < 0) return hr;

        hr.hit = true;
        hr.target_id = targets[closest_idx].id;

        float base_dmg = weapon.damage * char_stats.ranged_damage_mult;
        bool crit = roll_critical(char_stats.critical_chance);
        if (crit) {
            base_dmg *= char_stats.critical_multiplier;
            hr.critical = true;
        }

        bool is_explosive = (weapon.damage_category == DamageCategory::Explosive);
        bool is_energy = (weapon.damage_category == DamageCategory::Energy);
        float resist = targets[closest_idx].get_resistance(is_explosive, is_energy, false);
        float final_dmg = base_dmg * (1.0f - resist);
        hr.damage_dealt = final_dmg;

        bool killed = targets[closest_idx].take_damage(final_dmg, resist);
        hr.killed = killed;
        if (killed) {
            hr.score_earned = targets[closest_idx].config.score_value;
        }

        return hr;
    }

    bool roll_critical(float chance) {
        return random_range(0.0f, 1.0f) < chance;
    }

    float random_range(float lo, float hi) {
        return rng_.random_float(lo, hi);
    }
};

} // namespace tps
} // namespace game
} // namespace qe
