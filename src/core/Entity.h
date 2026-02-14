#pragma once
/**
 * @file Entity.h
 * @brief Game entity with health, AABB collision, damage, and respawn.
 */

#include "../math/Quaternion.h"
#include "../math/Vec3.h"
#include "AABB.h"

namespace qe {
namespace core {

struct Entity {
    // Transform
    math::Vec3       position{0, 0, 0};
    math::Quaternion rotation = math::Quaternion::identity();
    math::Vec3       scale{1, 1, 1};

    // Collision (local-space, centered at origin)
    AABB local_bounds = AABB::from_center(math::Vec3::zero(), 0.5f);

    // Health
    float health     = 100.0f;
    float max_health = 100.0f;
    bool  alive      = true;

    // Respawn
    float respawn_timer = 0.0f;
    float respawn_delay = 3.0f;
    math::Vec3 spawn_position{0, 0, 0};

    // Visual feedback
    float hit_flash = 0.0f;     // > 0 means recently hit, decays to 0
    float death_timer = 0.0f;   // Shrink + spin on death

    // Identity
    int id = 0;
    bool destructible = true;

    /** Get world-space AABB. */
    AABB world_bounds() const {
        return local_bounds.transformed(position, scale);
    }

    /** Apply damage. Returns true if this killed the entity. */
    bool take_damage(float dmg) {
        if (!alive || !destructible) return false;

        health -= dmg;
        hit_flash = 0.3f;  // Flash for 0.3 seconds

        if (health <= 0.0f) {
            health = 0.0f;
            alive = false;
            death_timer = 0.0f;
            return true;
        }
        return false;
    }

    /** Update entity state (hit flash decay, death animation, respawn). */
    void update(float dt) {
        if (hit_flash > 0.0f) {
            hit_flash -= dt;
            if (hit_flash < 0.0f) hit_flash = 0.0f;
        }

        if (!alive) {
            death_timer += dt;
            if (death_timer >= respawn_delay) {
                respawn();
            }
        }
    }

    /** Respawn at original position with full health. */
    void respawn() {
        position = spawn_position;
        health = max_health;
        alive = true;
        death_timer = 0.0f;
        hit_flash = 0.0f;
    }

    /** Health as 0-1 fraction. */
    float health_fraction() const {
        return max_health > 0.0f ? health / max_health : 0.0f;
    }
};

} // namespace core
} // namespace qe
