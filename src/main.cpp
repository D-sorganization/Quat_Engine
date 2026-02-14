/**
 * @file main.cpp
 * @brief QuatEngine Phase 5 Demo — FPS Shooting, Collision, Entities.
 *
 * Phase 5 adds:
 *   - Left-click shooting (projectile + hitscan)
 *   - AABB collision detection
 *   - Destructible entities with health, hit flash, and respawn
 *   - Crosshair overlay
 *   - Projectile rendering (bright cubes)
 *   - Score tracking
 *   - All previous features (textures, FPS/TPS camera, OBJ, etc.)
 *
 * Controls:
 *   Left Click     - Shoot
 *   WASD / Mouse   - Move & Look
 *   Shift          - Sprint
 *   Space / C      - Up / Down
 *   Tab            - FPS / TPS toggle
 *   Scroll         - Zoom (TPS)
 *   R              - Reset all entities
 *   1 / 2          - SLERP off / on
 *   F              - Wireframe
 *   Escape         - Quit
 */

#include "core/AABB.h"
#include "core/Entity.h"
#include "core/Projectile.h"
#include "core/Transform.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/Camera.h"
#include "renderer/GLLoader.h"
#include "renderer/Mesh.h"
#include "renderer/OBJLoader.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

constexpr int   WINDOW_WIDTH  = 1280;
constexpr int   WINDOW_HEIGHT = 720;
constexpr float PI = 3.14159265358979f;

// ── Scene Object (static decoration) ────────────────────────────────────────
struct SceneObject {
    qe::math::Vec3       position;
    qe::math::Quaternion rotation;
    qe::math::Vec3       scale;
    enum class Anim { None, SpinY, SpinTilted };
    Anim anim = Anim::None;
    float anim_speed = 1.0f;
    float anim_phase = 0.0f;
    enum class MeshType { Cube, Sphere, Floor };
    MeshType mesh_type = MeshType::Cube;
    int texture_id = -1;

    qe::math::Mat4 model_matrix(float time) const {
        qe::math::Quaternion rot = rotation;
        if (anim == Anim::SpinY)
            rot = qe::math::Quaternion::from_axis_angle(
                qe::math::Vec3::up(), time * anim_speed + anim_phase) * rot;
        else if (anim == Anim::SpinTilted)
            rot = qe::math::Quaternion::from_axis_angle(
                qe::math::Vec3(0,1,0.3f).normalized(), time*anim_speed+anim_phase) * rot;
        return qe::math::Mat4::trs(position, rot, scale);
    }
};

// ── Application State ───────────────────────────────────────────────────────
struct AppState {
    SDL_Window*   window      = nullptr;
    SDL_GLContext gl_context  = nullptr;
    bool running   = true;
    bool wireframe = false;
    bool slerp_on  = true;

    qe::renderer::Camera camera;
    qe::renderer::Shader shader;
    qe::renderer::Shader crosshair_shader;

    // Meshes
    qe::renderer::Mesh cube, sphere, floor_plane, grid;
    qe::renderer::Mesh crosshair_mesh;

    // Textures
    qe::renderer::Texture tex_checker, tex_bricks, tex_floor, tex_white;
    std::vector<qe::renderer::Texture*> textures;

    // Scene
    std::vector<SceneObject> decorations;
    std::vector<qe::core::Entity> entities;
    std::vector<qe::core::Projectile> projectiles;

    // Shooting
    float shoot_cooldown = 0.0f;
    float projectile_speed = 40.0f;
    int score = 0;
    int total_shots = 0;
    int total_hits = 0;

    // Timing
    float  time = 0.0f;
    Uint64 last_time = 0;
    int    frame_count = 0;
    float  fps_timer = 0.0f;
    float  current_fps = 0.0f;
};

// Forward declarations
bool init_sdl(AppState& app);
bool init_opengl(AppState& app);
void create_textures(AppState& app);
void create_crosshair(AppState& app);
void build_scene(AppState& app);
void spawn_entities(AppState& app);
void shoot(AppState& app);
void update_projectiles(AppState& app, float dt);
void check_collisions(AppState& app);
void process_events(AppState& app);
void update(AppState& app, float dt);
void render(AppState& app);
void render_crosshair(AppState& app);
void cleanup(AppState& app);
void update_title(AppState& app);

