/**
 * @file main.cpp
 * @brief QuatEngine Demo — flagship quaternion-based 3D FPS shooter.
 *
 * Orchestrates all engine subsystems:
 *   - Wave system with progressive difficulty
 *   - AI target behaviors using quaternion SLERP paths
 *   - Particle system with quaternion-driven rotation
 *   - Power-up system with quaternion-animated collectibles
 *   - Weapon system with quaternion-based spread mechanics
 *   - Combo scoring system with multipliers
 *   - Enhanced rendering with point lights, emission, and fog
 *   - FPS/TPS dual-mode quaternion camera
 */

#include "game/Combat.h"
#include "game/PowerUp.h"
#include "game/Scene.h"
#include "game/Scoring.h"
#include "game/TargetBehavior.h"
#include "game/WaveSystem.h"
#include "game/Weapons.h"
#include "input/InputManager.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/Camera.h"
#include "renderer/GLLoader.h"
#include "renderer/HUD.h"
#include "renderer/Mesh.h"
#include "renderer/ParticleSystem.h"
#include "renderer/PostProcess.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// ── Application State ───────────────────────────────────────────────────────
struct App {
    // SDL
    SDL_Window*   window     = nullptr;
    SDL_GLContext gl_context = nullptr;
    bool running = true;

    // Subsystems
    qe::input::InputManager   input;
    qe::renderer::Camera      camera;
    qe::renderer::Shader      world_shader;
    qe::renderer::Shader      particle_shader;
    qe::renderer::Shader      hud_shader;
    qe::renderer::HUD         hud;

    // Meshes
    qe::renderer::Mesh cube, sphere, floor_plane, grid;

    // Textures
    qe::renderer::Texture tex_checker, tex_bricks, tex_floor;
    std::vector<qe::renderer::Texture*> textures;

    // Game Systems
    qe::game::WaveSystem       waves;
    qe::game::WeaponManager    weapons;
    qe::game::ScoreTracker     score;
    qe::game::PowerUpManager   powerups;
    qe::renderer::ParticleSystem particles;
    std::unique_ptr<qe::renderer::PostProcess> postProcess;

    // Scene
    std::vector<qe::game::Decoration>       decorations;
    std::vector<qe::core::Entity>           entities;
    std::vector<qe::game::TargetBehavior>   behaviors;
    std::vector<qe::core::Projectile>       projectiles;

    // Combat config (adapts to weapon)
    qe::game::CombatConfig combat_cfg;

    // Visual state
    bool wireframe = false;
    bool slerp_on  = true;

    // Timing
    float  time = 0.0f;
    Uint64 last_time = 0;
    int    frame_count = 0;
    float  fps_timer = 0.0f;
    float  current_fps = 0.0f;
};

// ── Forward Declarations ────────────────────────────────────────────────────
bool init_window(App& app);
bool init_gl(App& app);
void init_assets(App& app);
void handle_events(App& app);
void update(App& app, float dt);
void render_world(App& app);
void render_particles(App& app);
void render_hud(App& app);
void update_title(App& app);
void cleanup(App& app);

// Wave management
void spawn_wave_targets(App& app);
int count_alive(const std::vector<qe::core::Entity>& entities);

// ── Entry Point ─────────────────────────────────────────────────────────────
int main(int /*argc*/, char* /*argv*/[]) {
    App app;

    if (!init_window(app)) return 1;
    if (!init_gl(app))     return 1;
    init_assets(app);

    // Camera
    qe::renderer::Camera::Config cc;
    cc.aspect = 1280.0f / 720.0f;
    cc.smoothing = 0.85f;
    cc.move_speed = 5.0f;
    cc.sprint_mult = 2.5f;
    app.camera = qe::renderer::Camera(cc);
    app.camera.set_position({0, 1.5f, 15});

    // Input
    app.input.init();
    app.input.set_gamepad_look_speed(5.0f);

    // Scene decorations (static, always present)
    app.decorations = qe::game::build_decorations();

    // Start the game immediately
    app.waves.start_game();
    spawn_wave_targets(app);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "\nQuatEngine — Quaternion-Powered FPS Shooter\n"
              << "═══════════════════════════════════════════\n"
              << "  WASD / Left Stick    - Move\n"
              << "  Mouse / Right Stick  - Aim\n"
              << "  Click / RT           - Shoot\n"
              << "  Shift / L3           - Sprint\n"
              << "  Tab / Y              - Camera mode\n"
              << "  Q/E / Bumpers        - Switch weapon\n"
              << "  R / Back             - Reload / Reset\n"
              << "  1-5                  - Select weapon\n"
              << "  F / X                - Wireframe\n"
              << "  Esc / Start+Back     - Quit\n"
              << "═══════════════════════════════════════════\n";

    while (app.running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - app.last_time) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        app.last_time = now;
        if (dt > 0.1f) dt = 0.1f;

        handle_events(app);
        update(app, dt);

        if (app.postProcess) app.postProcess->bind();
        render_world(app);
        render_particles(app);
        if (app.postProcess) app.postProcess->unbind();
        if (app.postProcess) app.postProcess->render(app.time);

        render_hud(app);
        SDL_GL_SwapWindow(app.window);

        app.frame_count++;
        app.fps_timer += dt;
        if (app.fps_timer >= 0.5f) {
            app.current_fps = static_cast<float>(app.frame_count) / app.fps_timer;
            update_title(app);
            app.frame_count = 0;
            app.fps_timer = 0.0f;
        }
    }

    cleanup(app);
    return 0;
}

