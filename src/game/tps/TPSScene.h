#pragma once
/**
 * @file TPSScene.h
 * @brief Level environment generation and enemy spawning for TPS game.
 *
 * Creates world layout data from LevelData. Generates:
 *   - Ground plane dimensions and texture tiling
 *   - Cover objects (walls, pillars, barricades) per environment type
 *   - Decorative props (ruins, debris, vegetation)
 *   - Enemy spawn positions within arena bounds
 *   - Lighting and atmosphere parameters
 *
 * Pure data generation — no rendering dependencies.
 * Rendering layer reads this data to create actual meshes.
 */

#include "../../core/Rng.h"
#include "../../math/Constants.h"
#include "../../math/Quaternion.h"
#include "../../math/Vec3.h"
#include "LevelSystem.h"
#include "MutantTypes.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qe {
namespace game {
namespace tps {

using math::PI;

// ── Scene Object ─────────────────────────────────────────────────────────────

enum class SceneObjectType {
    Cube,
    Sphere,
    Floor,
    Cylinder,
    Cone,
    Capsule,
    Wedge,
    Pyramid
};

struct SceneObject {
    math::Vec3 position;
    math::Quaternion rotation = math::Quaternion::identity();
    math::Vec3 scale{1, 1, 1};
    SceneObjectType mesh_type = SceneObjectType::Cube;
    math::Vec3 color{0.5f, 0.5f, 0.5f};
    int texture_id = -1;
    bool destructible = false;
};

// ── Scene Data ───────────────────────────────────────────────────────────────

struct TPSSceneData {
    // Ground
    math::Vec3 ground_scale;
    int ground_texture_id = 0;

    // Objects
    std::vector<SceneObject> cover_objects;
    std::vector<SceneObject> decorations;

    // Spawned enemies
    std::vector<MutantInstance> enemies;

