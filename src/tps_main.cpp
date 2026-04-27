// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file tps_main.cpp
 * @brief Full rendering entry point for the TPS game.
 *
 * Wires TPSGameState into the QuatEngine renderer: SDL2 window, OpenGL 3.3,
 * camera, meshes, shaders, particles, HUD, and post-processing.
 *
 * Game loop: input → game update → render world → render particles → render HUD
 */

#include "core/EngineConfig.h"
#include "game/tps/TPSGameState.h"
#include "game/tps/TPSScene.h"
#include "game/tps/TPSHUD.h"
#include "game/tps/DamageFeedback.h"

#include "renderer/Camera.h"
#include "renderer/HUD.h"
#include "renderer/Mesh.h"
#include "renderer/MeshPrimitives.h"
#include "renderer/ParticleSystem.h"
#include "renderer/PostProcess.h"
#include "renderer/Shader.h"

#include "input/InputManager.h"

#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"

#include <SDL.h>
#include <iostream>
#include <memory>
#include <string>

// ── App State ────────────────────────────────────────────────────────────────

struct TPSApp {
    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    bool running = true;
    int window_w = qe::config::DEFAULT_WINDOW_WIDTH;
    int window_h = qe::config::DEFAULT_WINDOW_HEIGHT;

    // Engine systems
    qe::input::InputManager input;
    qe::renderer::Camera camera;
    qe::renderer::Shader world_shader;
    qe::renderer::Shader particle_shader;
    qe::renderer::Shader hud_shader;
    qe::renderer::HUD hud;
    qe::renderer::ParticleSystem particles;
    std::unique_ptr<qe::renderer::PostProcess> post_process;

    // Meshes
    qe::renderer::Mesh cube;
    qe::renderer::Mesh sphere;
    qe::renderer::Mesh floor_mesh;
    qe::renderer::Mesh grid;
    qe::renderer::Mesh cylinder;
    qe::renderer::Mesh cone;
    qe::renderer::Mesh capsule;
    qe::renderer::Mesh wedge;
    qe::renderer::Mesh pyramid;

    // Game
    qe::game::tps::TPSGame game;
    float time = 0.0f;
    float current_fps = 60.0f;
    float fps_timer = 0.0f;
    int fps_frames = 0;
};

// ── Forward Declarations ─────────────────────────────────────────────────────

static bool init_window(TPSApp& app);
static void init_gl(TPSApp& app);
static void init_assets(TPSApp& app);
static void handle_events(TPSApp& app);
static qe::game::tps::TPSInputState build_input(TPSApp& app);
static void update(TPSApp& app, float dt);
static void render_world(TPSApp& app);
static void render_particles(TPSApp& app);
static void render_hud(TPSApp& app);
static void shutdown(TPSApp& app);

/** Draw the mesh corresponding to a SceneObjectType. */
static void draw_mesh(TPSApp& app, qe::game::tps::SceneObjectType type) {
    switch (type) {
        case qe::game::tps::SceneObjectType::Cube:     app.cube.draw(); break;
        case qe::game::tps::SceneObjectType::Sphere:   app.sphere.draw(); break;
        case qe::game::tps::SceneObjectType::Floor:    app.floor_mesh.draw(); break;
        case qe::game::tps::SceneObjectType::Cylinder: app.cylinder.draw(); break;
        case qe::game::tps::SceneObjectType::Cone:     app.cone.draw(); break;
        case qe::game::tps::SceneObjectType::Capsule:  app.capsule.draw(); break;
        case qe::game::tps::SceneObjectType::Wedge:    app.wedge.draw(); break;
        case qe::game::tps::SceneObjectType::Pyramid:  app.pyramid.draw(); break;
    }
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    TPSApp app;

    if (!init_window(app)) return 1;
    init_gl(app);
    init_assets(app);

    // Auto-select Vanguard and load level 1
    app.game.select_class(qe::game::tps::CharacterClassType::Vanguard);
    app.game.load_level(1);

    // Set camera to TPS mode
    app.camera.set_mode(qe::renderer::CameraMode::ThirdPerson);
    app.camera.config.orbit_distance = qe::config::DEFAULT_ORBIT_DISTANCE;
    app.camera.config.orbit_height = qe::config::DEFAULT_ORBIT_HEIGHT;
    app.camera.config.orbit_smoothing = qe::config::DEFAULT_ORBIT_SMOOTHING;
    app.camera.config.sensitivity = qe::config::DEFAULT_CAMERA_SENSITIVITY;

    Uint64 prev_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    while (app.running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - prev_time) / static_cast<float>(freq);
        prev_time = now;
        if (dt > qe::config::MAX_DELTA_TIME) dt = qe::config::MAX_DELTA_TIME;

        app.time += dt;

        handle_events(app);
        update(app, dt);

        // Render
        if (app.post_process) app.post_process->bind();

        render_world(app);
        render_particles(app);

        if (app.post_process) {
            app.post_process->unbind();
            app.post_process->draw();
        }

        render_hud(app);
        SDL_GL_SwapWindow(app.window);

        // FPS counter
        app.fps_frames++;
        app.fps_timer += dt;
        if (app.fps_timer >= qe::config::FPS_UPDATE_INTERVAL) {
            app.current_fps = static_cast<float>(app.fps_frames) / app.fps_timer;
            app.fps_frames = 0;
            app.fps_timer = 0.0f;
        }
    }

    shutdown(app);
    return 0;
}