// ── Init: Window ────────────────────────────────────────────────────────────
bool init_window(App& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    app.window = SDL_CreateWindow("QuatEngine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!app.window) return false;

    app.gl_context = SDL_GL_CreateContext(app.window);
    if (!app.gl_context) return false;

    SDL_GL_SetSwapInterval(1);
    return true;
}

// ── Init: OpenGL ────────────────────────────────────────────────────────────
bool init_gl(App& app) {
    if (!qe::renderer::gl::load()) return false;

    const char* gpu = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_RENDERER));
    std::cout << "GPU: " << (gpu ? gpu : "?") << std::endl;

    using namespace qe::renderer::gl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.02f, 0.02f, 0.06f, 1.0f);

    // World shader (enhanced with point lights, emission, fog)
    if (!app.world_shader.load_from_files("shaders/world.vert", "shaders/world.frag"))
        return false;

    // Particle shader
    if (!app.particle_shader.load_from_files("shaders/particle.vert", "shaders/particle.frag"))
        return false;

    // HUD shader
    if (!app.hud_shader.load_from_files("shaders/hud.vert", "shaders/hud.frag"))
        return false;

    // Post-processing
    app.postProcess = std::make_unique<qe::renderer::PostProcess>(1280, 720);
    app.postProcess->init("shaders/post.vert", "shaders/post.frag");
    app.postProcess->crtEnabled = 1;
    app.postProcess->aberrationEnabled = 1;
    app.postProcess->vignetteEnabled = 1;
    app.postProcess->grainEnabled = 1;

    return true;
}

// ── Init: Assets ────────────────────────────────────────────────────────────
void init_assets(App& app) {
    app.cube        = qe::renderer::Mesh::create_cube();
    app.sphere      = qe::renderer::Mesh::create_sphere(3, 0.5f, 0.8f, 0.6f, 0.3f);
    app.floor_plane = qe::renderer::Mesh::create_floor_plane(40, 12);
    app.grid        = qe::renderer::Mesh::create_grid(40, 1);

    app.tex_checker = qe::renderer::Texture::create_checkerboard(256,8, 220,220,230, 50,50,60);
    app.tex_bricks  = qe::renderer::Texture::create_bricks(256);
    app.tex_floor   = qe::renderer::Texture::create_floor(256);
    app.textures = {&app.tex_checker, &app.tex_bricks, &app.tex_floor};

    app.hud.init_crosshair();
}

