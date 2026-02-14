#pragma once
/**
 * @file Projectile.h
 * @brief Simple 3D projectile with velocity, lifetime, and visual properties.
 */

#include "../math/Vec3.h"
#include "AABB.h"

namespace qe {
namespace core {

struct Projectile {
    math::Vec3 position{0, 0, 0};
    math::Vec3 velocity{0, 0, 0};
    float lifetime     = 3.0f;    // Seconds until despawn
    float age          = 0.0f;
    float radius       = 0.1f;    // For collision AABB
    float damage       = 25.0f;
    bool  active       = true;

    // Visual
    float brightness   = 1.0f;    // Fades with age
    math::Vec3 color{1.0f, 0.9f, 0.3f};  // Yellow-white

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

    bool is_alive() const { return active && age < lifetime; }

    AABB bounds() const {
        return AABB::from_center(position, radius);
    }
};

} // namespace core
} // namespace qe