// ── Entry Point ─────────────────────────────────────────────────────────────
int main(int /*argc*/, char* /*argv*/[]) {
    AppState app;

    if (!init_sdl(app))    return 1;
    if (!init_opengl(app)) return 1;

    // Camera
    qe::renderer::Camera::Config cfg;
    cfg.aspect = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    cfg.smoothing = 0.85f;
    cfg.move_speed = 5.0f;
    cfg.sprint_mult = 2.5f;
    app.camera = qe::renderer::Camera(cfg);
    app.camera.set_position(qe::math::Vec3(0, 1.5f, 15));

    // Shaders
    if (!app.shader.load_from_files("shaders/basic.vert", "shaders/basic.frag")) {
        std::cerr << "Failed to compile main shader" << std::endl;
        cleanup(app); return 1;
    }

    // Crosshair shader (embedded — no file needed)
    const char* ch_vert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 2) in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";
    const char* ch_frag = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 0.8);
        }
    )";
    app.crosshair_shader.compile(ch_vert, ch_frag);

    create_textures(app);
    create_crosshair(app);

    app.cube = qe::renderer::Mesh::create_cube();
    app.sphere = qe::renderer::Mesh::create_sphere(3, 0.5f, 0.8f, 0.6f, 0.3f);
    app.floor_plane = qe::renderer::Mesh::create_floor_plane(30, 10);
    app.grid = qe::renderer::Mesh::create_grid(30, 1);

    build_scene(app);
    spawn_entities(app);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "\nQuatEngine Phase 5 — FPS Shooter Demo" << std::endl;
    std::cout << "  Left Click      - SHOOT!" << std::endl;
    std::cout << "  WASD / Mouse    - Move & Look" << std::endl;
    std::cout << "  Shift           - Sprint" << std::endl;
    std::cout << "  Tab             - FPS / TPS" << std::endl;
    std::cout << "  R               - Reset entities" << std::endl;
    std::cout << "  F               - Wireframe" << std::endl;
    std::cout << "  Escape          - Quit" << std::endl;

    while (app.running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - app.last_time) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        app.last_time = now;
        if (dt > 0.1f) dt = 0.1f;

        process_events(app);
        update(app, dt);
        render(app);
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

// ── SDL / OpenGL ────────────────────────────────────────────────────────────
bool init_sdl(AppState& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    app.window = SDL_CreateWindow("QuatEngine — Phase 5",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!app.window) return false;

    app.gl_context = SDL_GL_CreateContext(app.window);
    if (!app.gl_context) return false;

    SDL_GL_SetSwapInterval(1);
    return true;
}

bool init_opengl(AppState& /*app*/) {
    if (!qe::renderer::gl::load()) {
        std::cerr << "Failed to load GL" << std::endl;
        return false;
    }
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
    return true;
}

void create_textures(AppState& app) {
    app.tex_checker = qe::renderer::Texture::create_checkerboard(256, 8, 220,220,230, 50,50,60);
    app.tex_bricks  = qe::renderer::Texture::create_bricks(256);
    app.tex_floor   = qe::renderer::Texture::create_floor(256);
    app.tex_white   = qe::renderer::Texture::create_solid(255,255,255);
    app.textures = {&app.tex_checker, &app.tex_bricks, &app.tex_floor, &app.tex_white};
}

