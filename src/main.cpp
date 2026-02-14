/**
 * @file main.cpp
 * @brief QuatEngine Demo — thin orchestration layer.
 *
 * This file ONLY wires modules together. All logic lives in:
 *   - input/InputManager.h   (keyboard, mouse, gamepad)
 *   - game/Scene.h           (scene data + factories)
 *   - game/Combat.h          (shooting, collision, scoring)
 *   - renderer/Camera.h      (FPS/TPS quaternion camera)
 *   - renderer/*             (GL, meshes, textures, shaders)
 */

#include "game/Combat.h"
#include "game/Scene.h"
#include "input/InputManager.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/Camera.h"
#include "renderer/GLLoader.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"

#include <SDL.h>

#include <iostream>
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
    qe::input::InputManager input;
    qe::renderer::Camera    camera;
    qe::renderer::Shader    world_shader;
    qe::renderer::Shader    hud_shader;

    // Meshes
    qe::renderer::Mesh cube, sphere, floor_plane, grid, crosshair;

    // Textures
    qe::renderer::Texture tex_checker, tex_bricks, tex_floor;
    std::vector<qe::renderer::Texture*> textures;

    // Game state
    std::vector<qe::game::Decoration>  decorations;
    std::vector<qe::core::Entity>      entities;
    std::vector<qe::core::Projectile>  projectiles;
    qe::game::CombatStats  stats;
    qe::game::CombatConfig combat_cfg;
    float shoot_cooldown = 0.0f;

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

// ── Forward Declarations (one per responsibility) ───────────────────────────
bool init_window(App& app);
bool init_gl(App& app);
void init_assets(App& app);
void init_crosshair(App& app);
void handle_events(App& app);
void update(App& app, float dt);
void render_world(App& app);
void render_hud(App& app);
void update_title(App& app);
void cleanup(App& app);

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

    // Scene
    app.decorations = qe::game::build_decorations();
    app.entities = qe::game::spawn_targets();

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "\nQuatEngine Phase 5 — FPS Shooter + Gamepad\n"
              << "  WASD / Left Stick   - Move\n"
              << "  Mouse / Right Stick - Aim\n"
              << "  Click / RT          - Shoot\n"
              << "  Shift / L3          - Sprint\n"
              << "  Tab / Y             - Camera mode\n"
              << "  R / Back            - Reset targets\n"
              << "  F / X               - Wireframe\n"
              << "  Esc / Start+Back    - Quit\n";

    while (app.running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - app.last_time) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        app.last_time = now;
        if (dt > 0.1f) dt = 0.1f;

        handle_events(app);
        update(app, dt);
        render_world(app);
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
    glClearColor(0.03f, 0.03f, 0.08f, 1.0f);

    // Compile shaders
    if (!app.world_shader.load_from_files("shaders/basic.vert", "shaders/basic.frag"))
        return false;

    const char* hud_v = R"(#version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=2) in vec3 aColor;
        out vec3 vColor;
        void main() { gl_Position=vec4(aPos,1); vColor=aColor; })";
    const char* hud_f = R"(#version 330 core
        in vec3 vColor; out vec4 FragColor;
        void main() { FragColor=vec4(vColor,0.8); })";
    return app.hud_shader.compile(hud_v, hud_f);
}

// ── Init: Assets ────────────────────────────────────────────────────────────
void init_assets(App& app) {
    app.cube        = qe::renderer::Mesh::create_cube();
    app.sphere      = qe::renderer::Mesh::create_sphere(3, 0.5f, 0.8f, 0.6f, 0.3f);
    app.floor_plane = qe::renderer::Mesh::create_floor_plane(30, 10);
    app.grid        = qe::renderer::Mesh::create_grid(30, 1);

    app.tex_checker = qe::renderer::Texture::create_checkerboard(256,8, 220,220,230, 50,50,60);
    app.tex_bricks  = qe::renderer::Texture::create_bricks(256);
    app.tex_floor   = qe::renderer::Texture::create_floor(256);
    app.textures = {&app.tex_checker, &app.tex_bricks, &app.tex_floor};

    init_crosshair(app);
}

