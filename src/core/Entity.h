// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

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

    /** Get world-space AABB.
     *  Transforms the local-space bounding box to world coordinates using position and scale.
     *  @return World-space AABB
     *  @complexity O(1) - only performs 6 scalar multiplications and additions
     */
    AABB world_bounds() const {
        return local_bounds.transformed(position, scale);
    }

    /** Apply damage to this entity.
     *  Reduces health, triggers hit flash visual feedback, and handles entity death state.
     *  @param dmg Damage amount (will be subtracted from health)
     *  @return True if this damage killed the entity (health <= 0), false otherwise
     *  @complexity O(1) - constant time damage application and state update
     */
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

    /** Update entity state for a given delta time.
     *  Handles hit flash animation decay, death animation timer, and respawn logic.
     *  Call once per frame with the frame delta time.
     *  @param dt Delta time in seconds
     *  @complexity O(1) - only updates internal timers and conditionals
     */
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

    /** Respawn the entity at its original spawn position.
     *  Restores health to maximum, resets death timers, and marks entity as alive.
     *  Used when respawn_delay has elapsed or when explicitly respawning.
     *  @complexity O(1) - only updates member variables
     */
    void respawn() {
        position = spawn_position;
        health = max_health;
        alive = true;
        death_timer = 0.0f;
        hit_flash = 0.0f;
    }

    /** Get health as a normalized 0-1 fraction.
     *  Useful for rendering health bars and UI displays.
     *  Returns 0.0 if max_health is 0 to avoid division by zero.
     *  @return Health fraction: 0.0 (dead) to 1.0 (full health)
     *  @complexity O(1) - single division operation
     */
    float health_fraction() const {
        return max_health > 0.0f ? health / max_health : 0.0f;
    }
};

} // namespace core
} // namespace qe