// ── Wave Target Spawning ────────────────────────────────────────────────────
void spawn_wave_targets(App& app) {
    using namespace qe::math;
    using namespace qe::game;
    using namespace qe::core;

    const auto& cfg = app.waves.wave_config();
    app.entities.clear();
    app.behaviors.clear();
    app.projectiles.clear();

    // Number of each behavior type (scales with wave)
    int count = cfg.target_count;
    float hp = 50.0f * cfg.health_multiplier;
    float spd = cfg.speed_multiplier;

    for (int i = 0; i < count; ++i) {
        float angle = (2.0f * PI * i) / count;
        float r_min = cfg.spawn_radius_min;
        float r_max = cfg.spawn_radius_max;
        float radius = r_min + (r_max - r_min) * (static_cast<float>(i % 5) / 5.0f);
        float height = 1.0f + (i % 4) * 0.8f;

        Vec3 pos(std::cos(angle) * radius, height, std::sin(angle) * radius);

        Entity ent;
        ent.id = i;
        ent.position = pos;
        ent.spawn_position = pos;
        ent.scale = Vec3(0.7f + (i % 3) * 0.3f, 0.7f + (i % 3) * 0.3f, 0.7f + (i % 3) * 0.3f);
        ent.health = hp;
        ent.max_health = hp;
        ent.local_bounds = AABB::from_center(Vec3::zero(), 0.5f);
        ent.respawn_delay = 999.0f;  // No respawn in wave mode — they stay dead

        // Assign behavior based on index (variety within each wave)
        TargetBehavior behavior;
        int behavior_type = i % 7;  // Cycle through all behavior types
        Vec3 center = pos;

        switch (behavior_type) {
            case 0: behavior = TargetBehavior::create_orbit(center, 3.0f, spd * 0.8f, angle); break;
            case 1: behavior = TargetBehavior::create_figure8(center, 2.5f, spd * 0.6f, angle); break;
            case 2: behavior = TargetBehavior::create_zigzag(center, 2.0f, spd * 1.0f, angle); break;
            case 3: behavior = TargetBehavior::create_spiral(center, 3.0f, spd * 0.5f, angle); break;
            case 4: behavior = TargetBehavior::create_patrol(center, 4.0f, spd * 0.4f, angle); break;
            case 5: behavior = TargetBehavior::create_dodge(center, spd); break;
            default: break; // Static (default-constructed)
        }

        app.entities.push_back(ent);
        app.behaviors.push_back(behavior);
    }
}

int count_alive(const std::vector<qe::core::Entity>& entities) {
    int n = 0;
    for (const auto& e : entities) if (e.alive) n++;
    return n;
}

