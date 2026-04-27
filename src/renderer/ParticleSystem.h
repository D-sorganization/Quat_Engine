// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file ParticleSystem.h
 * @brief Quaternion-driven particle system for visual effects.
 *
 * Showcases quaternion rotation in a practical visual-effects context:
 *   - Particles carry orientation as a quaternion, updated each frame via
 *     axis-angle composition (no Euler angles, no gimbal lock).
 *   - Emitter shapes (Cone, Ring) use quaternion rotation to distribute
 *     particle directions and positions in 3D space.
 *   - Deterministic xorshift RNG allows reproducible effect playback.
 *
 * Header-only. Depends only on Vec3.h and Quaternion.h from qe::math.
 */

#include "../core/Rng.h"
#include "../math/Constants.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qe {
namespace renderer {

// ============================================================================
//  Particle
// ============================================================================

struct Particle {
    math::Vec3 position{0, 0, 0};
    math::Vec3 velocity{0, 0, 0};
    math::Quaternion rotation = math::Quaternion::identity();
    math::Vec3 spin_axis{0, 1, 0};   ///< Axis of angular rotation.
    float spin_speed = 0.0f;          ///< Radians per second.
    math::Vec3 color{1, 1, 1};
    math::Vec3 color_end{0.2f, 0.1f, 0.0f};
    float size = 0.1f;
    float size_end = 0.0f;
    float lifetime = 1.0f;
    float age = 0.0f;
    float gravity = -9.8f;
    bool alive = true;

    /** Advance the particle by dt seconds. */
    void update(float dt) {
        if (!alive) return;
        age += dt;
        if (age >= lifetime) { alive = false; return; }

        // Gravity affects y-velocity.
        velocity.y += gravity * dt;
        position += velocity * dt;

        // Quaternion-based spin: compose incremental rotation.
        float angle_dt = spin_speed * dt;
        if (angle_dt > 1e-6f) {
            math::Quaternion spin_increment =
                math::Quaternion::from_axis_angle(spin_axis, angle_dt);
            rotation = (spin_increment * rotation).normalized();
        }
    }

    /** Normalized age in [0, 1]. */
    float progress() const {
        return lifetime > 0.0f ? age / lifetime : 1.0f;
    }

    /** Color linearly interpolated from start to end over lifetime. */
    math::Vec3 current_color() const {
        return color.lerp(color_end, progress());
    }

    /** Size linearly interpolated from start to end over lifetime. */
    float current_size() const {
        return size + (size_end - size) * progress();
    }
};

// ============================================================================
//  Emitter Configuration
// ============================================================================

enum class EmitterShape { Point, Sphere, Cone, Ring };

struct EmitterConfig {
    EmitterShape shape = EmitterShape::Point;
    math::Vec3 position{0, 0, 0};
    math::Quaternion orientation = math::Quaternion::identity();

    // Spawn ranges (randomized between min and max).
    float speed_min = 2.0f;
    float speed_max = 8.0f;
    float lifetime_min = 0.5f;
    float lifetime_max = 1.5f;
    float size_min = 0.05f;
    float size_max = 0.15f;
    float size_end_min = 0.0f;
    float size_end_max = 0.02f;
    float gravity = -9.8f;
    float spin_speed_min = 0.0f;
    float spin_speed_max = 10.0f;
    float cone_angle = 0.5f;     ///< Half-angle in radians (Cone shape).
    float ring_radius = 1.0f;    ///< Radius (Ring shape).

    math::Vec3 color_start{1.0f, 0.8f, 0.3f};
    math::Vec3 color_end{0.3f, 0.1f, 0.0f};

    int burst_count = 20;        ///< Particles per burst.
};

// ============================================================================
//  Particle System
// ============================================================================

class ParticleSystem {
public:
    static constexpr int MAX_PARTICLES = 2048;

    // --- Public interface ---

