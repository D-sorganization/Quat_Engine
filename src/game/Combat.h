#pragma once
/**
 * @file Combat.h
 * @brief Shooting mechanics, projectile management, and collision resolution.
 *
 * Single Responsibility: manages the shoot → travel → collide → damage pipeline.
 * Reusable: not coupled to any specific entity arrangement.
 */

#include "../core/AABB.h"
#include "../core/Entity.h"
#include "../core/Projectile.h"
#include "../math/Vec3.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace qe {
namespace game {

struct CombatStats {
    int score       = 0;
    int total_shots = 0;
    int total_hits  = 0;

    float accuracy() const {
        return total_shots > 0
            ? static_cast<float>(total_hits) / total_shots * 100.0f
            : 0.0f;
    }

    void reset() { score = 0; total_shots = 0; total_hits = 0; }
};

struct CombatConfig {
    float projectile_speed = 40.0f;
    float fire_rate        = 0.15f;  // Seconds between shots
    float projectile_damage = 25.0f;
    float projectile_lifetime = 3.0f;
    float projectile_radius  = 0.08f;
    int   kill_score         = 100;
};

/** Fire a projectile and do instant hitscan. */
inline void shoot(const math::Vec3& origin, const math::Vec3& direction,
                  const CombatConfig& config,
                  std::vector<core::Projectile>& projectiles,
                  std::vector<core::Entity>& entities,
                  CombatStats& stats) {
    stats.total_shots++;

    // Spawn projectile
    core::Projectile proj;
    proj.position = origin + direction * 0.5f;
    proj.velocity = direction * config.projectile_speed;
    proj.lifetime = config.projectile_lifetime;
    proj.radius   = config.projectile_radius;
    proj.damage   = config.projectile_damage;
    projectiles.push_back(proj);

    // Instant hitscan
    float closest_t = 999.0f;
    int closest_id = -1;

    for (size_t i = 0; i < entities.size(); ++i) {
        if (!entities[i].alive) continue;
        float t = 0;
        if (entities[i].world_bounds().ray_intersect(origin, direction, t)) {
            if (t < closest_t) {
                closest_t = t;
                closest_id = static_cast<int>(i);
            }
        }
    }

    if (closest_id >= 0) {
        stats.total_hits++;
        if (entities[closest_id].take_damage(config.projectile_damage)) {
            stats.score += config.kill_score;
        }
    }
}

/** Update all projectiles and remove dead ones. */
inline void update_projectiles(std::vector<core::Projectile>& projectiles, float dt) {
    for (auto& p : projectiles) p.update(dt);
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [](const core::Projectile& p) { return !p.is_alive(); }),
        projectiles.end());
}

/** Check projectile-entity AABB collisions. */
inline void check_projectile_collisions(
        std::vector<core::Projectile>& projectiles,
        std::vector<core::Entity>& entities,
        CombatStats& stats, int kill_score = 100) {
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        core::AABB pb = proj.bounds();

        for (auto& ent : entities) {
            if (!ent.alive) continue;
            if (pb.intersects(ent.world_bounds())) {
                if (ent.take_damage(proj.damage)) {
                    stats.score += kill_score;
                }
                stats.total_hits++;
                proj.active = false;
                break;
            }
        }
    }
}

} // namespace game
} // namespace qe
