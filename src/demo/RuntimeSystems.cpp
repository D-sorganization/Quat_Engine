// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file RuntimeSystems.cpp
 * @brief Demo input orchestration and runtime update systems.
 */

#include "demo/App.h"
#include "demo/RuntimeSession.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/GLLoader.h"

#include <cmath>
#include <sstream>
#include <string_view>

namespace qe::demo {

void spawn_wave_targets(App& app) {
    using namespace qe::core;
    using namespace qe::game;
    using namespace qe::math;

    const auto& cfg = app.waves.wave_config();
    app.entities.clear();
    app.behaviors.clear();
    app.projectiles.clear();

    const int count = cfg.target_count;
    const float hp = 50.0f * cfg.health_multiplier;
    const float speed = cfg.speed_multiplier;

    for (int i = 0; i < count; ++i) {
        const float angle = (2.0f * PI * i) / count;
        const float radius = cfg.spawn_radius_min + (cfg.spawn_radius_max - cfg.spawn_radius_min) *
                                                        (static_cast<float>(i % 5) / 5.0f);
        const float height = 1.0f + (i % 4) * 0.8f;
        const Vec3 pos(std::cos(angle) * radius, height, std::sin(angle) * radius);

        Entity entity;
        entity.id = i;
        entity.position = pos;
        entity.spawn_position = pos;
        entity.scale = Vec3(0.7f + (i % 3) * 0.3f, 0.7f + (i % 3) * 0.3f, 0.7f + (i % 3) * 0.3f);
        entity.health = hp;
        entity.max_health = hp;
        entity.local_bounds = AABB::from_center(Vec3::zero(), 0.5f);
        entity.respawn_delay = 999.0f;

        TargetBehavior behavior;
        switch (i % 7) {
            case 0:
                behavior = TargetBehavior::create_orbit(pos, 3.0f, speed * 0.8f, angle);
                break;
            case 1:
                behavior = TargetBehavior::create_figure8(pos, 2.5f, speed * 0.6f, angle);
                break;
            case 2:
                behavior = TargetBehavior::create_zigzag(pos, 2.0f, speed, angle);
                break;
            case 3:
                behavior = TargetBehavior::create_spiral(pos, 3.0f, speed * 0.5f, angle);
                break;
            case 4:
                behavior = TargetBehavior::create_patrol(pos, 4.0f, speed * 0.4f, angle);
                break;
            case 5:
                behavior = TargetBehavior::create_dodge(pos, speed);
                break;
            default:
                break;
        }

        app.entities.push_back(entity);
        app.behaviors.push_back(behavior);
    }
}

void handle_events(App& app) {
    app.input.begin_frame();

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            app.running = false;
            return;
        }
        if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            qe::renderer::gl::glViewport(0, 0, ev.window.data1, ev.window.data2);
            app.camera.set_aspect(static_cast<float>(ev.window.data1) / ev.window.data2);
            if (app.post_process) {
                app.post_process->updateResolution(ev.window.data1, ev.window.data2);
            }
        }
        app.input.handle_event(ev);
    }
    app.input.poll();

    if (app.input.quit()) {
        app.running = false;
    }
    if (app.input.toggle_camera()) {
        app.camera.toggle_mode();
    }
    if (app.input.toggle_wireframe()) {
        app.wireframe = !app.wireframe;
        qe::renderer::gl::glPolygonMode(GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys) {
        if (keys[SDL_SCANCODE_1])
            app.weapons.switch_weapon(0);
        if (keys[SDL_SCANCODE_2])
            app.weapons.switch_weapon(1);
        if (keys[SDL_SCANCODE_3])
            app.weapons.switch_weapon(2);
        if (keys[SDL_SCANCODE_4])
            app.weapons.switch_weapon(3);
        if (keys[SDL_SCANCODE_5])
            app.weapons.switch_weapon(4);
        if (keys[SDL_SCANCODE_Q])
            app.weapons.prev_weapon();
        if (keys[SDL_SCANCODE_E])
            app.weapons.next_weapon();
    }

    if (app.input.reset()) {
        if (app.waves.state() == qe::game::GameState::GameOver) {
            app.waves.start_game();
            spawn_wave_targets(app);
            app.score.reset();
            app.powerups.clear();
            app.particles.clear();
            app.weapons.init_loadout();
        } else {
            app.weapons.reload();
        }
    }

    if (app.input.slerp_off()) {
        app.camera.set_smoothing(0.0f);
        app.slerp_on = false;
    }
    if (app.input.slerp_on()) {
        app.camera.set_smoothing(qe::config::DEFAULT_CAMERA_SMOOTHING());
        app.slerp_on = true;
    }
}