    /** Burst-emit particles according to config. */
    void emit(const EmitterConfig& config) {
        for (int i = 0; i < config.burst_count; ++i) {
            if (static_cast<int>(particles_.size()) >= MAX_PARTICLES) break;

            Particle p;
            p.gravity = config.gravity;
            p.lifetime = random_float(config.lifetime_min, config.lifetime_max);
            p.size = random_float(config.size_min, config.size_max);
            p.size_end = random_float(config.size_end_min, config.size_end_max);
            p.color = config.color_start;
            p.color_end = config.color_end;

            // Random spin (quaternion showcase: arbitrary axis).
            p.spin_axis = random_spin_axis();
            p.spin_speed = random_float(config.spin_speed_min, config.spin_speed_max);
            p.rotation = math::Quaternion::identity();

            float speed = random_float(config.speed_min, config.speed_max);

            switch (config.shape) {
                case EmitterShape::Point: {
                    p.position = config.position;
                    math::Vec3 dir = random_direction_in_cone(
                        config.orientation, config.cone_angle);
                    p.velocity = dir * speed;
                    break;
                }
                case EmitterShape::Sphere: {
                    math::Vec3 dir = random_unit_vector();
                    p.position = config.position + dir * config.ring_radius;
                    p.velocity = dir * speed;
                    break;
                }
                case EmitterShape::Cone: {
                    p.position = config.position;
                    // QUATERNION SHOWCASE: orient the cone axis via quaternion,
                    // then pick random directions within the half-angle.
                    math::Vec3 dir = random_direction_in_cone(
                        config.orientation, config.cone_angle);
                    p.velocity = dir * speed;
                    break;
                }
                case EmitterShape::Ring: {
                    // QUATERNION SHOWCASE: distribute particles around a ring
                    // using axis-angle rotation.
                    float angle = random_float(0.0f, 2.0f * qe::math::PI);
                    math::Quaternion ring_rot =
                        math::Quaternion::from_axis_angle(math::Vec3(0, 1, 0), angle);
                    math::Vec3 offset = ring_rot.rotate(
                        math::Vec3(config.ring_radius, 0, 0));
                    // Rotate the offset into emitter space.
                    offset = config.orientation.rotate(offset);
                    p.position = config.position + offset;

                    // Velocity outward from the ring centre.
                    math::Vec3 outward = offset.length_squared() > 1e-8f
                        ? offset.normalized()
                        : math::Vec3(1, 0, 0);
                    p.velocity = outward * speed;
                    break;
                }
            }

            particles_.push_back(p);
        }
    }

