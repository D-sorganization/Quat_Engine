// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Rendering.cpp
 * @brief Demo world, particle, and HUD rendering systems.
 */

#include "demo/App.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/GLLoader.h"

#include <cmath>
#include <string>

namespace qe::demo {

// ── render_world helpers ────────────────────────────────────────────────────

static void setup_world_shader(App& app) {
    using namespace qe::math;

    app.world_shader.use();

    const Mat4 vp = app.camera.vp_matrix();
    app.world_shader.set_mat4("uViewProjection", vp);
    app.world_shader.set_vec3("uLightDir", Vec3(0.3f, 0.8f, 0.5f).normalized());
    app.world_shader.set_vec3("uLightColor", Vec3(1, 0.95f, 0.9f));
    app.world_shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    app.world_shader.set_vec3("uCameraPos", app.camera.position());
    app.world_shader.set_int("uTexture0", 0);
    app.world_shader.set_float("uTime", app.time);
    app.world_shader.set_float("uFogNear", qe::config::FOG_NEAR());
    app.world_shader.set_float("uFogFar", qe::config::FOG_FAR());
    app.world_shader.set_vec3(
        "uFogColor", Vec3(qe::config::CLEAR_R(), qe::config::CLEAR_G(), qe::config::CLEAR_B()));
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);
    app.world_shader.set_float("uRimPower", qe::config::RIM_POWER());
    app.world_shader.set_vec3("uRimColor", Vec3(0.1f, 0.2f, 0.4f));
}

static void setup_point_lights(App& app) {
    using namespace qe::math;

    int point_light_count = 0;
    auto set_point_light = [&](const Vec3& pos, const Vec3& color, float radius) {
        if (point_light_count >= qe::config::MAX_POINT_LIGHTS()) {
            return;
        }
        std::string prefix = "uPointLightPos[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_vec3(prefix, pos);
        prefix = "uPointLightColor[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_vec3(prefix, color);
        prefix = "uPointLightRadius[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_float(prefix, radius);
        ++point_light_count;
    };

    for (const auto& projectile : app.projectiles) {
        if (projectile.active && point_light_count < 6) {
            set_point_light(projectile.position,
                            Vec3(1.0f, 0.7f, 0.2f) * projectile.brightness,
                            5.0f);
        }
    }

    for (const auto& pickup : app.powerups.pickups()) {
        if (pickup.alive && point_light_count < 8) {
            const auto cfg = qe::game::PowerUpManager::get_config(pickup.type);
            set_point_light(pickup.display_position(), cfg.color * pickup.glow_intensity(), 4.0f);
        }
    }

    app.world_shader.set_int("uPointLightCount", point_light_count);
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);
}

static void render_decorations(App& app) {
    using namespace qe::renderer::gl;

    for (const auto& decoration : app.decorations) {
        app.world_shader.set_mat4("uModel", decoration.model_matrix(app.time));
        const bool textured = decoration.texture_id >= 0 &&
                              decoration.texture_id < static_cast<int>(app.textures.size());
        app.world_shader.set_int("uUseTexture", textured ? 1 : 0);
        if (textured) {
            app.textures[decoration.texture_id]->bind(0);
        }

        if (decoration.mesh_type == qe::game::Decoration::MeshType::Floor) {
            glDisable(GL_CULL_FACE);
            app.floor_plane.draw();
            glEnable(GL_CULL_FACE);
        } else if (decoration.mesh_type == qe::game::Decoration::MeshType::Sphere) {
            app.sphere.draw();
        } else {
            app.cube.draw();
        }
    }
}

static void render_entities(App& app) {
    using namespace qe::math;

    app.world_shader.set_int("uUseTexture", 0);
    for (const auto& ent : app.entities) {
        if (!ent.alive) {
            if (ent.death_timer < 1.0f) {
                const float t = ent.death_timer;
                app.world_shader.set_mat4("uModel",
                                          Mat4::trs(ent.position + Vec3(0, t * 2, 0),
                                                    Quaternion::from_axis_angle(Vec3::up(), t * 10),
                                                    ent.scale * (1 - t)));
                app.world_shader.set_vec3("uEmission", Vec3(1.0f, 0.3f, 0.1f));
                app.world_shader.set_float("uEmissionStrength", 1.0f - t);
                app.sphere.draw();
                app.world_shader.set_vec3("uEmission", Vec3::zero());
                app.world_shader.set_float("uEmissionStrength", 0.0f);
            }
            continue;
        }

        app.world_shader.set_mat4("uModel", Mat4::trs(ent.position, ent.rotation, ent.scale));
        if (ent.hit_flash > 0) {
            const float flash = ent.hit_flash / 0.3f;
            app.world_shader.set_vec3("uEmission", Vec3(flash, flash * 0.5f, flash * 0.2f));
            app.world_shader.set_float("uEmissionStrength", flash);
        }

        app.cube.draw();

        if (ent.hit_flash > 0) {
            app.world_shader.set_vec3("uEmission", Vec3::zero());
            app.world_shader.set_float("uEmissionStrength", 0.0f);
        }
    }
}