// ── Events ──────────────────────────────────────────────────────────────────
void handle_events(App& app) {
    app.input.begin_frame();

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { app.running = false; return; }
        if (ev.type == SDL_WINDOWEVENT &&
            ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            qe::renderer::gl::glViewport(0, 0, ev.window.data1, ev.window.data2);
            app.camera.config.aspect =
                static_cast<float>(ev.window.data1) / ev.window.data2;
            if (app.postProcess) {
                app.postProcess->updateResolution(ev.window.data1, ev.window.data2);
            }
        }
        app.input.handle_event(ev);
    }
    app.input.poll();

    // Core actions
    if (app.input.quit())            app.running = false;
    if (app.input.toggle_camera())   app.camera.toggle_mode();
    if (app.input.toggle_wireframe()) {
        app.wireframe = !app.wireframe;
        qe::renderer::gl::glPolygonMode(
            GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
    }

    // Weapon switching via number keys
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys) {
        if (keys[SDL_SCANCODE_1]) app.weapons.switch_weapon(0);
        if (keys[SDL_SCANCODE_2]) app.weapons.switch_weapon(1);
        if (keys[SDL_SCANCODE_3]) app.weapons.switch_weapon(2);
        if (keys[SDL_SCANCODE_4]) app.weapons.switch_weapon(3);
        if (keys[SDL_SCANCODE_5]) app.weapons.switch_weapon(4);
        if (keys[SDL_SCANCODE_Q]) app.weapons.prev_weapon();
        if (keys[SDL_SCANCODE_E]) app.weapons.next_weapon();
    }

    // Reload
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

    // SLERP toggle
    if (app.input.slerp_off()) { app.camera.config.smoothing = 0; app.slerp_on = false; }
    if (app.input.slerp_on())  { app.camera.config.smoothing = 0.85f; app.slerp_on = true; }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(App& app, float dt) {
    using namespace qe::math;
    using namespace qe::game;

    // Camera input
    app.camera.process_mouse(app.input.look_x(), app.input.look_y());
    app.camera.process_scroll(app.input.zoom());
    app.camera.process_movement(
        app.input.move_forward(), app.input.move_right(),
        app.input.move_up(), app.input.sprint(), dt);
    app.camera.update(dt);

    // Wave system
    int alive = count_alive(app.entities);
    app.waves.update(dt, alive);

    // On wave clear → spawn next wave
    if (app.waves.state() == GameState::WaveIntro && alive == 0 &&
        app.waves.time_in_state() < 0.1f && app.waves.current_wave() > 1) {
        // Award wave clear bonus
        app.score.add_bonus(app.waves.wave_config().bonus_points);
        spawn_wave_targets(app);
    }
    // Spawn targets at the start of wave intro for wave 1
    if (app.waves.state() == GameState::WaveIntro && app.entities.empty()) {
        spawn_wave_targets(app);
    }

    // Weapon system update
    app.weapons.update(dt);

    // Apply power-up modifiers to combat config
    float fire_rate_mult = app.powerups.get_fire_rate_multiplier();
    float damage_mult = app.powerups.get_damage_multiplier();
    const auto& wpn = app.weapons.current();
    app.combat_cfg.fire_rate = wpn.fire_rate * fire_rate_mult;
    app.combat_cfg.projectile_damage = wpn.damage * damage_mult;
    app.combat_cfg.projectile_speed = wpn.projectile_speed;
    app.combat_cfg.projectile_lifetime = 3.0f;
    app.combat_cfg.projectile_radius = 0.08f;
    app.combat_cfg.kill_score = 100;

    // Shooting
    if (app.input.shoot_held() && app.weapons.can_fire() &&
        app.waves.state() == GameState::WaveActive) {
        app.weapons.fire();

        // Generate fire directions using quaternion-based spread
        auto dirs = app.weapons.compute_fire_directions(
            app.camera.forward(), Vec3::up());

        for (const auto& dir : dirs) {
            qe::game::CombatStats dummy_stats;
            qe::game::shoot(app.camera.position(), dir,
                           app.combat_cfg, app.projectiles,
                           app.entities, dummy_stats);

            // Track hits via score system
            if (dummy_stats.total_hits > 0) {
                app.score.record_hit();
            } else {
                app.score.record_miss();
            }

            // Check for kills
            for (auto& ent : app.entities) {
                if (!ent.alive && ent.health <= 0 && ent.death_timer < 0.01f) {
                    float score_mult = app.powerups.get_score_multiplier();
                    app.score.record_kill(app.combat_cfg.kill_score, score_mult,
                                         app.waves.wave_config().bonus_points / 10);

                    // Spawn death particles
                    auto death_cfg = qe::renderer::ParticleSystem::preset_death_burst();
                    death_cfg.position = ent.position;
                    app.particles.emit(death_cfg);

                    // Maybe spawn power-up
                    app.powerups.try_spawn_random(ent.position + Vec3(0, 0.5f, 0));

                    // Alert nearby dodge enemies
                    for (size_t j = 0; j < app.behaviors.size(); ++j) {
                        if (app.behaviors[j].type == BehaviorType::Dodge &&
                            app.entities[j].alive) {
                            float dist = ent.position.distance_to(app.entities[j].position);
                            if (dist < 8.0f) {
                                app.behaviors[j].alert();
                            }
                        }
                    }
                }
            }

            // Muzzle flash particles
            auto muzzle_cfg = qe::renderer::ParticleSystem::preset_muzzle_flash();
            muzzle_cfg.position = app.camera.position() + dir * 0.8f;
            muzzle_cfg.orientation = qe::math::Quaternion::from_two_vectors(
                Vec3(0, 0, 1), dir);
            app.particles.emit(muzzle_cfg);
        }
    }

    // Update projectiles
    qe::game::update_projectiles(app.projectiles, dt);

    // Manual projectile-entity collision (with particle effects and scoring)
    for (auto& proj : app.projectiles) {
        if (!proj.active) continue;
        auto pb = proj.bounds();
        for (size_t i = 0; i < app.entities.size(); ++i) {
            auto& ent = app.entities[i];
            if (!ent.alive) continue;
            if (pb.intersects(ent.world_bounds())) {
                bool killed = ent.take_damage(proj.damage);
                if (killed) {
                    float score_mult = app.powerups.get_score_multiplier();
                    app.score.record_kill(app.combat_cfg.kill_score, score_mult);

                    auto death_cfg = qe::renderer::ParticleSystem::preset_death_burst();
                    death_cfg.position = ent.position;
                    app.particles.emit(death_cfg);

                    app.powerups.try_spawn_random(ent.position + Vec3(0, 0.5f, 0));
                }

                // Hit sparks
                auto sparks_cfg = qe::renderer::ParticleSystem::preset_hit_sparks();
                sparks_cfg.position = proj.position;
                app.particles.emit(sparks_cfg);

                proj.active = false;
                break;
            }
        }
    }

    // Entity updates: apply AI behavior positions + standard update
    float enemy_speed_mult = app.powerups.get_enemy_speed_multiplier();
    for (size_t i = 0; i < app.entities.size(); ++i) {
        app.entities[i].update(dt);

        if (i < app.behaviors.size() && app.entities[i].alive) {
            app.behaviors[i].update(dt);
            float adjusted_time = app.time * enemy_speed_mult;
            Vec3 new_pos = app.behaviors[i].compute_position(adjusted_time);
            app.entities[i].position = new_pos;
            app.entities[i].rotation = app.behaviors[i].compute_rotation(adjusted_time);
        }
    }

    // Power-up system
    app.powerups.update(dt);
    app.powerups.try_collect(app.camera.position());

    // Particle system
    app.particles.update(dt);

    // Scoring system
    app.score.update(dt);

    // Game over check: in a wave-based game, you don't really "die"
    // but we can add time pressure or other conditions later

    app.time += dt;
}

