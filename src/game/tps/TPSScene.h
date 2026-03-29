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

inline constexpr float SCENE_PI = 3.14159265358979f;

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

/** Build a complete scene from level data.
 *  @pre level_data.check_invariants() passes
 *  @post scene.enemies.size() == level_data.total_enemy_count
 */
inline TPSSceneData build_scene(const LevelData& level_data, uint32_t seed = 42) {
    TPSSceneData scene;
    scene.environment = level_data.environment;

    // Ground
    float radius = level_data.arena_radius;
    scene.ground_scale = math::Vec3(radius, 0.1f, radius);

    // RNG
    uint32_t rng = seed + static_cast<uint32_t>(level_data.level_number * 7919);
    auto rand_float = [&rng](float lo, float hi) -> float {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        float t = static_cast<float>(rng & 0xFFFF) / 65535.0f;
        return lo + t * (hi - lo);
    };

    // Generate cover objects based on environment type
    int cover_count = 8 + level_data.level_number * 2;
    for (int i = 0; i < cover_count; ++i) {
        SceneObject obj;
        float angle = (2.0f * SCENE_PI * i) / static_cast<float>(cover_count);
        float dist = rand_float(radius * 0.2f, radius * 0.7f);

        obj.position = math::Vec3(
            std::cos(angle) * dist,
            0.0f,
            std::sin(angle) * dist
        );
        obj.rotation = math::Quaternion::from_axis_angle(
            math::Vec3::up(), rand_float(0.0f, SCENE_PI * 2.0f));

        // Pick shape variation within environment
        int shape_variant = static_cast<int>(rng & 0x3); // 0-3
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;

        switch (level_data.environment.type) {
            case EnvironmentType::Military:
                // Barricades (cubes), sandbag walls (wedges), watchtower legs (cylinders)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Wedge;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Cylinder;
                obj.scale = math::Vec3(rand_float(1.0f, 3.0f), rand_float(1.5f, 3.0f),
                                       rand_float(0.5f, 1.5f));
                obj.color = math::Vec3(0.35f, 0.3f, 0.25f);
                break;
            case EnvironmentType::Underground:
                // Stalagmites (cones), pillars (cylinders), rubble (cubes)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cone;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Cylinder;
                obj.scale = math::Vec3(rand_float(0.8f, 2.0f), rand_float(2.0f, 4.0f),
                                       rand_float(0.8f, 2.0f));
                obj.color = math::Vec3(0.25f, 0.25f, 0.3f);
                break;
            case EnvironmentType::Urban:
                // Buildings (cubes), bollards (cylinders), ramps (wedges), rubble (pyramids)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Wedge;
                else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Pyramid;
                obj.scale = math::Vec3(rand_float(1.0f, 4.0f), rand_float(0.5f, 2.5f),
                                       rand_float(1.0f, 3.0f));
                obj.color = math::Vec3(0.4f, 0.35f, 0.3f);
                break;
            case EnvironmentType::Forest:
                // Tree trunks (cylinders), canopies (spheres), boulders (spheres), stumps (cones)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Sphere;
                else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Cone;
                else obj.mesh_type = SceneObjectType::Sphere;
                obj.scale = math::Vec3(rand_float(0.3f, 0.8f), rand_float(2.0f, 5.0f),
                                       rand_float(0.3f, 0.8f));
                obj.color = math::Vec3(0.2f, 0.35f, 0.15f);
                break;
            case EnvironmentType::Industrial:
                // Pipes (cylinders), tanks (capsules), crates (cubes), hoppers (cones)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Capsule;
                else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Cone;
                obj.scale = math::Vec3(rand_float(2.0f, 5.0f), rand_float(1.0f, 3.0f),
                                       rand_float(0.5f, 1.0f));
                obj.color = math::Vec3(0.4f, 0.3f, 0.2f);
                break;
            case EnvironmentType::Cathedral:
                // Columns (cylinders), arches (capsules), altars (pyramids), pews (cubes)
                if (shape_variant == 0) obj.mesh_type = SceneObjectType::Cylinder;
                else if (shape_variant == 1) obj.mesh_type = SceneObjectType::Capsule;
                else if (shape_variant == 2) obj.mesh_type = SceneObjectType::Pyramid;
                obj.scale = math::Vec3(rand_float(1.0f, 2.0f), rand_float(3.0f, 8.0f),
                                       rand_float(1.0f, 2.0f));
                obj.color = math::Vec3(0.45f, 0.4f, 0.35f);
                break;
            default:
                obj.scale = math::Vec3(rand_float(1.0f, 3.0f), rand_float(1.0f, 3.0f),
                                       rand_float(1.0f, 3.0f));
                obj.color = math::Vec3(0.35f, 0.35f, 0.35f);
                break;
        }
        obj.position.y = obj.scale.y * 0.5f;
        scene.cover_objects.push_back(obj);
    }

    // Generate decorative props with varied shapes
    int deco_count = 5 + level_data.level_number;
    const SceneObjectType deco_shapes[] = {
        SceneObjectType::Cube, SceneObjectType::Cone, SceneObjectType::Pyramid,
        SceneObjectType::Wedge, SceneObjectType::Cylinder, SceneObjectType::Sphere
    };
    for (int i = 0; i < deco_count; ++i) {
        SceneObject deco;
        float angle = rand_float(0.0f, SCENE_PI * 2.0f);
        float dist = rand_float(radius * 0.1f, radius * 0.85f);
        deco.position = math::Vec3(
            std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);
        deco.scale = math::Vec3(rand_float(0.3f, 1.0f), rand_float(0.1f, 0.5f),
                                rand_float(0.3f, 1.0f));
        deco.color = scene.environment.ambient_color * 2.0f;
        deco.mesh_type = deco_shapes[i % 6];
        deco.position.y = deco.scale.y * 0.5f;
        scene.decorations.push_back(deco);
    }

    // Spawn enemies from level spawn table
    int enemy_id = 1;
    for (const auto& entry : level_data.spawn_table) {
        for (int i = 0; i < entry.count; ++i) {
            float angle = rand_float(0.0f, SCENE_PI * 2.0f);
            float dist = rand_float(radius * 0.3f, radius * 0.8f);
            math::Vec3 spawn_pos(
                std::cos(angle) * dist,
                0.0f,
                std::sin(angle) * dist
            );

            // Flying enemies spawn higher
            if (entry.type == MutantType::Amalgam) {
                spawn_pos.y = rand_float(3.0f, 6.0f);
            }

            auto mutant = spawn_mutant(entry.type, spawn_pos, enemy_id++);
            mutant.patrol_radius = rand_float(4.0f, 10.0f);
            mutant.patrol_phase = rand_float(0.0f, SCENE_PI * 2.0f);

            // Apply level difficulty scaling
            mutant.config.health *= level_data.enemy_health_mult;
            mutant.current_health = mutant.config.health;
            mutant.config.move_speed *= level_data.enemy_speed_mult;
            mutant.config.primary_attack.damage *= level_data.enemy_damage_mult;
            mutant.config.secondary_attack.damage *= level_data.enemy_damage_mult;

            scene.enemies.push_back(mutant);
        }
    }

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
