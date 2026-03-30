/**
 * @file test_game.cpp
 * @brief Tests for game components: AABB, Entity, Projectile, Combat.
 *
 * Validates:
 *   - AABB: containment, overlap, ray intersection (slab method)
 *   - Entity: damage, kill, respawn cycle
 *   - Projectile: lifetime, movement, deactivation
 *   - Combat: shoot + hitscan, collision resolution
 */

#include "test_framework.h"

#include "../src/core/AABB.h"
#include "../src/core/Entity.h"
#include "../src/core/Projectile.h"
#include "../src/game/Combat.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>


// ── AABB Tests ──────────────────────────────────────────────────────────────

void test_aabb_from_center() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    ASSERT_NEAR(box.min.x, -1.0f, 1e-5f);
    ASSERT_NEAR(box.max.x,  1.0f, 1e-5f);
    ASSERT_NEAR(box.min.y, -1.0f, 1e-5f);
    ASSERT_NEAR(box.max.y,  1.0f, 1e-5f);
}

void test_aabb_contains() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    ASSERT_TRUE(box.contains({0, 0, 0}));
    ASSERT_TRUE(box.contains({0.5f, 0.5f, 0.5f}));
    ASSERT_TRUE(!box.contains({2, 0, 0}));
    ASSERT_TRUE(!box.contains({0, -2, 0}));
}

void test_aabb_intersects() {
    auto a = qe::core::AABB::from_center({0, 0, 0}, 1.0f);
    auto b = qe::core::AABB::from_center({1.5f, 0, 0}, 1.0f);
    auto c = qe::core::AABB::from_center({5, 0, 0}, 1.0f);
    ASSERT_TRUE(a.intersects(b));   // Overlapping
    ASSERT_TRUE(!a.intersects(c));  // No overlap
}

void test_aabb_ray_hit() {
    auto box = qe::core::AABB::from_center({5, 0, 0}, 1.0f);
    float t = 0;
    bool hit = box.ray_intersect({0, 0, 0}, {1, 0, 0}, t);
    ASSERT_TRUE(hit);
    ASSERT_NEAR(t, 4.0f, 0.01f);  // Ray hits at x=4 (box starts at x=4)
}

void test_aabb_ray_miss() {
    auto box = qe::core::AABB::from_center({5, 0, 0}, 1.0f);
    float t = 0;
    bool hit = box.ray_intersect({0, 0, 0}, {0, 1, 0}, t);  // Shooting up
    ASSERT_TRUE(!hit);
}

void test_aabb_ray_behind() {
    auto box = qe::core::AABB::from_center({-5, 0, 0}, 1.0f);
    float t = 0;
    bool hit = box.ray_intersect({0, 0, 0}, {1, 0, 0}, t);  // Box behind ray
    ASSERT_TRUE(!hit);
}

void test_aabb_transform() {
    auto box = qe::core::AABB::from_center({0, 0, 0}, 0.5f);
    auto transformed = box.transformed({10, 0, 0}, {2, 1, 1});
    ASSERT_NEAR(transformed.center().x, 10.0f, 1e-5f);
    ASSERT_NEAR(transformed.half_extents().x, 1.0f, 1e-5f);
}

// ── Entity Tests ────────────────────────────────────────────────────────────

void test_entity_damage() {
    qe::core::Entity ent;
    ent.health = 100;
    ent.max_health = 100;
    bool killed = ent.take_damage(30);
    ASSERT_TRUE(!killed);
    ASSERT_NEAR(ent.health, 70, 1e-5f);
    ASSERT_TRUE(ent.alive);
    ASSERT_TRUE(ent.hit_flash > 0);
}

void test_entity_kill() {
    qe::core::Entity ent;
    ent.health = 50;
    ent.max_health = 100;
    bool killed = ent.take_damage(60);
    ASSERT_TRUE(killed);
    ASSERT_NEAR(ent.health, 0, 1e-5f);
    ASSERT_TRUE(!ent.alive);
}

void test_entity_respawn() {
    qe::core::Entity ent;
    ent.health = 100;
    ent.max_health = 100;
    ent.spawn_position = {5, 0, 5};
    ent.respawn_delay = 1.0f;

    ent.take_damage(200);
    ASSERT_TRUE(!ent.alive);

    // Simulate time passing
    ent.update(1.1f);
    ASSERT_TRUE(ent.alive);
    ASSERT_NEAR(ent.health, 100, 1e-5f);
    ASSERT_NEAR(ent.position.x, 5, 1e-5f);
}