// ── Window Init ──────────────────────────────────────────────────────────────

static bool init_window(TPSApp& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, qe::config::GL_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, qe::config::GL_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, qe::config::MSAA_SAMPLES);

    app.window = SDL_CreateWindow(
        "QuatEngine TPS — Wasteland Protocol",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_w, app.window_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!app.window) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        return false;
    }

    app.gl_context = SDL_GL_CreateContext(app.window);
    SDL_GL_SetSwapInterval(1);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    return true;
}

static void init_gl(TPSApp& app) {
    (void)app;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
}

static void init_assets(TPSApp& app) {
    app.world_shader.load("shaders/world.vert", "shaders/world.frag");
    app.particle_shader.load("shaders/particle.vert", "shaders/particle.frag");
    app.hud_shader.load("shaders/hud.vert", "shaders/hud.frag");

    app.cube = qe::renderer::create_cube();
    app.sphere = qe::renderer::create_sphere(2, 0.5f);
    app.floor_mesh = qe::renderer::create_floor_plane(50.0f, 8.0f);
    app.grid = qe::renderer::create_grid(25, 2.0f);
    app.cylinder = qe::renderer::create_cylinder();
    app.cone = qe::renderer::create_cone();
    app.capsule = qe::renderer::create_capsule();
    app.wedge = qe::renderer::create_wedge();
    app.pyramid = qe::renderer::create_pyramid();

    app.hud.init_crosshair();
    app.input.init();

    app.post_process = std::make_unique<qe::renderer::PostProcess>(
        app.window_w, app.window_h);

    app.camera.config.fov_y = 1.0472f;
    app.camera.config.aspect = static_cast<float>(app.window_w) / app.window_h;
    app.camera.config.move_speed = qe::config::DEFAULT_CAMERA_MOVE_SPEED;
    app.camera.config.smoothing = qe::config::DEFAULT_CAMERA_SMOOTHING;
}

// ── Input ────────────────────────────────────────────────────────────────────

static void handle_events(TPSApp& app) {
    app.input.begin_frame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) { app.running = false; return; }
        app.input.handle_event(event);
    }
    app.input.poll();

    if (app.input.quit()) app.running = false;
}

