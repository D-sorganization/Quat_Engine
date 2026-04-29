// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file Projectile.h
 * @brief Simple 3D projectile with velocity, lifetime, and visual properties.
 */

#include "../math/Vec3.h"
#include "AABB.h"

namespace qe {
namespace core {

/**
 * Transient world-space projectile state used by weapon and collision systems.
 *
 * The projectile owns only simple kinematic state: callers update it once per
 * frame, query whether it is still active, and derive a conservative AABB for
 * broad-phase collision checks. Instances are intentionally value-like so waves
 * or object pools can store them in contiguous containers.
 */
struct Projectile {
    /** Current world-space origin of the projectile. */
    math::Vec3 position{0, 0, 0};

    /** World-space velocity applied linearly during update(). */
    math::Vec3 velocity{0, 0, 0};

    /** Maximum lifetime in seconds before automatic deactivation. */
    float lifetime     = 3.0f;    // Seconds until despawn

    /** Elapsed lifetime in seconds. */
    float age          = 0.0f;

    /** Collision radius used to construct the broad-phase AABB. */
    float radius       = 0.1f;    // For collision AABB

    /** Damage delivered by gameplay systems when a hit is accepted. */
    float damage       = 25.0f;

    /** Whether the projectile should still be updated and tested. */
    bool  active       = true;

    // Visual
    /** Intensity multiplier used by rendering effects. */
    float brightness   = 1.0f;    // Fades with age

    /** RGB tint consumed by projectile rendering. */
    math::Vec3 color{1.0f, 0.9f, 0.3f};  // Yellow-white

    /**
     * Advance position, age, and visual fade by dt seconds.
     *
     * Deactivates the projectile when it reaches its configured lifetime or
     * drops below the simple world-floor cutoff used by the demo scenes.
     */
    void update(float dt) {
        if (!active) return;

        position += velocity * dt;
        age += dt;
        brightness = 1.0f - (age / lifetime) * 0.5f;  // Dim over time

        // Despawn conditions
        if (age >= lifetime || position.y < -1.0f) {
            active = false;
        }
    }

    /** Return true while the projectile is active and below its lifetime. */
    bool is_alive() const { return active && age < lifetime; }

    /** Build the projectile's conservative world-space collision bounds. */
    AABB bounds() const {
        return AABB::from_center(position, radius);
    }
};

} // namespace core
} // namespace qe