static void render_projectiles(App& app) {
    using namespace qe::math;

    for (const auto& projectile : app.projectiles) {
        if (!projectile.active) {
            continue;
        }
        app.world_shader.set_vec3("uEmission", Vec3(1.0f, 0.8f, 0.3f));
        app.world_shader.set_float("uEmissionStrength", projectile.brightness);
        app.world_shader.set_mat4("uModel",
                                  Mat4::trs(projectile.position,
                                            Quaternion::identity(),
                                            Vec3(0.1f, 0.1f, 0.1f)));
        app.cube.draw();
    }
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);
}

static void render_pickups(App& app) {
    using namespace qe::math;

    for (const auto& pickup : app.powerups.pickups()) {
        if (!pickup.alive) {
            continue;
        }
        const auto cfg = qe::game::PowerUpManager::get_config(pickup.type);
        const float glow = pickup.glow_intensity();

        app.world_shader.set_vec3("uEmission", cfg.color * glow);
        app.world_shader.set_float("uEmissionStrength", glow);
        app.world_shader.set_mat4("uModel",
                                  Mat4::trs(pickup.display_position(),
                                            pickup.rotation,
                                            Vec3(0.4f, 0.4f, 0.4f)));
        app.cube.draw();
    }
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);
}

// ── Public render_world ─────────────────────────────────────────────────────

void render_world(App& app) {
    using namespace qe::renderer::gl;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setup_world_shader(app);
    setup_point_lights(app);
    render_decorations(app);
    render_entities(app);
    render_projectiles(app);
    render_pickups(app);

    glDisable(GL_CULL_FACE);
    app.world_shader.set_int("uUseTexture", 0);
    app.world_shader.set_mat4("uModel", qe::math::Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

void render_particles(App& app) {
    using namespace qe::math;
    using namespace qe::renderer::gl;

    if (app.particles.alive_count() == 0) {
        return;
    }

    app.particle_shader.use();
    app.particle_shader.set_mat4("uViewProjection", app.camera.vp_matrix());

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (const auto& particle : app.particles.particles()) {
        const float size = particle.current_size();
        const float alpha = 1.0f - particle.progress();
        app.particle_shader.set_mat4(
            "uModel", Mat4::trs(particle.position, particle.rotation, Vec3(size, size, size)));
        app.particle_shader.set_float("uAlpha", alpha);
        app.cube.draw();
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void render_hud(App& app) {
    using namespace qe::renderer::gl;

    glDisable(GL_DEPTH_TEST);
    app.hud_shader.use();
    app.hud_shader.set_float("uAlpha", 0.8f);

    glLineWidth(2.0f);
    app.hud.draw_crosshair();
    glLineWidth(1.0f);

    qe::renderer::HUDBar health_bar;
    health_bar.x = -0.95f;
    health_bar.y = 0.88f;
    health_bar.width = 0.4f;
    health_bar.height = 0.04f;
    health_bar.fill = 1.0f;
    health_bar.r = 0.2f;
    health_bar.g = 0.9f;
    health_bar.b = 0.3f;
    app.hud.draw_bar(health_bar);

    const auto& weapon = app.weapons.current();
    if (weapon.max_ammo != -1) {
        qe::renderer::HUDBar ammo_bar;
        ammo_bar.x = 0.55f;
        ammo_bar.y = -0.95f;
        ammo_bar.width = 0.4f;
        ammo_bar.height = 0.03f;
        ammo_bar.fill = static_cast<float>(weapon.ammo) / weapon.max_ammo;
        ammo_bar.r = 0.9f;
        ammo_bar.g = 0.8f;
        ammo_bar.b = 0.2f;
        app.hud.draw_bar(ammo_bar);
    }

    if (app.weapons.is_reloading()) {
        qe::renderer::HUDBar reload_bar;
        reload_bar.x = -0.15f;
        reload_bar.y = -0.15f;
        reload_bar.width = 0.3f;
        reload_bar.height = 0.02f;
        reload_bar.fill = app.weapons.reload_progress();
        reload_bar.r = 0.2f;
        reload_bar.g = 0.6f;
        reload_bar.b = 1.0f;
        app.hud.draw_bar(reload_bar);
    }

    app.hud.draw_weapon_slots(app.weapons.current_index(), app.weapons.weapon_count());
    app.hud.draw_combo_indicator(app.score.combo().streak, app.time);

    float effect_y = 0.8f;
    for (const auto& effect : app.powerups.effects()) {
        if (!effect.is_active()) {
            continue;
        }
        const auto cfg = qe::game::PowerUpManager::get_config(effect.type);
        qe::renderer::HUDBar effect_bar;
        effect_bar.x = 0.6f;
        effect_bar.y = effect_y;
        effect_bar.width = 0.35f;
        effect_bar.height = 0.025f;
        effect_bar.fill = 1.0f - effect.progress();
        effect_bar.r = cfg.color.x;
        effect_bar.g = cfg.color.y;
        effect_bar.b = cfg.color.z;
        app.hud.draw_bar(effect_bar);
        effect_y -= 0.04f;
    }

    if (app.waves.state() == qe::game::GameState::WaveIntro) {
        const float pulse = 0.5f + 0.5f * std::sin(app.time * 4.0f);
        app.hud.draw_indicator(0.0f, 0.2f, 0.08f, pulse, pulse, pulse);
    }

    glEnable(GL_DEPTH_TEST);
}

}  // namespace qe::demo