static qe::game::tps::TPSInputState build_input(TPSApp& app) {
    using namespace qe::game::tps;
    TPSInputState input;
    input.clear();

    input.move_forward = app.input.move_forward();
    input.move_right = app.input.move_right();
    input.look_x = app.input.look_x();
    input.look_y = app.input.look_y();

    input.shoot_held = app.input.shoot_held();
    input.shoot_pressed = app.input.shoot_pressed();
    input.sprint_held = app.input.sprint();

    // Map keyboard keys for TPS actions
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    input.light_melee_pressed = keys[SDL_SCANCODE_V];
    input.heavy_melee_pressed = keys[SDL_SCANCODE_F];
    input.jump_pressed = keys[SDL_SCANCODE_SPACE];
    input.dodge_pressed = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_C];
    input.reload_pressed = app.input.reset();  // R key
    input.parry_pressed = keys[SDL_SCANCODE_G];
    input.ground_slam_pressed = keys[SDL_SCANCODE_X];

    // Lock-on: right mouse button or gamepad LT
    static bool prev_lock = false;
    bool lock_now = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
    input.lock_on_pressed = lock_now && !prev_lock;
    prev_lock = lock_now;

    // Weapon switching
    if (keys[SDL_SCANCODE_Q]) input.prev_weapon_pressed = true;
    if (keys[SDL_SCANCODE_E]) input.next_weapon_pressed = true;

    // Number keys for weapon select
    for (int i = 0; i < 7; ++i) {
        if (keys[SDL_SCANCODE_1 + i]) input.weapon_select = i + 1;
    }

    return input;
}

// ── Update ───────────────────────────────────────────────────────────────────

static void update(TPSApp& app, float dt) {
    auto tps_input = build_input(app);

    // Update game
    app.game.update(tps_input, dt);

    // Update camera to follow player
    const auto& player = app.game.player();
    qe::math::Vec3 cam_target = player.position() + qe::math::Vec3(0, 1.0f, 0);
    app.camera.set_position(cam_target);

    // Camera look input
    app.camera.process_mouse(app.input.look_x(), app.input.look_y());
    app.camera.process_scroll(app.input.zoom());
    app.camera.update(dt);

    // Apply screen shake to camera
    qe::math::Vec3 shake = app.game.feedback().get_shake_offset();
    if (shake.length_squared() > 0.0001f) {
        app.camera.set_position(cam_target + shake);
    }

    // Update particles
    app.particles.update(dt);

    // Emit particles for weapon fire
    if (tps_input.shoot_pressed || tps_input.shoot_held) {
        if (app.game.phase() == qe::game::tps::GamePhase::Playing) {
            auto muzzle_cfg = qe::renderer::ParticleSystem::preset_muzzle_flash();
            muzzle_cfg.position = player.position() + qe::math::Vec3(0, 1.0f, 0)
                + player.rotation().rotate(qe::math::Vec3(0, 0, -1.0f));
            muzzle_cfg.orientation = player.rotation();
            app.particles.emit(muzzle_cfg);
        }
    }

    // Emit particles for hits (from damage feedback)
    for (const auto& dn : app.game.feedback().damage_numbers()) {
        if (dn.age < 0.02f) {  // Just spawned
            auto sparks = qe::renderer::ParticleSystem::preset_hit_sparks();
            sparks.position = dn.world_pos;
            app.particles.emit(sparks);
        }
    }

    // Emit death particles for killed enemies
    for (const auto& enemy : app.game.enemies()) {
        if (!enemy.alive && enemy.current_health <= 0.0f) {
            // Only emit once (check death_timer near start)
            // This is a simplified check — in production use a flag
        }
    }
}

// ── Render World ─────────────────────────────────────────────────────────────

// ── render_world helpers ─────────────────────────────────────────────────────