void init_crosshair(App& app) {
    using qe::renderer::Vertex;
    float s = 0.02f, g = 0.005f;
    float c = 0.9f;
    std::vector<Vertex> v = {
        {{-s,0,0},{0,0,1},{c,1,c},{0,0}}, {{-g,0,0},{0,0,1},{c,1,c},{0,0}},
        {{ g,0,0},{0,0,1},{c,1,c},{0,0}}, {{ s,0,0},{0,0,1},{c,1,c},{0,0}},
        {{0, g,0},{0,0,1},{c,1,c},{0,0}}, {{0, s,0},{0,0,1},{c,1,c},{0,0}},
        {{0,-s,0},{0,0,1},{c,1,c},{0,0}}, {{0,-g,0},{0,0,1},{c,1,c},{0,0}},
    };
    std::vector<unsigned> idx = {0,1, 2,3, 4,5, 6,7};
    app.crosshair.upload(v, idx);
    app.crosshair.index_count = 8;
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
        }
        app.input.handle_event(ev);
    }
    app.input.poll();

    // Action mapping (thin — just maps input→state)
    if (app.input.quit())            app.running = false;
    if (app.input.toggle_camera())   app.camera.toggle_mode();
    if (app.input.toggle_wireframe()) {
        app.wireframe = !app.wireframe;
        qe::renderer::gl::glPolygonMode(
            GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
    }
    if (app.input.reset()) {
        app.entities = qe::game::spawn_targets();
        app.stats.reset();
    }
    if (app.input.slerp_off()) { app.camera.config.smoothing = 0; app.slerp_on = false; }
    if (app.input.slerp_on())  { app.camera.config.smoothing = 0.85f; app.slerp_on = true; }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(App& app, float dt) {
    // Camera input (unified: keyboard + gamepad)
    app.camera.process_mouse(app.input.look_x(), app.input.look_y());
    app.camera.process_scroll(app.input.zoom());
    app.camera.process_movement(
        app.input.move_forward(), app.input.move_right(),
        app.input.move_up(), app.input.sprint(), dt);
    app.camera.update(dt);

    // Shooting
    app.shoot_cooldown -= dt;
    if (app.shoot_cooldown < 0) app.shoot_cooldown = 0;
    if (app.input.shoot_held() && app.shoot_cooldown <= 0) {
        app.shoot_cooldown = app.combat_cfg.fire_rate;
        qe::game::shoot(app.camera.position(), app.camera.forward(),
                         app.combat_cfg, app.projectiles,
                         app.entities, app.stats);
    }

    // Projectiles & collisions
    qe::game::update_projectiles(app.projectiles, dt);
    qe::game::check_projectile_collisions(
        app.projectiles, app.entities, app.stats, app.combat_cfg.kill_score);

    // Entity updates
    for (auto& ent : app.entities) ent.update(dt);

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

    // Decorations
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

    // Entities
    app.world_shader.set_int("uUseTexture", 0);
    for (const auto& ent : app.entities) {
        if (!ent.alive) {
            if (ent.death_timer < 1.0f) {
                float t = ent.death_timer;
                app.world_shader.set_mat4("uModel",
                    Mat4::trs(ent.position + Vec3(0, t*2, 0),
                              Quaternion::from_axis_angle(Vec3::up(), t*10),
                              ent.scale * (1-t)));
                app.world_shader.set_vec3("uAmbient", Vec3(0.5f, 0.1f, 0.1f));
                app.sphere.draw();
                app.world_shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
            }
            continue;
        }

        app.world_shader.set_mat4("uModel", Mat4::trs(ent.position, ent.rotation, ent.scale));
        if (ent.hit_flash > 0) {
            float f = ent.hit_flash / 0.3f;
            app.world_shader.set_vec3("uAmbient", Vec3(f, f, f));
        }
        app.cube.draw();
        if (ent.hit_flash > 0)
            app.world_shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    }

    // Projectiles
    for (const auto& p : app.projectiles) {
        app.world_shader.set_vec3("uAmbient", Vec3(p.brightness, p.brightness*0.8f, p.brightness*0.3f));
        app.world_shader.set_mat4("uModel",
            Mat4::trs(p.position, Quaternion::identity(), Vec3(0.1f, 0.1f, 0.1f)));
        app.cube.draw();
    }
    app.world_shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));

    // Grid
    glDisable(GL_CULL_FACE);
    app.world_shader.set_int("uUseTexture", 0);
    app.world_shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

// ── Render: HUD ─────────────────────────────────────────────────────────────
void render_hud(App& app) {
    using namespace qe::renderer::gl;
    glDisable(GL_DEPTH_TEST);
    app.hud_shader.use();
    glLineWidth(2.0f);
    glBindVertexArray(app.crosshair.vao);
    glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

// ── Title ───────────────────────────────────────────────────────────────────
void update_title(App& app) {
    const char* mode =
        (app.camera.mode() == qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";
    int alive = 0;
    for (const auto& e : app.entities) if (e.alive) alive++;

    std::ostringstream t;
    t << "QuatEngine | " << static_cast<int>(app.current_fps) << " FPS"
      << " | " << mode
      << " | Score:" << app.stats.score
      << " | Acc:" << static_cast<int>(app.stats.accuracy()) << "%"
      << " | Targets:" << alive << "/" << app.entities.size();
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
    app.crosshair.destroy();
    app.world_shader.destroy();
    app.hud_shader.destroy();
    app.tex_checker.destroy();
    app.tex_bricks.destroy();
    app.tex_floor.destroy();
    if (app.gl_context) SDL_GL_DeleteContext(app.gl_context);
    if (app.window) SDL_DestroyWindow(app.window);
    SDL_Quit();
}
