// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_particles.cpp
 * @brief Tests for the quaternion-driven ParticleSystem.
 *
 * Validates:
 *   - Particle: lifetime, position update, gravity, color/size interpolation, spin
 *   - ParticleSystem: emit burst count, update removes dead, clear, MAX_PARTICLES
 *   - EmitterConfig presets: reasonable value ranges
 *   - Cone emitter: directions stay within cone angle
 *   - alive_count tracking
 */

#include "test_framework.h"

#include "../src/renderer/ParticleSystem.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>


using namespace qe::math;
using namespace qe::renderer;

// qe::math::PI is available via `using namespace qe::math` above
constexpr float EPS = 1e-4f;

// ============================================================================
//  Particle Tests
// ============================================================================

void test_particle_starts_alive() {
    Particle p;
    ASSERT_TRUE(p.alive);
    ASSERT_NEAR(p.age, 0.0f, EPS);
}

void test_particle_dies_after_lifetime() {
    Particle p;
    p.lifetime = 1.0f;
    p.gravity = 0.0f;

    // Update just under the lifetime -- still alive.
    p.update(0.9f);
    ASSERT_TRUE(p.alive);

    // Push past lifetime -- should die.
    p.update(0.2f);
    ASSERT_TRUE(!p.alive);
}

void test_particle_position_updates_with_velocity() {
    Particle p;
    p.velocity = Vec3(1.0f, 0.0f, 0.0f);
    p.gravity = 0.0f;
    p.lifetime = 10.0f;

    p.update(1.0f);
    ASSERT_NEAR(p.position.x, 1.0f, EPS);
    ASSERT_NEAR(p.position.y, 0.0f, EPS);
    ASSERT_NEAR(p.position.z, 0.0f, EPS);
}

void test_particle_gravity_affects_y_velocity() {
    Particle p;
    p.velocity = Vec3(0.0f, 0.0f, 0.0f);
    p.gravity = -9.8f;
    p.lifetime = 10.0f;

    p.update(1.0f);
    // After 1s: velocity.y = -9.8, position.y = -9.8 * 1.0 = -9.8
    ASSERT_NEAR(p.velocity.y, -9.8f, EPS);
    ASSERT_NEAR(p.position.y, -9.8f, 0.1f);
}

void test_particle_color_interpolation() {
    Particle p;
    p.color = Vec3(1.0f, 1.0f, 1.0f);
    p.color_end = Vec3(0.0f, 0.0f, 0.0f);
    p.lifetime = 2.0f;
    p.gravity = 0.0f;

    // At age 0: full start color.
    Vec3 c0 = p.current_color();
    ASSERT_NEAR(c0.x, 1.0f, EPS);
    ASSERT_NEAR(c0.y, 1.0f, EPS);
    ASSERT_NEAR(c0.z, 1.0f, EPS);

    // Advance to midpoint.
    p.update(1.0f);
    Vec3 cm = p.current_color();
    ASSERT_NEAR(cm.x, 0.5f, EPS);
    ASSERT_NEAR(cm.y, 0.5f, EPS);
    ASSERT_NEAR(cm.z, 0.5f, EPS);
}

void test_particle_size_interpolation() {
    Particle p;
    p.size = 1.0f;
    p.size_end = 0.0f;
    p.lifetime = 2.0f;
    p.gravity = 0.0f;

    ASSERT_NEAR(p.current_size(), 1.0f, EPS);

    p.update(1.0f); // halfway
    ASSERT_NEAR(p.current_size(), 0.5f, EPS);
}

void test_particle_rotation_changes_with_spin() {
    Particle p;
    p.spin_axis = Vec3(0, 1, 0);
    p.spin_speed = PI; // 180 deg/s
    p.lifetime = 10.0f;
    p.gravity = 0.0f;

    Quaternion original = p.rotation;

    p.update(1.0f); // 1 second -> 180 deg rotation

    // The rotation should have changed.
    ASSERT_TRUE(!p.rotation.approx_equal(original, EPS));

    // Verify it actually applied a rotation around Y.
    // Rotating (1,0,0) by 180 deg around Y should give (-1,0,0).
    Vec3 rotated = p.rotation.rotate(Vec3(1, 0, 0));
    ASSERT_NEAR(rotated.x, -1.0f, 0.05f);
    ASSERT_NEAR(rotated.y, 0.0f, 0.05f);
}

void test_particle_dead_does_not_update() {
    Particle p;
    p.alive = false;
    p.velocity = Vec3(10, 0, 0);
    p.gravity = 0.0f;

    p.update(1.0f);
    ASSERT_NEAR(p.position.x, 0.0f, EPS); // should not have moved
}

// ============================================================================
//  ParticleSystem Tests
// ============================================================================

void test_emit_creates_correct_burst_count() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = 30;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == 30);
}