static void setup_tps_world_shader(TPSApp& app) {
    const auto& env = app.game.scene_data().environment;

    glClearColor(env.sky_color.x * 0.3f, env.sky_color.y * 0.3f,
                 env.sky_color.z * 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app.world_shader.use();

    qe::math::Mat4 vp = app.camera.vp_matrix();
    app.world_shader.set_mat4("u_VP", vp);
    app.world_shader.set_vec3("u_CameraPos", app.camera.position());

    app.world_shader.set_vec3("u_LightDir",
        env.light_direction.length_squared() > 0.01f
        ? env.light_direction.normalized() : qe::math::Vec3(0.3f, -0.8f, 0.5f));
    app.world_shader.set_vec3("u_LightColor",
        qe::math::Vec3(1.0f, 0.95f, 0.9f) * env.light_intensity);
    app.world_shader.set_vec3("u_AmbientColor", env.ambient_color);

    app.world_shader.set_vec3("u_FogColor", env.fog_color);
    app.world_shader.set_float("u_FogStart", env.fog_start);
    app.world_shader.set_float("u_FogEnd", env.fog_end);

    app.world_shader.set_int("u_UseTexture", 0);
}

static void render_tps_scene_objects(TPSApp& app) {
    // Floor
    {
        qe::math::Mat4 model = qe::math::Mat4::trs(
            qe::math::Vec3(0, -0.01f, 0),
            qe::math::Quaternion::identity(),
            app.game.scene_data().ground_scale);
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", qe::math::Vec3(0.25f, 0.22f, 0.18f));
        app.floor_mesh.draw();
    }

    // Cover objects
    for (const auto& obj : app.game.scene_data().cover_objects) {
        qe::math::Mat4 model = qe::math::Mat4::trs(
            obj.position, obj.rotation, obj.scale);
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", obj.color);
        draw_mesh(app, obj.mesh_type);
    }

    // Decorations
    for (const auto& deco : app.game.scene_data().decorations) {
        qe::math::Mat4 model = qe::math::Mat4::trs(
            deco.position, deco.rotation, deco.scale);
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", deco.color);
        draw_mesh(app, deco.mesh_type);
    }
}

static qe::math::Vec3 player_class_color(qe::game::tps::CharacterClassType cls) {
    switch (cls) {
        case qe::game::tps::CharacterClassType::Vanguard:
            return {0.2f, 0.5f, 0.8f};
        case qe::game::tps::CharacterClassType::Recon:
            return {0.3f, 0.8f, 0.3f};
        case qe::game::tps::CharacterClassType::Heavy:
            return {0.8f, 0.5f, 0.2f};
        case qe::game::tps::CharacterClassType::Phantom:
            return {0.5f, 0.2f, 0.7f};
    }
    return {0.5f, 0.5f, 0.5f};
}

static void render_tps_characters(TPSApp& app) {
    // Player
    {
        const auto& player = app.game.player();
        qe::math::Vec3 player_scale(0.4f, 0.9f, 0.4f);
        qe::math::Mat4 model = qe::math::Mat4::trs(
            player.position() + qe::math::Vec3(0, player_scale.y * 0.5f, 0),
            player.rotation(), player_scale);
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", player_class_color(player.class_type()));
        app.capsule.draw();
    }

    // Enemies
    for (const auto& enemy : app.game.enemies()) {
        if (!enemy.alive) continue;

        qe::math::Vec3 scale = qe::game::tps::mutant_scale(enemy.config.type);
        qe::math::Vec3 color = qe::game::tps::mutant_color(enemy.config.type);

        if (enemy.current_health < enemy.config.health * 0.99f) {
            // Recently damaged — brief white flash
        }

        qe::math::Mat4 model = qe::math::Mat4::trs(
            enemy.position + qe::math::Vec3(0, scale.y * 0.5f, 0),
            enemy.rotation, scale);
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", color);

        draw_mesh(app, qe::game::tps::mutant_mesh_type(enemy.config.type));
    }

    // Projectiles
    for (const auto& proj : app.game.projectiles()) {
        if (!proj.active) continue;
        qe::math::Mat4 model = qe::math::Mat4::trs(
            proj.position, qe::math::Quaternion::identity(),
            qe::math::Vec3(0.1f, 0.1f, 0.1f));
        app.world_shader.set_mat4("u_Model", model);
        app.world_shader.set_vec3("u_Tint", proj.color);
        app.sphere.draw();
    }
}

static void render_world(TPSApp& app) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setup_tps_world_shader(app);
    render_tps_scene_objects(app);
    render_tps_characters(app);

    // Grid
    app.world_shader.set_mat4("u_Model", qe::math::Mat4::identity());
    app.world_shader.set_vec3("u_Tint", qe::math::Vec3(0.15f, 0.15f, 0.15f));
    app.grid.draw();
}

// ── Render Particles ─────────────────────────────────────────────────────────

