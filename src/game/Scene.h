#pragma once
/**
 * @file Scene.h
 * @brief Scene object definitions and factory functions.
 *
 * Responsible only for describing what exists in the world.
 * Does NOT contain rendering or physics logic. (SRP)
 */

#include "../core/AABB.h"
#include "../core/Entity.h"
#include "../math/Constants.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"
#include "../math/Mat4.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace qe {
namespace game {

using math::PI;

// ── Static Decoration ───────────────────────────────────────────────────────

struct Decoration {
    math::Vec3       position;
    math::Quaternion rotation = math::Quaternion::identity();
    math::Vec3       scale{1, 1, 1};

    enum class MeshType { Cube, Sphere, Floor };
    MeshType mesh_type = MeshType::Cube;
    int texture_id = -1;

    enum class Anim { None, SpinY, SpinTilted };
    Anim anim = Anim::None;
    float anim_speed = 0.0f;
    float anim_phase = 0.0f;

    math::Mat4 model_matrix(float time) const {
        math::Quaternion rot = rotation;
        if (anim == Anim::SpinY)
            rot = math::Quaternion::from_axis_angle(
                math::Vec3::up(), time * anim_speed + anim_phase) * rot;
        else if (anim == Anim::SpinTilted)
            rot = math::Quaternion::from_axis_angle(
                math::Vec3(0, 1, 0.3f).normalized(),
                time * anim_speed + anim_phase) * rot;
        return math::Mat4::trs(position, rot, scale);
    }
};

// ── Scene Builder ───────────────────────────────────────────────────────────

/** Build static decorations (pillars, floor, platforms). */
inline std::vector<Decoration> build_decorations() {
    using namespace math;
    std::vector<Decoration> decs;

    // Floor
    decs.push_back({Vec3(0, -0.01f, 0), Quaternion::identity(), Vec3::one(),
                    Decoration::MeshType::Floor, 2});

    // Brick pillars
    for (int i = 0; i < 8; ++i) {
        float a = (2 * PI * i) / 8;
        float h = 2.0f + (i % 3) * 1.5f;
        decs.push_back({
            Vec3(std::cos(a) * 14, h * 0.5f, std::sin(a) * 14),
            Quaternion::from_axis_angle(Vec3::up(), a),
            Vec3(0.8f, h, 0.8f),
            Decoration::MeshType::Cube, 1});
    }

    // Floating platforms
    for (int i = 0; i < 5; ++i) {
        float a = (2 * PI * i) / 5 + 0.3f;
        float r = 8.0f + i * 0.5f;
        decs.push_back({
            Vec3(std::cos(a) * r, 0.1f + i * 0.8f, std::sin(a) * r),
            Quaternion::from_axis_angle(Vec3::up(), a),
            Vec3(2, 0.15f, 2),
            Decoration::MeshType::Cube, 2});
    }

    return decs;
}

/** Spawn destructible target entities. */
inline std::vector<core::Entity> spawn_targets() {
    using namespace math;
    std::vector<core::Entity> entities;

    // Inner ring — 12 small targets
    for (int i = 0; i < 12; ++i) {
        float a = (2 * PI * i) / 12;
        core::Entity ent;
        ent.id = i;
        ent.position = Vec3(std::cos(a) * 10, 1.0f + (i % 3) * 1.2f, std::sin(a) * 10);
        ent.spawn_position = ent.position;
        ent.scale = Vec3(0.8f, 0.8f, 0.8f);
        ent.health = 50.0f;
        ent.max_health = 50.0f;
        ent.local_bounds = core::AABB::from_center(Vec3::zero(), 0.5f);
        ent.respawn_delay = 4.0f;
        entities.push_back(ent);
    }

    // Outer ring — 6 big targets
    for (int i = 0; i < 6; ++i) {
        float a = (2 * PI * i) / 6 + PI / 6;
        core::Entity ent;
        ent.id = 12 + i;
        ent.position = Vec3(std::cos(a) * 18, 2.0f, std::sin(a) * 18);
        ent.spawn_position = ent.position;
        ent.scale = Vec3(1.5f, 1.5f, 1.5f);
        ent.health = 100.0f;
        ent.max_health = 100.0f;
        ent.local_bounds = core::AABB::from_center(Vec3::zero(), 0.5f);
        ent.respawn_delay = 5.0f;
        entities.push_back(ent);
    }

    return entities;
}

} // namespace game
} // namespace qe