void test_update_removes_dead_particles() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = 10;
    config.lifetime_min = 0.1f;
    config.lifetime_max = 0.1f;
    config.gravity = 0.0f;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == 10);

    // Update past their lifetime.
    ps.update(0.2f);
    ASSERT_TRUE(ps.alive_count() == 0);
}

void test_clear_empties_all() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = 50;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == 50);

    ps.clear();
    ASSERT_TRUE(ps.alive_count() == 0);
    ASSERT_TRUE(ps.particles().empty());
}

void test_alive_count_tracks_correctly() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = 20;
    config.lifetime_min = 1.0f;
    config.lifetime_max = 1.0f;
    config.gravity = 0.0f;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == 20);

    // Small dt -- all still alive.
    ps.update(0.01f);
    ASSERT_TRUE(ps.alive_count() == 20);

    // Past lifetime -- all dead.
    ps.update(2.0f);
    ASSERT_TRUE(ps.alive_count() == 0);
}

void test_max_particles_limit() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = ParticleSystem::MAX_PARTICLES + 100;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() <= ParticleSystem::MAX_PARTICLES);
}

void test_multiple_emits_respect_cap() {
    ParticleSystem ps;
    EmitterConfig config;
    config.burst_count = ParticleSystem::MAX_PARTICLES;
    config.lifetime_min = 10.0f;
    config.lifetime_max = 10.0f;

    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == ParticleSystem::MAX_PARTICLES);

    // Second emit should add nothing -- already at capacity.
    config.burst_count = 100;
    ps.emit(config);
    ASSERT_TRUE(ps.alive_count() == ParticleSystem::MAX_PARTICLES);
}

// ============================================================================
//  Cone Emitter Direction Tests
// ============================================================================

void test_cone_emitter_directions_within_angle() {
    ParticleSystem ps;
    EmitterConfig config;
    config.shape = EmitterShape::Cone;
    config.cone_angle = 0.3f; // ~17 degrees
    config.orientation = Quaternion::identity(); // cone along +Z
    config.burst_count = 200;
    config.speed_min = 1.0f;
    config.speed_max = 1.0f;
    config.lifetime_min = 10.0f;
    config.lifetime_max = 10.0f;
    config.gravity = 0.0f;

    ps.emit(config);

    Vec3 cone_axis(0, 0, 1); // identity orientation -> +Z is the cone axis
    float max_allowed_cos = std::cos(config.cone_angle);

    for (const auto& p : ps.particles()) {
        Vec3 dir = p.velocity.normalized();
        float cos_angle = dir.dot(cone_axis);
        // The direction should be within the cone (cos >= cos(cone_angle)).
        ASSERT_TRUE(cos_angle >= max_allowed_cos - 0.001f);
    }
}

void test_cone_emitter_oriented() {
    // Rotate the cone to face +X instead of +Z.
    ParticleSystem ps;
    EmitterConfig config;
    config.shape = EmitterShape::Cone;
    config.cone_angle = 0.2f;
    // Rotate +Z to +X: +90 deg around Y.
    config.orientation = Quaternion::from_axis_angle(Vec3(0, 1, 0), PI / 2.0f);
    config.burst_count = 100;
    config.speed_min = 5.0f;
    config.speed_max = 5.0f;
    config.lifetime_min = 10.0f;
    config.lifetime_max = 10.0f;
    config.gravity = 0.0f;

    ps.emit(config);

    // +90 deg Y rotation maps +Z to +X.
    Vec3 cone_axis = config.orientation.rotate(Vec3(0, 0, 1));
    float max_allowed_cos = std::cos(config.cone_angle);

    for (const auto& p : ps.particles()) {
        Vec3 dir = p.velocity.normalized();
        float cos_angle = dir.dot(cone_axis);
        ASSERT_TRUE(cos_angle >= max_allowed_cos - 0.01f);
    }
}

// ============================================================================
//  Ring Emitter Test
// ============================================================================

void test_ring_emitter_positions_on_ring() {
    ParticleSystem ps;
    EmitterConfig config;
    config.shape = EmitterShape::Ring;
    config.ring_radius = 2.0f;
    config.position = Vec3(0, 0, 0);
    config.orientation = Quaternion::identity();
    config.burst_count = 100;
    config.lifetime_min = 10.0f;
    config.lifetime_max = 10.0f;
    config.gravity = 0.0f;

    ps.emit(config);

    for (const auto& p : ps.particles()) {
        // Particle position should be approximately ring_radius from center
        // (in the XZ plane, since the ring rotates around Y).
        float dist = p.position.length();
        ASSERT_NEAR(dist, config.ring_radius, 0.1f);
    }
}

// ============================================================================
//  Sphere Emitter Test
// ============================================================================