    // Environment
    EnvironmentConfig environment;
};

// ── Scene Builder ────────────────────────────────────────────────────────────

// ── build_scene helpers ─────────────────────────────────────────────────────

/** Apply environment-specific shape, scale, and color to a cover object. */
inline void apply_cover_style(SceneObject& obj, EnvironmentType env_type,
                               int shape_variant, core::Rng& rng) {
    auto rf = [&rng](float lo, float hi) { return rng.random_float(lo, hi); };

    switch (env_type) {
        case EnvironmentType::Military:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Wedge;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Cylinder;
            obj.scale = math::Vec3(rf(1.0f, 3.0f), rf(1.5f, 3.0f), rf(0.5f, 1.5f));
            obj.color = math::Vec3(0.35f, 0.3f, 0.25f);
            break;
        case EnvironmentType::Underground:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cone;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Cylinder;
            obj.scale = math::Vec3(rf(0.8f, 2.0f), rf(2.0f, 4.0f), rf(0.8f, 2.0f));
            obj.color = math::Vec3(0.25f, 0.25f, 0.3f);
            break;
        case EnvironmentType::Urban:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Wedge;
            else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Pyramid;
            obj.scale = math::Vec3(rf(1.0f, 4.0f), rf(0.5f, 2.5f), rf(1.0f, 3.0f));
            obj.color = math::Vec3(0.4f, 0.35f, 0.3f);
            break;
        case EnvironmentType::Forest:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Sphere;
            else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Cone;
            else obj.mesh_type = SceneObjectType::Sphere;
            obj.scale = math::Vec3(rf(0.3f, 0.8f), rf(2.0f, 5.0f), rf(0.3f, 0.8f));
            obj.color = math::Vec3(0.2f, 0.35f, 0.15f);
            break;
        case EnvironmentType::Industrial:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Capsule;
            else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Cone;
            obj.scale = math::Vec3(rf(2.0f, 5.0f), rf(1.0f, 3.0f), rf(0.5f, 1.0f));
            obj.color = math::Vec3(0.4f, 0.3f, 0.2f);
            break;
        case EnvironmentType::Cathedral:
            if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
            else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Capsule;
            else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Pyramid;
            obj.scale = math::Vec3(rf(1.0f, 2.0f), rf(3.0f, 8.0f), rf(1.0f, 2.0f));
            obj.color = math::Vec3(0.45f, 0.4f, 0.35f);
            break;
        default:
            obj.scale = math::Vec3(rf(1.0f, 3.0f), rf(1.0f, 3.0f), rf(1.0f, 3.0f));
            obj.color = math::Vec3(0.35f, 0.35f, 0.35f);
            break;
    }
}

/** Generate cover objects for the scene. */
inline void generate_cover(TPSSceneData& scene, const LevelData& level_data,
                            float radius, core::Rng& rng) {
    int cover_count = 8 + level_data.level_number * 2;
    for (int i = 0; i < cover_count; ++i) {
        SceneObject obj;
        float angle = (2.0f * PI * i) / static_cast<float>(cover_count);
        float dist = rng.random_float(radius * 0.2f, radius * 0.7f);

        obj.position = math::Vec3(
            std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);
        obj.rotation = math::Quaternion::from_axis_angle(
            math::Vec3::up(), rng.random_float(0.0f, PI * 2.0f));

        int shape_variant = static_cast<int>(rng.next() & 0x3);
        apply_cover_style(obj, level_data.environment.type, shape_variant, rng);

        obj.position.y = obj.scale.y * 0.5f;
        scene.cover_objects.push_back(obj);
    }
}

/** Generate decorative props for the scene. */
inline void generate_decorations(TPSSceneData& scene, const LevelData& level_data,
                                  float radius, core::Rng& rng) {
    int deco_count = 5 + level_data.level_number;
    const SceneObjectType deco_shapes[] = {
        SceneObjectType::Cube, SceneObjectType::Cone, SceneObjectType::Pyramid,
        SceneObjectType::Wedge, SceneObjectType::Cylinder, SceneObjectType::Sphere
    };
    for (int i = 0; i < deco_count; ++i) {
        SceneObject deco;
        float angle = rng.random_float(0.0f, PI * 2.0f);
        float dist = rng.random_float(radius * 0.1f, radius * 0.85f);
        deco.position = math::Vec3(
            std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);
        deco.scale = math::Vec3(rng.random_float(0.3f, 1.0f),
                                rng.random_float(0.1f, 0.5f),
                                rng.random_float(0.3f, 1.0f));
        deco.color = scene.environment.ambient_color * 2.0f;
        deco.mesh_type = deco_shapes[i % 6];
        deco.position.y = deco.scale.y * 0.5f;
        scene.decorations.push_back(deco);
    }
}

/** Spawn enemies from the level's spawn table with difficulty scaling. */
inline void spawn_enemies(TPSSceneData& scene, const LevelData& level_data,
                           float radius, core::Rng& rng) {
    int enemy_id = 1;
    for (const auto& entry : level_data.spawn_table) {
        for (int i = 0; i < entry.count; ++i) {
            float angle = rng.random_float(0.0f, PI * 2.0f);
            float dist = rng.random_float(radius * 0.3f, radius * 0.8f);
            math::Vec3 spawn_pos(
                std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);

            if (entry.type == MutantType::Amalgam) {
                spawn_pos.y = rng.random_float(3.0f, 6.0f);
            }

            auto mutant = spawn_mutant(entry.type, spawn_pos, enemy_id++);
            mutant.patrol_radius = rng.random_float(4.0f, 10.0f);
            mutant.patrol_phase = rng.random_float(0.0f, PI * 2.0f);

            mutant.config.health *= level_data.enemy_health_mult;
            mutant.current_health = mutant.config.health;
            mutant.config.move_speed *= level_data.enemy_speed_mult;
            mutant.config.primary_attack.damage *= level_data.enemy_damage_mult;
            mutant.config.secondary_attack.damage *= level_data.enemy_damage_mult;

            scene.enemies.push_back(mutant);
        }
    }
}

// ── Scene Builder ────────────────────────────────────────────────────────────

/** Build a complete scene from level data.
 *  @pre level_data.check_invariants() passes
 *  @post scene.enemies.size() == level_data.total_enemy_count
 */
inline TPSSceneData build_scene(const LevelData& level_data, uint32_t seed = 42) {
    TPSSceneData scene;
    scene.environment = level_data.environment;

    float radius = level_data.arena_radius;
    scene.ground_scale = math::Vec3(radius, 0.1f, radius);

    core::Rng rng(seed + static_cast<uint32_t>(level_data.level_number * 7919));

    generate_cover(scene, level_data, radius, rng);
    generate_decorations(scene, level_data, radius, rng);
    spawn_enemies(scene, level_data, radius, rng);

    assert(static_cast<int>(scene.enemies.size()) == level_data.total_enemy_count);
    return scene;
}

/** Get mesh shape for a mutant type (for rendering). */
inline SceneObjectType mutant_mesh_type(MutantType type) {
    switch (type) {
        case MutantType::Grunt:    return SceneObjectType::Cube;      // basic humanoid
        case MutantType::Crawler:  return SceneObjectType::Wedge;     // low crouching form
        case MutantType::Brute:    return SceneObjectType::Cube;      // bulky box
        case MutantType::Stalker:  return SceneObjectType::Capsule;   // sleek elongated
        case MutantType::Spitter:  return SceneObjectType::Cone;      // hunched posture
        case MutantType::Screamer: return SceneObjectType::Pyramid;   // angular menacing
        case MutantType::Hound:    return SceneObjectType::Wedge;     // low fast profile
        case MutantType::Amalgam:  return SceneObjectType::Sphere;    // floating mass
        case MutantType::Behemoth: return SceneObjectType::Cylinder;  // massive pillar
        case MutantType::Apex:     return SceneObjectType::Capsule;   // towering boss
    }
    return SceneObjectType::Cube;
}

/** Get color for a mutant type (for rendering). */
inline math::Vec3 mutant_color(MutantType type) {
    switch (type) {
        case MutantType::Grunt:    return {0.5f, 0.45f, 0.35f};
        case MutantType::Crawler:  return {0.35f, 0.5f, 0.3f};
        case MutantType::Brute:    return {0.6f, 0.4f, 0.3f};
        case MutantType::Stalker:  return {0.3f, 0.3f, 0.4f};
        case MutantType::Spitter:  return {0.4f, 0.6f, 0.2f};
        case MutantType::Screamer: return {0.6f, 0.2f, 0.5f};
        case MutantType::Hound:    return {0.5f, 0.35f, 0.25f};
        case MutantType::Amalgam:  return {0.5f, 0.3f, 0.4f};
        case MutantType::Behemoth: return {0.55f, 0.35f, 0.25f};
        case MutantType::Apex:     return {0.7f, 0.2f, 0.15f};
    }
    return {0.5f, 0.5f, 0.5f};
}

/** Get scale for a mutant type (for rendering). */
inline math::Vec3 mutant_scale(MutantType type) {
    switch (type) {
        case MutantType::Grunt:    return {0.5f, 0.9f, 0.5f};
        case MutantType::Crawler:  return {0.7f, 0.4f, 0.7f};
        case MutantType::Brute:    return {1.0f, 1.4f, 1.0f};
        case MutantType::Stalker:  return {0.4f, 1.0f, 0.4f};
        case MutantType::Spitter:  return {0.5f, 0.8f, 0.5f};
        case MutantType::Screamer: return {0.5f, 0.9f, 0.5f};
        case MutantType::Hound:    return {0.5f, 0.4f, 0.7f};
        case MutantType::Amalgam:  return {0.8f, 0.6f, 0.8f};
        case MutantType::Behemoth: return {1.5f, 2.0f, 1.5f};
        case MutantType::Apex:     return {1.8f, 2.5f, 1.8f};
    }
    return {0.5f, 0.5f, 0.5f};
}

} // namespace tps
} // namespace game
} // namespace qe