// ── Render: World ───────────────────────────────────────────────────────────
void render_world(App& app) {
    using namespace qe::renderer::gl;
    using namespace qe::math;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    app.world_shader.use();

    Mat4 vp = app.camera.vp_matrix();
    app.world_shader.set_mat4("uViewProjection", vp);
    app.world_shader.set_vec3("uLightDir", Vec3(0.3f, 0.8f, 0.5f).normalized());
    app.world_shader.set_vec3("uLightColor", Vec3(1, 0.95f, 0.9f));
    app.world_shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    app.world_shader.set_vec3("uCameraPos", app.camera.position());
    app.world_shader.set_int("uTexture0", 0);
    app.world_shader.set_float("uTime", app.time);

    // Default fog
    app.world_shader.set_float("uFogNear", 30.0f);
    app.world_shader.set_float("uFogFar", 80.0f);
    app.world_shader.set_vec3("uFogColor", Vec3(0.02f, 0.02f, 0.06f));

    // Default no emission
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);

    // Rim light for sci-fi feel
    app.world_shader.set_float("uRimPower", 3.0f);
    app.world_shader.set_vec3("uRimColor", Vec3(0.1f, 0.2f, 0.4f));

    // Point lights from active power-ups and projectiles
    int point_light_count = 0;
    auto set_point_light = [&](const Vec3& pos, const Vec3& color, float radius) {
        if (point_light_count >= 8) return;
        std::string prefix = "uPointLightPos[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_vec3(prefix, pos);
        prefix = "uPointLightColor[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_vec3(prefix, color);
        prefix = "uPointLightRadius[" + std::to_string(point_light_count) + "]";
        app.world_shader.set_float(prefix, radius);
        point_light_count++;
    };

    // Projectiles emit light
    for (const auto& p : app.projectiles) {
        if (p.active && point_light_count < 6) {
            set_point_light(p.position, Vec3(1.0f, 0.7f, 0.2f) * p.brightness, 5.0f);
        }
    }

    // Power-ups emit light
    for (const auto& p : app.powerups.pickups()) {
        if (p.alive && point_light_count < 8) {
            auto cfg = qe::game::PowerUpManager::get_config(p.type);
            set_point_light(p.display_position(), cfg.color * p.glow_intensity(), 4.0f);
        }
    }

    app.world_shader.set_int("uPointLightCount", point_light_count);

    // ── Decorations ─────────────────────────────────────────────────────
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);

    for (const auto& d : app.decorations) {
        app.world_shader.set_mat4("uModel", d.model_matrix(app.time));
        bool textured = d.texture_id >= 0 &&
            d.texture_id < static_cast<int>(app.textures.size());
        app.world_shader.set_int("uUseTexture", textured ? 1 : 0);
        if (textured) app.textures[d.texture_id]->bind(0);

        if (d.mesh_type == qe::game::Decoration::MeshType::Floor) {
            glDisable(GL_CULL_FACE);
            app.floor_plane.draw();
            glEnable(GL_CULL_FACE);
        } else if (d.mesh_type == qe::game::Decoration::MeshType::Sphere) {
            app.sphere.draw();
        } else {
            app.cube.draw();
        }
    }

    // ── Entities (targets) ──────────────────────────────────────────────
    app.world_shader.set_int("uUseTexture", 0);
    for (const auto& ent : app.entities) {
        if (!ent.alive) {
            // Death animation with quaternion spin
            if (ent.death_timer < 1.0f) {
                float t = ent.death_timer;
                app.world_shader.set_mat4("uModel",
                    Mat4::trs(ent.position + Vec3(0, t*2, 0),
                              Quaternion::from_axis_angle(Vec3::up(), t*10),
                              ent.scale * (1-t)));
                app.world_shader.set_vec3("uEmission", Vec3(1.0f, 0.3f, 0.1f));
                app.world_shader.set_float("uEmissionStrength", 1.0f - t);
                app.sphere.draw();
                app.world_shader.set_vec3("uEmission", Vec3::zero());
                app.world_shader.set_float("uEmissionStrength", 0.0f);
            }
            continue;
        }

        app.world_shader.set_mat4("uModel", Mat4::trs(ent.position, ent.rotation, ent.scale));

        // Hit flash with emission
        if (ent.hit_flash > 0) {
            float f = ent.hit_flash / 0.3f;
            app.world_shader.set_vec3("uEmission", Vec3(f, f * 0.5f, f * 0.2f));
            app.world_shader.set_float("uEmissionStrength", f);
        }

        app.cube.draw();

        if (ent.hit_flash > 0) {
            app.world_shader.set_vec3("uEmission", Vec3::zero());
            app.world_shader.set_float("uEmissionStrength", 0.0f);
        }
    }

    // ── Projectiles ─────────────────────────────────────────────────────
    for (const auto& p : app.projectiles) {
        if (!p.active) continue;
        app.world_shader.set_vec3("uEmission", Vec3(1.0f, 0.8f, 0.3f));
        app.world_shader.set_float("uEmissionStrength", p.brightness);
        app.world_shader.set_mat4("uModel",
            Mat4::trs(p.position, Quaternion::identity(), Vec3(0.1f, 0.1f, 0.1f)));
        app.cube.draw();
    }
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);

    // ── Power-Ups ───────────────────────────────────────────────────────
    for (const auto& p : app.powerups.pickups()) {
        if (!p.alive) continue;
        auto cfg = qe::game::PowerUpManager::get_config(p.type);
        float glow = p.glow_intensity();

        app.world_shader.set_vec3("uEmission", cfg.color * glow);
        app.world_shader.set_float("uEmissionStrength", glow);
        app.world_shader.set_mat4("uModel",
            Mat4::trs(p.display_position(), p.rotation, Vec3(0.4f, 0.4f, 0.4f)));
        app.cube.draw();
    }
    app.world_shader.set_vec3("uEmission", Vec3::zero());
    app.world_shader.set_float("uEmissionStrength", 0.0f);

    // ── Grid ────────────────────────────────────────────────────────────
    glDisable(GL_CULL_FACE);
    app.world_shader.set_int("uUseTexture", 0);
    app.world_shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