void test_sphere_emitter_velocity_outward() {
    ParticleSystem ps;
    EmitterConfig config;
    config.shape = EmitterShape::Sphere;
    config.ring_radius = 1.0f;
    config.position = Vec3(0, 0, 0);
    config.burst_count = 100;
    config.speed_min = 5.0f;
    config.speed_max = 5.0f;
    config.lifetime_min = 10.0f;
    config.lifetime_max = 10.0f;
    config.gravity = 0.0f;

    ps.emit(config);

    for (const auto& p : ps.particles()) {
        // Velocity should point roughly outward from center.
        Vec3 to_particle = p.position.normalized();
        Vec3 vel_dir = p.velocity.normalized();
        float dot = to_particle.dot(vel_dir);
        // Should be positive (outward).
        ASSERT_TRUE(dot > 0.9f);
    }
}

// ============================================================================
//  Preset Config Tests
// ============================================================================

void test_preset_explosion_values() {
    auto c = ParticleSystem::preset_explosion();
    ASSERT_TRUE(c.shape == EmitterShape::Sphere);
    ASSERT_TRUE(c.burst_count > 0);
    ASSERT_TRUE(c.speed_max > c.speed_min);
    ASSERT_TRUE(c.lifetime_max > 0.0f);
    ASSERT_TRUE(c.spin_speed_max > 0.0f);
}

void test_preset_muzzle_flash_values() {
    auto c = ParticleSystem::preset_muzzle_flash();
    ASSERT_TRUE(c.shape == EmitterShape::Cone);
    ASSERT_TRUE(c.burst_count > 0);
    ASSERT_TRUE(c.speed_max > c.speed_min);
    ASSERT_TRUE(c.lifetime_max > 0.0f);
    ASSERT_TRUE(c.cone_angle > 0.0f);
    ASSERT_TRUE(c.gravity == 0.0f); // muzzle flash has no gravity
}

void test_preset_hit_sparks_values() {
    auto c = ParticleSystem::preset_hit_sparks();
    ASSERT_TRUE(c.shape == EmitterShape::Cone);
    ASSERT_TRUE(c.burst_count > 0);
    ASSERT_TRUE(c.speed_max > c.speed_min);
    ASSERT_TRUE(c.gravity < 0.0f); // sparks fall
}

void test_preset_death_burst_values() {
    auto c = ParticleSystem::preset_death_burst();
    ASSERT_TRUE(c.shape == EmitterShape::Sphere);
    ASSERT_TRUE(c.burst_count > 0);
    ASSERT_TRUE(c.lifetime_max > c.lifetime_min);
}

void test_preset_powerup_collect_values() {
    auto c = ParticleSystem::preset_powerup_collect();
    ASSERT_TRUE(c.shape == EmitterShape::Ring);
    ASSERT_TRUE(c.burst_count > 0);
    ASSERT_TRUE(c.gravity > 0.0f); // floats upward
    ASSERT_TRUE(c.ring_radius > 0.0f);
}

// ============================================================================
//  Progress Edge Cases
// ============================================================================

void test_particle_progress_zero_lifetime() {
    Particle p;
    p.lifetime = 0.0f;
    // Zero lifetime should return progress 1.0 (fully done).
    ASSERT_NEAR(p.progress(), 1.0f, EPS);
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "=== QuatEngine Particle System Tests ===" << std::endl;

    std::cout << "\n--- Particle ---" << std::endl;
    RUN_TEST(test_particle_starts_alive);
    RUN_TEST(test_particle_dies_after_lifetime);
    RUN_TEST(test_particle_position_updates_with_velocity);
    RUN_TEST(test_particle_gravity_affects_y_velocity);
    RUN_TEST(test_particle_color_interpolation);
    RUN_TEST(test_particle_size_interpolation);
    RUN_TEST(test_particle_rotation_changes_with_spin);
    RUN_TEST(test_particle_dead_does_not_update);

    std::cout << "\n--- ParticleSystem ---" << std::endl;
    RUN_TEST(test_emit_creates_correct_burst_count);
    RUN_TEST(test_update_removes_dead_particles);
    RUN_TEST(test_clear_empties_all);
    RUN_TEST(test_alive_count_tracks_correctly);
    RUN_TEST(test_max_particles_limit);
    RUN_TEST(test_multiple_emits_respect_cap);

    std::cout << "\n--- Emitter Shapes ---" << std::endl;
    RUN_TEST(test_cone_emitter_directions_within_angle);
    RUN_TEST(test_cone_emitter_oriented);
    RUN_TEST(test_ring_emitter_positions_on_ring);
    RUN_TEST(test_sphere_emitter_velocity_outward);

    std::cout << "\n--- Presets ---" << std::endl;
    RUN_TEST(test_preset_explosion_values);
    RUN_TEST(test_preset_muzzle_flash_values);
    RUN_TEST(test_preset_hit_sparks_values);
    RUN_TEST(test_preset_death_burst_values);
    RUN_TEST(test_preset_powerup_collect_values);

    std::cout << "\n--- Edge Cases ---" << std::endl;
    RUN_TEST(test_particle_progress_zero_lifetime);

    return TEST_REPORT();
}