void test_entity_indestructible() {
    qe::core::Entity ent;
    ent.destructible = false;
    ent.health = 100;
    bool killed = ent.take_damage(999);
    ASSERT_TRUE(!killed);
    ASSERT_NEAR(ent.health, 100, 1e-5f);
}

void test_entity_health_fraction() {
    qe::core::Entity ent;
    ent.health = 75;
    ent.max_health = 100;
    ASSERT_NEAR(ent.health_fraction(), 0.75f, 1e-5f);
}

// ── Projectile Tests ────────────────────────────────────────────────────────

void test_projectile_movement() {
    qe::core::Projectile p;
    p.position = {0, 0, 0};
    p.velocity = {10, 0, 0};
    p.lifetime = 5.0f;

    p.update(0.5f);
    ASSERT_NEAR(p.position.x, 5.0f, 1e-5f);
    ASSERT_TRUE(p.is_alive());
}

void test_projectile_lifetime() {
    qe::core::Projectile p;
    p.lifetime = 1.0f;
    p.update(0.5f);
    ASSERT_TRUE(p.is_alive());
    p.update(0.6f);
    ASSERT_TRUE(!p.is_alive());
}

void test_projectile_bounds() {
    qe::core::Projectile p;
    p.position = {5, 3, 2};
    p.radius = 0.5f;
    auto b = p.bounds();
    ASSERT_TRUE(b.contains({5, 3, 2}));
    ASSERT_TRUE(!b.contains({6, 3, 2}));
}

// ── Combat Tests ────────────────────────────────────────────────────────────

void test_combat_shoot_hit() {
    std::vector<qe::core::Projectile> projectiles;
    std::vector<qe::core::Entity> entities;
    qe::game::CombatStats stats;
    qe::game::CombatConfig cfg;

    // Place entity directly ahead
    qe::core::Entity target;
    target.position = {0, 0, -10};
    target.scale = {2, 2, 2};
    target.local_bounds = qe::core::AABB::from_center({0,0,0}, 0.5f);
    target.health = 50;
    target.max_health = 50;
    entities.push_back(target);

    // Shoot toward it
    qe::game::shoot({0,0,0}, {0,0,-1}, cfg, projectiles, entities, stats);

    ASSERT_TRUE(stats.total_shots == 1);
    ASSERT_TRUE(stats.total_hits == 1);
    ASSERT_TRUE(projectiles.size() == 1);
}

void test_combat_shoot_miss() {
    std::vector<qe::core::Projectile> projectiles;
    std::vector<qe::core::Entity> entities;
    qe::game::CombatStats stats;
    qe::game::CombatConfig cfg;

    // Place entity to the side
    qe::core::Entity target;
    target.position = {10, 0, 0};
    target.scale = {1, 1, 1};
    target.local_bounds = qe::core::AABB::from_center({0,0,0}, 0.5f);
    entities.push_back(target);

    // Shoot forward (miss)
    qe::game::shoot({0,0,0}, {0,0,-1}, cfg, projectiles, entities, stats);

    ASSERT_TRUE(stats.total_shots == 1);
    ASSERT_TRUE(stats.total_hits == 0);
}

void test_combat_accuracy() {
    qe::game::CombatStats stats;
    stats.total_shots = 10;
    stats.total_hits = 7;
    ASSERT_NEAR(stats.accuracy(), 70.0f, 1e-3f);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== QuatEngine Game Tests ===" << std::endl;

    std::cout << "\n--- AABB ---" << std::endl;
    RUN_TEST(test_aabb_from_center);
    RUN_TEST(test_aabb_contains);
    RUN_TEST(test_aabb_intersects);
    RUN_TEST(test_aabb_ray_hit);
    RUN_TEST(test_aabb_ray_miss);
    RUN_TEST(test_aabb_ray_behind);
    RUN_TEST(test_aabb_transform);

    std::cout << "\n--- Entity ---" << std::endl;
    RUN_TEST(test_entity_damage);
    RUN_TEST(test_entity_kill);
    RUN_TEST(test_entity_respawn);
    RUN_TEST(test_entity_indestructible);
    RUN_TEST(test_entity_health_fraction);

    std::cout << "\n--- Projectile ---" << std::endl;
    RUN_TEST(test_projectile_movement);
    RUN_TEST(test_projectile_lifetime);
    RUN_TEST(test_projectile_bounds);

    std::cout << "\n--- Combat ---" << std::endl;
    RUN_TEST(test_combat_shoot_hit);
    RUN_TEST(test_combat_shoot_miss);
    RUN_TEST(test_combat_accuracy);

    return TEST_REPORT();
}