// ── Update helpers ──────────────────────────────────────────────────────────

static void update_camera(App& app, float dt) {
    app.camera.process_mouse(app.input.look_x(), app.input.look_y());
    app.camera.process_scroll(app.input.zoom());
    app.camera.process_movement(app.input.move_forward(),
                                app.input.move_right(),
                                app.input.move_up(),
                                app.input.sprint(),
                                dt);
    app.camera.update(dt);
}

static void update_wave_spawns(App& app) {
    using namespace qe::game;

    const int alive = count_alive(app.entities);

    if (app.waves.state() == GameState::WaveIntro && alive == 0 &&
        app.waves.time_in_state() < 0.1f && app.waves.current_wave() > 1) {
        app.score.add_bonus(app.waves.wave_config().bonus_points);
        spawn_wave_targets(app);
    }
    if (app.waves.state() == GameState::WaveIntro && app.entities.empty()) {
        spawn_wave_targets(app);
    }
}

static void update_combat_config(App& app) {
    const float fire_rate_mult = app.powerups.get_fire_rate_multiplier();
    const float damage_mult = app.powerups.get_damage_multiplier();
    const auto& weapon = app.weapons.current();
    app.combat_cfg.fire_rate = weapon.fire_rate * fire_rate_mult;
    app.combat_cfg.projectile_damage = weapon.damage * damage_mult;
    app.combat_cfg.projectile_speed = weapon.projectile_speed;
    app.combat_cfg.projectile_lifetime = qe::config::PROJECTILE_LIFETIME();
    app.combat_cfg.projectile_radius = qe::config::PROJECTILE_RADIUS();
    app.combat_cfg.kill_score = qe::config::KILL_SCORE();
}

static void process_new_kills(App& app, const qe::math::Vec3& /*dir*/) {
    using namespace qe::math;
    using namespace qe::game;

    for (auto& ent : app.entities) {
        if (!ent.alive && ent.health <= 0 && ent.death_timer < 0.01f) {
            const float score_mult = app.powerups.get_score_multiplier();
            app.score.record_kill(app.combat_cfg.kill_score,
                                  score_mult,
                                  app.waves.wave_config().bonus_points / 10);

            auto death_cfg = qe::renderer::ParticleSystem::preset_death_burst();
            death_cfg.position = ent.position;
            app.particles.emit(death_cfg);

            app.powerups.try_spawn_random(ent.position + Vec3(0, 0.5f, 0));

            for (size_t j = 0; j < app.behaviors.size(); ++j) {
                if (app.behaviors[j].type == BehaviorType::Dodge && app.entities[j].alive) {
                    const float dist = ent.position.distance_to(app.entities[j].position);
                    if (dist < 8.0f) {
                        app.behaviors[j].alert();
                    }
                }
            }
        }
    }
}