// ── Render: Particles ───────────────────────────────────────────────────────
void render_particles(App& app) {
    using namespace qe::renderer::gl;
    using namespace qe::math;

    if (app.particles.alive_count() == 0) return;

    app.particle_shader.use();
    Mat4 vp = app.camera.vp_matrix();
    app.particle_shader.set_mat4("uViewProjection", vp);

    // Additive blending for particles
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (const auto& p : app.particles.particles()) {
        Vec3 color = p.current_color();
        float size = p.current_size();
        float alpha = 1.0f - p.progress();

        // Use quaternion rotation for each particle's orientation
        app.particle_shader.set_mat4("uModel",
            Mat4::trs(p.position, p.rotation, Vec3(size, size, size)));
        app.particle_shader.set_float("uAlpha", alpha);

        // Set particle color via vertex color override
        // (particles use the cube mesh with overridden color)
        app.cube.draw();
    }

    // Restore standard blending
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ── Render: HUD ─────────────────────────────────────────────────────────────
void render_hud(App& app) {
    using namespace qe::renderer::gl;

    glDisable(GL_DEPTH_TEST);
    app.hud_shader.use();
    app.hud_shader.set_float("uAlpha", 0.8f);

    // Crosshair
    glLineWidth(2.0f);
    app.hud.draw_crosshair();
    glLineWidth(1.0f);

    // Health bar (top-left)
    qe::renderer::HUDBar health_bar;
    health_bar.x = -0.95f;
    health_bar.y = 0.88f;
    health_bar.width = 0.4f;
    health_bar.height = 0.04f;
    health_bar.fill = 1.0f; // Player doesn't take damage yet, always full
    health_bar.r = 0.2f; health_bar.g = 0.9f; health_bar.b = 0.3f;
    app.hud.draw_bar(health_bar);

    // Ammo bar (bottom-right)
    const auto& wpn = app.weapons.current();
    if (wpn.max_ammo != -1) {
        qe::renderer::HUDBar ammo_bar;
        ammo_bar.x = 0.55f;
        ammo_bar.y = -0.95f;
        ammo_bar.width = 0.4f;
        ammo_bar.height = 0.03f;
        ammo_bar.fill = static_cast<float>(wpn.ammo) / wpn.max_ammo;
        ammo_bar.r = 0.9f; ammo_bar.g = 0.8f; ammo_bar.b = 0.2f;
        app.hud.draw_bar(ammo_bar);
    }

    // Reload progress bar
    if (app.weapons.is_reloading()) {
        qe::renderer::HUDBar reload_bar;
        reload_bar.x = -0.15f;
        reload_bar.y = -0.15f;
        reload_bar.width = 0.3f;
        reload_bar.height = 0.02f;
        reload_bar.fill = app.weapons.reload_progress();
        reload_bar.r = 0.2f; reload_bar.g = 0.6f; reload_bar.b = 1.0f;
        app.hud.draw_bar(reload_bar);
    }

    // Weapon slots
    app.hud.draw_weapon_slots(app.weapons.current_index(), app.weapons.weapon_count());

    // Combo indicator
    app.hud.draw_combo_indicator(app.score.combo().streak, app.time);

    // Active power-up effects (right side)
    float effect_y = 0.8f;
    for (const auto& e : app.powerups.effects()) {
        if (!e.is_active()) continue;
        auto cfg = qe::game::PowerUpManager::get_config(e.type);
        qe::renderer::HUDBar effect_bar;
        effect_bar.x = 0.6f;
        effect_bar.y = effect_y;
        effect_bar.width = 0.35f;
        effect_bar.height = 0.025f;
        effect_bar.fill = 1.0f - e.progress();
        effect_bar.r = cfg.color.x;
        effect_bar.g = cfg.color.y;
        effect_bar.b = cfg.color.z;
        app.hud.draw_bar(effect_bar);
        effect_y -= 0.04f;
    }

    // Wave intro/clear overlay indicators
    if (app.waves.state() == qe::game::GameState::WaveIntro) {
        float pulse = 0.5f + 0.5f * std::sin(app.time * 4.0f);
        app.hud.draw_indicator(0.0f, 0.2f, 0.08f, pulse, pulse, pulse);
    }

    glEnable(GL_DEPTH_TEST);
}

// ── Title ───────────────────────────────────────────────────────────────────
void update_title(App& app) {
    const char* mode =
        (app.camera.mode() == qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";
    int alive = count_alive(app.entities);

    // Weapon name
    const char* wpn_names[] = {"Pistol", "Shotgun", "RailGun", "Rocket", "MiniGun"};
    int wpn_idx = app.weapons.current_index();
    const char* wpn_name = (wpn_idx >= 0 && wpn_idx < 5) ? wpn_names[wpn_idx] : "?";

    // Combo name
    const char* combo = app.score.combo().combo_name();

    std::ostringstream t;
    t << "QuatEngine | " << static_cast<int>(app.current_fps) << " FPS"
      << " | " << mode
      << " | Wave:" << app.waves.current_wave()
      << " | Score:" << app.score.score()
      << " | " << wpn_name;

    if (app.weapons.current().max_ammo != -1) {
        t << " [" << app.weapons.current().ammo << "/" << app.weapons.current().max_ammo << "]";
    }

    t << " | Targets:" << alive << "/" << app.entities.size();

    if (combo[0] != '\0') {
        t << " | " << combo;
    }

    if (app.input.gamepad_connected())
        t << " | Gamepad: " << app.input.gamepad().name();

    SDL_SetWindowTitle(app.window, t.str().c_str());
}

// ── Cleanup ─────────────────────────────────────────────────────────────────
void cleanup(App& app) {
    app.cube.destroy();
    app.sphere.destroy();
    app.floor_plane.destroy();
    app.grid.destroy();
    app.hud.destroy();
    app.world_shader.destroy();
    app.particle_shader.destroy();
    app.hud_shader.destroy();
    app.tex_checker.destroy();
    app.tex_bricks.destroy();
    app.tex_floor.destroy();
    if (app.gl_context) SDL_GL_DeleteContext(app.gl_context);
    if (app.window) SDL_DestroyWindow(app.window);
    SDL_Quit();
}