    /** Advance every particle and remove the dead ones. */
    void update(float dt) {
        for (auto& p : particles_) {
            p.update(dt);
        }
        // Erase-remove dead particles.
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                           [](const Particle& p) { return !p.alive; }),
            particles_.end());
    }

    /** Read-only access to live particles. */
    const std::vector<Particle>& particles() const { return particles_; }

    /** Number of currently alive particles. */
    int alive_count() const { return static_cast<int>(particles_.size()); }

    /** Remove all particles immediately. */
    void clear() { particles_.clear(); }

    // --- Preset emitter configs (reusable) ---

    static EmitterConfig preset_explosion() {
        EmitterConfig c;
        c.shape = EmitterShape::Sphere;
        c.speed_min = 5.0f;
        c.speed_max = 15.0f;
        c.lifetime_min = 0.4f;
        c.lifetime_max = 1.2f;
        c.size_min = 0.1f;
        c.size_max = 0.3f;
        c.size_end_min = 0.0f;
        c.size_end_max = 0.05f;
        c.gravity = -4.0f;
        c.spin_speed_min = 2.0f;
        c.spin_speed_max = 15.0f;
        c.color_start = math::Vec3(1.0f, 0.6f, 0.1f);
        c.color_end = math::Vec3(0.3f, 0.05f, 0.0f);
        c.burst_count = 80;
        c.ring_radius = 0.5f;
        return c;
    }

    static EmitterConfig preset_muzzle_flash() {
        EmitterConfig c;
        c.shape = EmitterShape::Cone;
        c.speed_min = 10.0f;
        c.speed_max = 25.0f;
        c.lifetime_min = 0.05f;
        c.lifetime_max = 0.15f;
        c.size_min = 0.02f;
        c.size_max = 0.06f;
        c.size_end_min = 0.0f;
        c.size_end_max = 0.0f;
        c.gravity = 0.0f;
        c.cone_angle = 0.2f;
        c.spin_speed_min = 5.0f;
        c.spin_speed_max = 20.0f;
        c.color_start = math::Vec3(1.0f, 1.0f, 0.8f);
        c.color_end = math::Vec3(1.0f, 0.5f, 0.0f);
        c.burst_count = 15;
        return c;
    }

    static EmitterConfig preset_hit_sparks() {
        EmitterConfig c;
        c.shape = EmitterShape::Cone;
        c.speed_min = 3.0f;
        c.speed_max = 12.0f;
        c.lifetime_min = 0.2f;
        c.lifetime_max = 0.6f;
        c.size_min = 0.01f;
        c.size_max = 0.04f;
        c.size_end_min = 0.0f;
        c.size_end_max = 0.0f;
        c.gravity = -9.8f;
        c.cone_angle = 0.8f;
        c.spin_speed_min = 10.0f;
        c.spin_speed_max = 30.0f;
        c.color_start = math::Vec3(1.0f, 0.9f, 0.5f);
        c.color_end = math::Vec3(0.6f, 0.2f, 0.0f);
        c.burst_count = 30;
        return c;
    }

    static EmitterConfig preset_death_burst() {
        EmitterConfig c;
        c.shape = EmitterShape::Sphere;
        c.speed_min = 2.0f;
        c.speed_max = 8.0f;
        c.lifetime_min = 0.8f;
        c.lifetime_max = 2.0f;
        c.size_min = 0.08f;
        c.size_max = 0.2f;
        c.size_end_min = 0.0f;
        c.size_end_max = 0.02f;
        c.gravity = -2.0f;
        c.spin_speed_min = 1.0f;
        c.spin_speed_max = 8.0f;
        c.color_start = math::Vec3(0.8f, 0.2f, 0.2f);
        c.color_end = math::Vec3(0.1f, 0.0f, 0.0f);
        c.burst_count = 50;
        c.ring_radius = 0.3f;
        return c;
    }

    static EmitterConfig preset_powerup_collect() {
        EmitterConfig c;
        c.shape = EmitterShape::Ring;
        c.speed_min = 1.0f;
        c.speed_max = 4.0f;
        c.lifetime_min = 0.5f;
        c.lifetime_max = 1.0f;
        c.size_min = 0.04f;
        c.size_max = 0.1f;
        c.size_end_min = 0.0f;
        c.size_end_max = 0.01f;
        c.gravity = 1.0f;   // Float upward.
        c.ring_radius = 0.8f;
        c.spin_speed_min = 3.0f;
        c.spin_speed_max = 12.0f;
        c.color_start = math::Vec3(0.3f, 1.0f, 0.5f);
        c.color_end = math::Vec3(1.0f, 1.0f, 1.0f);
        c.burst_count = 25;
        return c;
    }

private:
    std::vector<Particle> particles_;

    // Deterministic RNG for reproducible effects.
    qe::core::Rng rng_{12345};

    /** Uniform float in [min, max]. */
    float random_float(float min, float max) {
        return rng_.random_float(min, max);
    }

    /** Generate a random direction within a cone defined by orientation and half-angle.
     *
     *  Quaternion showcase: the emitter's orientation quaternion defines the
     *  cone axis, then a random local direction is rotated into world space.
     */
    math::Vec3 random_direction_in_cone(const math::Quaternion& orientation,
                                        float cone_angle) {
        // Sample uniformly within a spherical cap.
        float z = random_float(std::cos(cone_angle), 1.0f);
        float phi = random_float(0.0f, 2.0f * qe::math::PI);
        float r = std::sqrt(1.0f - z * z);
        math::Vec3 local_dir(r * std::cos(phi), r * std::sin(phi), z);
        // Rotate local direction into world space by emitter orientation.
        return orientation.rotate(local_dir);
    }

    /** Random unit vector (uniform on sphere). */
    math::Vec3 random_unit_vector() {
        // Rejection-free: use spherical coordinates.
        float z = random_float(-1.0f, 1.0f);
        float phi = random_float(0.0f, 2.0f * qe::math::PI);
        float r = std::sqrt(1.0f - z * z);
        return math::Vec3(r * std::cos(phi), r * std::sin(phi), z);
    }

    /** Random normalized axis for particle spin. */
    math::Vec3 random_spin_axis() {
        // Generate random vector, normalize. Retry on degenerate case.
        for (int attempt = 0; attempt < 4; ++attempt) {
            math::Vec3 v(random_float(-1, 1), random_float(-1, 1), random_float(-1, 1));
            float len2 = v.length_squared();
            if (len2 > 0.01f) {
                return v.normalized();
            }
        }
        return math::Vec3(0, 1, 0); // Fallback.
    }
};

} // namespace renderer
} // namespace qe