static void update_shooting(App& app) {
    using namespace qe::game;
    using namespace qe::math;

    if (!(app.input.shoot_held() && app.weapons.can_fire() &&
          app.waves.state() == GameState::WaveActive)) {
        return;
    }

    app.weapons.fire();

    const auto directions = app.weapons.compute_fire_directions(app.camera.forward(), Vec3::up());

    for (const auto& dir : directions) {
        qe::game::CombatStats dummy_stats;
        qe::game::shoot(
            app.camera.position(), dir, app.combat_cfg, app.projectiles, app.entities, dummy_stats);

        if (dummy_stats.total_hits > 0) {
            app.score.record_hit();
        } else {
            app.score.record_miss();
        }

        process_new_kills(app, dir);

        auto muzzle_cfg = qe::renderer::ParticleSystem::preset_muzzle_flash();
        muzzle_cfg.position = app.camera.position() + dir * 0.8f;
        muzzle_cfg.orientation = qe::math::Quaternion::from_two_vectors(Vec3(0, 0, 1), dir);
        app.particles.emit(muzzle_cfg);
    }
}

static void update_projectile_collisions(App& app) {
    using namespace qe::math;

    for (auto& proj : app.projectiles) {
        if (!proj.active) {
            continue;
        }
        const auto pb = proj.bounds();
        for (size_t i = 0; i < app.entities.size(); ++i) {
            auto& ent = app.entities[i];
            if (!ent.alive) {
                continue;
            }
            if (pb.intersects(ent.world_bounds())) {
                const bool killed = ent.take_damage(proj.damage);
                if (killed) {
                    const float score_mult = app.powerups.get_score_multiplier();
                    app.score.record_kill(app.combat_cfg.kill_score, score_mult);

                    auto death_cfg = qe::renderer::ParticleSystem::preset_death_burst();
                    death_cfg.position = ent.position;
                    app.particles.emit(death_cfg);

                    app.powerups.try_spawn_random(ent.position + Vec3(0, 0.5f, 0));
                }

                auto sparks_cfg = qe::renderer::ParticleSystem::preset_hit_sparks();
                sparks_cfg.position = proj.position;
                app.particles.emit(sparks_cfg);

                proj.active = false;
                break;
            }
        }
    }
}

static void update_entities(App& app, float dt) {
    const float enemy_speed_mult = app.powerups.get_enemy_speed_multiplier();
    for (size_t i = 0; i < app.entities.size(); ++i) {
        app.entities[i].update(dt);

        if (i < app.behaviors.size() && app.entities[i].alive) {
            app.behaviors[i].update(dt);
            const float adjusted_time = app.time * enemy_speed_mult;
            app.entities[i].position = app.behaviors[i].compute_position(adjusted_time);
            app.entities[i].rotation = app.behaviors[i].compute_rotation(adjusted_time);
        }
    }
}

// ── Public update function ──────────────────────────────────────────────────

void update(App& app, float dt) {
    using namespace qe::game;

    update_camera(app, dt);

    const int alive = count_alive(app.entities);
    app.waves.update(dt, alive);
    update_wave_spawns(app);

    app.weapons.update(dt);
    update_combat_config(app);
    update_shooting(app);

    qe::game::update_projectiles(app.projectiles, dt);
    update_projectile_collisions(app);
    update_entities(app, dt);

    app.powerups.update(dt);
    app.powerups.try_collect(app.camera.position());
    app.particles.update(dt);
    app.score.update(dt);
    app.time += dt;
}

void update_title(App& app) {
    const char* mode = (app.camera.mode() == qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";
    const int alive = count_alive(app.entities);
    const std::string_view weapon_name = weapon_name_for_index(app.weapons.current_index());
    const char* combo = app.score.combo().combo_name();

    std::ostringstream title;
    title << "QuatEngine | " << static_cast<int>(app.current_fps) << " FPS"
          << " | " << mode << " | Wave:" << app.waves.current_wave()
          << " | Score:" << app.score.score() << " | " << weapon_name;

    if (app.weapons.current().max_ammo != -1) {
        title << " [" << app.weapons.current().ammo << "/" << app.weapons.current().max_ammo << "]";
    }

    title << " | Targets:" << alive << "/" << app.entities.size();

    if (combo[0] != '\0') {
        title << " | " << combo;
    }
    if (app.input.gamepad_connected()) {
        title << " | Gamepad: " << app.input.gamepad().name();
    }

    SDL_SetWindowTitle(app.window, title.str().c_str());
}

}  // namespace qe::demo