// ── Crosshair (NDC-space mesh) ──────────────────────────────────────────────
void create_crosshair(AppState& app) {
    using qe::renderer::Vertex;
    float s = 0.02f;   // Size in NDC
    float g = 0.005f;  // Gap in center
    float r = 0.8f, gc = 1.0f, b = 0.8f;  // Light green

    // 4 short lines forming a + with a gap in the middle
    std::vector<Vertex> verts = {
        // Horizontal left
        {{-s, 0, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        {{-g, 0, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        // Horizontal right
        {{ g, 0, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        {{ s, 0, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        // Vertical top
        {{0,  g, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        {{0,  s, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        // Vertical bottom
        {{0, -s, 0}, {0,0,1}, {r,gc,b}, {0,0}},
        {{0, -g, 0}, {0,0,1}, {r,gc,b}, {0,0}},
    };
    std::vector<unsigned int> indices = {0,1, 2,3, 4,5, 6,7};

    app.crosshair_mesh.index_count = 8;
    using namespace qe::renderer::gl;
    glGenVertexArrays(1, &app.crosshair_mesh.vao);
    glGenBuffers(1, &app.crosshair_mesh.vbo);
    glGenBuffers(1, &app.crosshair_mesh.ebo);
    glBindVertexArray(app.crosshair_mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, app.crosshair_mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size()*sizeof(Vertex)),
                 verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app.crosshair_mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size()*sizeof(unsigned)),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex,position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex,color)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// ── Scene & Entity Setup ────────────────────────────────────────────────────
void build_scene(AppState& app) {
    using namespace qe::math;

    // Floor
    app.decorations.push_back({
        Vec3(0,-0.01f,0), Quaternion::identity(), Vec3::one(),
        SceneObject::Anim::None, 0, 0, SceneObject::MeshType::Floor, 2
    });

    // Brick pillars
    for (int i = 0; i < 8; ++i) {
        float a = (2*PI*i)/8;
        float h = 2.0f + (i%3)*1.5f;
        app.decorations.push_back({
            Vec3(std::cos(a)*14, h*0.5f, std::sin(a)*14),
            Quaternion::from_axis_angle(Vec3::up(), a),
            Vec3(0.8f, h, 0.8f),
            SceneObject::Anim::None, 0, 0, SceneObject::MeshType::Cube, 1
        });
    }

    // Floating platforms
    for (int i = 0; i < 5; ++i) {
        float a = (2*PI*i)/5 + 0.3f;
        float r = 8.0f + i*0.5f;
        app.decorations.push_back({
            Vec3(std::cos(a)*r, 0.1f+i*0.8f, std::sin(a)*r),
            Quaternion::from_axis_angle(Vec3::up(), a),
            Vec3(2, 0.15f, 2),
            SceneObject::Anim::None, 0, 0, SceneObject::MeshType::Cube, 2
        });
    }
}

void spawn_entities(AppState& app) {
    using namespace qe::math;
    app.entities.clear();

    // Ring of target entities
    constexpr int TARGET_COUNT = 12;
    for (int i = 0; i < TARGET_COUNT; ++i) {
        float a = (2*PI*i) / TARGET_COUNT;
        float r = 10.0f;
        float y = 1.0f + (i % 3) * 1.2f;

        qe::core::Entity ent;
        ent.id = i;
        ent.position = Vec3(std::cos(a)*r, y, std::sin(a)*r);
        ent.spawn_position = ent.position;
        ent.scale = Vec3(0.8f, 0.8f, 0.8f);
        ent.health = 50.0f;
        ent.max_health = 50.0f;
        ent.local_bounds = qe::core::AABB::from_center(Vec3::zero(), 0.5f);
        ent.respawn_delay = 4.0f;
        app.entities.push_back(ent);
    }

    // Bigger targets further out
    for (int i = 0; i < 6; ++i) {
        float a = (2*PI*i)/6 + PI/6;
        qe::core::Entity ent;
        ent.id = TARGET_COUNT + i;
        ent.position = Vec3(std::cos(a)*18, 2.0f, std::sin(a)*18);
        ent.spawn_position = ent.position;
        ent.scale = Vec3(1.5f, 1.5f, 1.5f);
        ent.health = 100.0f;
        ent.max_health = 100.0f;
        ent.local_bounds = qe::core::AABB::from_center(Vec3::zero(), 0.5f);
        ent.respawn_delay = 5.0f;
        app.entities.push_back(ent);
    }

    std::cout << "Entities: " << app.entities.size() << " targets spawned" << std::endl;
}

// ── Shooting ────────────────────────────────────────────────────────────────
void shoot(AppState& app) {
    if (app.shoot_cooldown > 0.0f) return;
    app.shoot_cooldown = 0.15f;  // Fire rate
    app.total_shots++;

    qe::math::Vec3 origin = app.camera.position();
    qe::math::Vec3 dir = app.camera.forward();

    // Spawn projectile
    qe::core::Projectile proj;
    proj.position = origin + dir * 0.5f;  // Slightly ahead of camera
    proj.velocity = dir * app.projectile_speed;
    proj.lifetime = 3.0f;
    proj.radius = 0.08f;
    proj.damage = 25.0f;
    app.projectiles.push_back(proj);

    // Also do instant hitscan for immediate feedback
    float closest_t = 999.0f;
    int closest_id = -1;

    for (size_t i = 0; i < app.entities.size(); ++i) {
        auto& ent = app.entities[i];
        if (!ent.alive) continue;

        qe::core::AABB wb = ent.world_bounds();
        float t = 0;
        if (wb.ray_intersect(origin, dir, t)) {
            if (t < closest_t) {
                closest_t = t;
                closest_id = static_cast<int>(i);
            }
        }
    }

    if (closest_id >= 0) {
        auto& ent = app.entities[closest_id];
        bool killed = ent.take_damage(proj.damage);
        app.total_hits++;
        if (killed) {
            app.score += 100;
            std::cout << "KILL! Score: " << app.score << std::endl;
        }
    }
}

// ── Projectile Update & Collision ───────────────────────────────────────────
void update_projectiles(AppState& app, float dt) {
    for (auto& p : app.projectiles) {
        p.update(dt);
    }
    // Remove dead projectiles (keep vector compact)
    app.projectiles.erase(
        std::remove_if(app.projectiles.begin(), app.projectiles.end(),
                        [](const qe::core::Projectile& p) { return !p.is_alive(); }),
        app.projectiles.end());
}

void check_collisions(AppState& app) {
    for (auto& proj : app.projectiles) {
        if (!proj.active) continue;
        qe::core::AABB pb = proj.bounds();

        for (auto& ent : app.entities) {
            if (!ent.alive) continue;
            qe::core::AABB eb = ent.world_bounds();

            if (pb.intersects(eb)) {
                bool killed = ent.take_damage(proj.damage);
                proj.active = false;
                app.total_hits++;
                if (killed) {
                    app.score += 100;
                }
                break;
            }
        }
    }
}

// ── Events ──────────────────────────────────────────────────────────────────
void process_events(AppState& app) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT: app.running = false; break;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE: app.running = false; break;
                    case SDLK_TAB:
                        app.camera.toggle_mode();
                        std::cout << "Camera: "
                            << (app.camera.mode()==qe::renderer::CameraMode::FirstPerson
                                ? "FPS" : "TPS") << std::endl;
                        break;
                    case SDLK_f:
                        app.wireframe = !app.wireframe;
                        qe::renderer::gl::glPolygonMode(
                            GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
                        break;
                    case SDLK_r:
                        spawn_entities(app);
                        app.score = 0;
                        app.total_shots = 0;
                        app.total_hits = 0;
                        std::cout << "Entities reset!" << std::endl;
                        break;
                    case SDLK_1:
                        app.camera.config.smoothing = 0;
                        app.slerp_on = false;
                        break;
                    case SDLK_2:
                        app.camera.config.smoothing = 0.85f;
                        app.slerp_on = true;
                        break;
                    default: break;
                }
                break;
            case SDL_MOUSEMOTION:
                app.camera.process_mouse(
                    static_cast<float>(ev.motion.xrel),
                    static_cast<float>(ev.motion.yrel));
                break;
            case SDL_MOUSEWHEEL:
                app.camera.process_scroll(static_cast<float>(ev.wheel.y));
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    shoot(app);
                }
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    qe::renderer::gl::glViewport(0, 0, ev.window.data1, ev.window.data2);
                    app.camera.config.aspect =
                        static_cast<float>(ev.window.data1) /
                        static_cast<float>(ev.window.data2);
                }
                break;
            default: break;
        }
    }

    // Continuous fire if holding left mouse
    if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        shoot(app);
    }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(AppState& app, float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float fw=0, rt=0, up=0;
    bool sprint = false;
    if (keys[SDL_SCANCODE_W]) fw += 1;
    if (keys[SDL_SCANCODE_S]) fw -= 1;
    if (keys[SDL_SCANCODE_D]) rt += 1;
    if (keys[SDL_SCANCODE_A]) rt -= 1;
    if (keys[SDL_SCANCODE_SPACE]) up += 1;
    if (keys[SDL_SCANCODE_C])     up -= 1;
    if (keys[SDL_SCANCODE_LSHIFT]) sprint = true;

    app.camera.process_movement(fw, rt, up, sprint, dt);
    app.camera.update(dt);

    app.shoot_cooldown -= dt;
    if (app.shoot_cooldown < 0) app.shoot_cooldown = 0;

    update_projectiles(app, dt);
    check_collisions(app);

    for (auto& ent : app.entities) {
        ent.update(dt);
    }

    app.time += dt;
}

// ── Render ──────────────────────────────────────────────────────────────────
void render(AppState& app) {
    using namespace qe::renderer::gl;
    using namespace qe::math;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    app.shader.use();

    Mat4 vp = app.camera.vp_matrix();
    app.shader.set_mat4("uViewProjection", vp);
    app.shader.set_vec3("uLightDir", Vec3(0.3f, 0.8f, 0.5f).normalized());
    app.shader.set_vec3("uLightColor", Vec3(1.0f, 0.95f, 0.9f));
    app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    app.shader.set_vec3("uCameraPos", app.camera.position());
    app.shader.set_int("uTexture0", 0);

    // --- Decorations ---
    for (const auto& obj : app.decorations) {
        app.shader.set_mat4("uModel", obj.model_matrix(app.time));
        if (obj.texture_id >= 0 && obj.texture_id < static_cast<int>(app.textures.size())) {
            app.textures[obj.texture_id]->bind(0);
            app.shader.set_int("uUseTexture", 1);
        } else {
            app.shader.set_int("uUseTexture", 0);
        }
        switch (obj.mesh_type) {
            case SceneObject::MeshType::Floor:
                glDisable(GL_CULL_FACE); app.floor_plane.draw(); glEnable(GL_CULL_FACE); break;
            case SceneObject::MeshType::Sphere: app.sphere.draw(); break;
            default: app.cube.draw(); break;
        }
    }

    // --- Entities (targets) ---
    app.shader.set_int("uUseTexture", 0);
    for (const auto& ent : app.entities) {
        if (!ent.alive) {
            // Death animation: shrink and spin
            if (ent.death_timer < 1.0f) {
                float t = ent.death_timer;
                float shrink = 1.0f - t;
                Quaternion spin = Quaternion::from_axis_angle(Vec3::up(), t * 10.0f);
                Mat4 model = Mat4::trs(ent.position + Vec3(0, t*2, 0),
                                        spin, ent.scale * shrink);
                app.shader.set_mat4("uModel", model);
                app.shader.set_vec3("uAmbient", Vec3(0.5f, 0.1f, 0.1f));  // Red glow
                app.sphere.draw();
                app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
            }
            continue;
        }

        Mat4 model = Mat4::trs(ent.position, ent.rotation, ent.scale);
        app.shader.set_mat4("uModel", model);

        // Hit flash: override ambient to white briefly
        if (ent.hit_flash > 0.0f) {
            float flash = ent.hit_flash / 0.3f;
            app.shader.set_vec3("uAmbient", Vec3(flash, flash, flash));
        }

        // Color based on health
        float hp = ent.health_fraction();
        Vec3 color(1 - hp, hp, 0.2f);  // Red → Green gradient
        app.shader.set_vec3("uLightColor", Vec3(1, 0.95f, 0.9f));

        app.cube.draw();

        // Reset ambient
        if (ent.hit_flash > 0.0f) {
            app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
        }
    }

    // --- Projectiles ---
    app.shader.set_int("uUseTexture", 0);
    for (const auto& p : app.projectiles) {
        float glow = p.brightness;
        app.shader.set_vec3("uAmbient", Vec3(glow, glow * 0.8f, glow * 0.3f));
        Mat4 model = Mat4::trs(p.position, Quaternion::identity(),
                                Vec3(0.1f, 0.1f, 0.1f));
        app.shader.set_mat4("uModel", model);
        app.cube.draw();
    }
    app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));

    // --- Grid ---
    glDisable(GL_CULL_FACE);
    app.shader.set_int("uUseTexture", 0);
    app.shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);

    // --- Crosshair (2D overlay) ---
    render_crosshair(app);
}

void render_crosshair(AppState& app) {
    using namespace qe::renderer::gl;

    glDisable(GL_DEPTH_TEST);
    app.crosshair_shader.use();
    glLineWidth(2.0f);
    glBindVertexArray(app.crosshair_mesh.vao);
    glDrawElements(GL_LINES, app.crosshair_mesh.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

// ── Title ───────────────────────────────────────────────────────────────────
void update_title(AppState& app) {
    const char* mode =
        (app.camera.mode()==qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";
    float accuracy = app.total_shots > 0
        ? (static_cast<float>(app.total_hits) / app.total_shots * 100.0f) : 0;

    int alive_count = 0;
    for (const auto& e : app.entities) if (e.alive) alive_count++;

    std::ostringstream t;
    t << "QuatEngine | " << static_cast<int>(app.current_fps) << " FPS"
      << " | " << mode
      << " | Score:" << app.score
      << " | Acc:" << static_cast<int>(accuracy) << "%"
      << " | Targets:" << alive_count << "/" << app.entities.size();
    SDL_SetWindowTitle(app.window, t.str().c_str());
}

// ── Cleanup ─────────────────────────────────────────────────────────────────
void cleanup(AppState& app) {
    app.cube.destroy();
    app.sphere.destroy();
    app.floor_plane.destroy();
    app.grid.destroy();
    app.crosshair_mesh.destroy();
    app.shader.destroy();
    app.crosshair_shader.destroy();
    app.tex_checker.destroy();
    app.tex_bricks.destroy();
    app.tex_floor.destroy();
    app.tex_white.destroy();
    if (app.gl_context) SDL_GL_DeleteContext(app.gl_context);
    if (app.window) SDL_DestroyWindow(app.window);
    SDL_Quit();
}