static void render_particles(TPSApp& app) {
    if (app.particles.alive_count() == 0) return;

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    app.particle_shader.use();
    qe::math::Mat4 vp = app.camera.vp_matrix();
    app.particle_shader.set_mat4("u_VP", vp);

    for (const auto& p : app.particles.particles()) {
        if (!p.alive) continue;
        float s = p.current_size();
        qe::math::Vec3 color = p.current_color();
        qe::math::Mat4 model = qe::math::Mat4::trs(
            p.position, p.rotation, qe::math::Vec3(s, s, s));
        app.particle_shader.set_mat4("u_Model", model);
        app.particle_shader.set_vec3("u_Color", color);
        app.particle_shader.set_float("u_Alpha", 1.0f - p.progress());
        app.cube.draw();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

// ── Render HUD ───────────────────────────────────────────────────────────────

// ── render_hud helpers ───────────────────────────────────────────────────────

/** Convert a TPSHUDBarData to a renderer HUDBar and draw it. */
static void draw_hud_bar(qe::renderer::HUD& hud,
                          const qe::game::tps::HUDBarData& src) {
    qe::renderer::HUDBar bar;
    bar.x = src.x;
    bar.y = src.y;
    bar.width = src.width;
    bar.height = src.height;
    bar.fill = src.fill;
    bar.r = src.fill_color.x;
    bar.g = src.fill_color.y;
    bar.b = src.fill_color.z;
    hud.draw_bar(bar);
}

static void render_hud_status_bars(TPSApp& app,
                                    const qe::game::tps::TPSHUDState& hud_state) {
    draw_hud_bar(app.hud, hud_state.health_bar);
    draw_hud_bar(app.hud, hud_state.stamina_bar);

    if (hud_state.boss_health_bar.visible) {
        draw_hud_bar(app.hud, hud_state.boss_health_bar);
    }
}

static void render_hud_combat_indicators(TPSApp& app,
                                          const qe::game::tps::TPSHUDState& hud_state) {
    // Crosshair / Lock-on reticle
    if (hud_state.lock_on_reticle.visible) {
        float rx = hud_state.lock_on_reticle.wobble_x;
        float ry = hud_state.lock_on_reticle.wobble_y;
        float pulse = hud_state.lock_on_reticle.pulse;
        app.hud.draw_indicator(rx, ry, 0.03f + pulse * 0.01f,
            1.0f, 0.3f, 0.2f);
    } else {
        app.hud.draw_crosshair();
    }

    // Hit marker
    if (hud_state.hit_marker_intensity > 0.0f) {
        float hi = hud_state.hit_marker_intensity;
        if (hud_state.hit_marker_kill) {
            app.hud.draw_indicator(0, 0, 0.025f * hi, 1.0f, 0.2f, 0.2f);
        } else {
            app.hud.draw_indicator(0, 0, 0.02f * hi, 1.0f, 1.0f, 1.0f);
        }
    }

    // Weapon slots
    app.hud.draw_weapon_slots(hud_state.weapon_slot_current,
                               hud_state.weapon_slot_total);

    // Damage direction indicators
    for (size_t i = 0; i < hud_state.damage_indicator_angles.size(); ++i) {
        float angle = hud_state.damage_indicator_angles[i];
        float intensity = hud_state.damage_indicator_intensities[i];
        float ix = std::sin(angle) * 0.3f;
        float iy = std::cos(angle) * 0.3f;
        app.hud.draw_indicator(ix, iy, 0.04f * intensity, 0.9f, 0.1f, 0.1f);
    }
}

static void render_hud(TPSApp& app) {
    glDisable(GL_DEPTH_TEST);
    app.hud_shader.use();

    auto hud_state = app.game.build_hud(app.time);

    render_hud_status_bars(app, hud_state);
    render_hud_combat_indicators(app, hud_state);

    glEnable(GL_DEPTH_TEST);
}

// ── Shutdown ─────────────────────────────────────────────────────────────────

static void shutdown(TPSApp& app) {
    app.cube.destroy();
    app.sphere.destroy();
    app.floor_mesh.destroy();
    app.grid.destroy();
    app.cylinder.destroy();
    app.cone.destroy();
    app.capsule.destroy();
    app.wedge.destroy();
    app.pyramid.destroy();
    app.hud.destroy();
    app.world_shader.destroy();
    app.particle_shader.destroy();
    app.hud_shader.destroy();
    if (app.post_process) app.post_process->destroy();

    SDL_GL_DeleteContext(app.gl_context);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}
