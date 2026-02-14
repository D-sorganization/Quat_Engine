/**
 * @file main.cpp
 * @brief QuatEngine Phase 3 Demo — FPS/TPS Camera, Sprint, Scene Objects.
 *
 * Phase 3 adds:
 *   - Third-person orbit camera (SLERP-smoothed)
 *   - Camera mode switching (Tab key)
 *   - Sprint (Left Shift) with head bob
 *   - Smooth acceleration / deceleration
 *   - Scene with multiple objects (pillars, platforms, rotating sculptures)
 *   - Scroll wheel zoom (TPS mode)
 *
 * Controls:
 *   WASD           - Move
 *   Mouse          - Look around
 *   Left Shift     - Sprint
 *   Space / C      - Up / Down
 *   Tab            - Toggle FPS / TPS camera
 *   Scroll Wheel   - Zoom in/out (TPS mode)
 *   1 / 2          - SLERP smoothing off / on
 *   F              - Toggle wireframe
 *   Escape         - Quit
 */

#include "core/Transform.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec3.h"
#include "renderer/Camera.h"
#include "renderer/GLLoader.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"

#include <SDL.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ── Constants ───────────────────────────────────────────────────────────────
constexpr int   WINDOW_WIDTH  = 1280;
constexpr int   WINDOW_HEIGHT = 720;
constexpr float PI = 3.14159265358979f;

// ── Scene Object ────────────────────────────────────────────────────────────
struct SceneObject {
    qe::math::Vec3       position;
    qe::math::Quaternion rotation;
    qe::math::Vec3       scale;

    // Animation
    enum class Anim { None, SpinY, SpinTilted, Orbit, Breathe, SlerpDemo };
    Anim anim = Anim::None;
    float anim_speed = 1.0f;
    float anim_phase = 0.0f;  // Random phase offset

    qe::math::Mat4 model_matrix(float time) const {
        qe::math::Quaternion rot = rotation;
        qe::math::Vec3 pos = position;
        qe::math::Vec3 s = scale;

        switch (anim) {
            case Anim::SpinY:
                rot = qe::math::Quaternion::from_axis_angle(
                    qe::math::Vec3::up(), time * anim_speed + anim_phase) * rot;
                break;

            case Anim::SpinTilted:
                rot = qe::math::Quaternion::from_axis_angle(
                    qe::math::Vec3(0.0f, 1.0f, 0.3f).normalized(),
                    time * anim_speed + anim_phase) * rot;
                break;

            case Anim::Orbit: {
                float angle = time * anim_speed + anim_phase;
                float radius = 3.0f;
                pos.x += std::cos(angle) * radius;
                pos.z += std::sin(angle) * radius;
                // Face direction of movement
                rot = qe::math::Quaternion::from_axis_angle(
                    qe::math::Vec3::up(), -angle + PI * 0.5f);
                break;
            }

            case Anim::Breathe: {
                float breath = 1.0f + 0.15f * std::sin(time * anim_speed + anim_phase);
                s = s * breath;
                break;
            }

            case Anim::SlerpDemo: {
                float t = (std::sin(time * anim_speed + anim_phase) + 1.0f) * 0.5f;
                qe::math::Quaternion a = qe::math::Quaternion::from_axis_angle(
                    qe::math::Vec3::up(), 0.0f);
                qe::math::Quaternion b = qe::math::Quaternion::from_axis_angle(
                    qe::math::Vec3(1, 1, 0).normalized(), PI);
                rot = qe::math::Quaternion::slerp(a, b, t);
                break;
            }

            case Anim::None:
            default:
                break;
        }

        return qe::math::Mat4::trs(pos, rot, s);
    }
};

// ── Application State ───────────────────────────────────────────────────────
struct AppState {
    SDL_Window*   window     = nullptr;
    SDL_GLContext gl_context = nullptr;
    bool running    = true;
    bool wireframe  = false;
    bool slerp_on   = true;

    qe::renderer::Camera camera;
    qe::renderer::Shader shader;
    qe::renderer::Mesh   cube;
    qe::renderer::Mesh   grid;

    std::vector<SceneObject> objects;

    // Timing
    float  time       = 0.0f;
    Uint64 last_time  = 0;
    int    frame_count = 0;
    float  fps_timer   = 0.0f;
    float  current_fps = 0.0f;
};

// ── Forward Declarations ────────────────────────────────────────────────────
bool init_sdl(AppState& app);
bool init_opengl(AppState& app);
void build_scene(AppState& app);
void process_events(AppState& app);
void update(AppState& app, float dt);
void render(AppState& app);
void cleanup(AppState& app);
void update_title(AppState& app);

// ── Entry Point ─────────────────────────────────────────────────────────────
int main(int /*argc*/, char* /*argv*/[]) {
    AppState app;

    if (!init_sdl(app))    return 1;
    if (!init_opengl(app)) return 1;

    // Camera setup
    qe::renderer::Camera::Config cam_config;
    cam_config.aspect      = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    cam_config.smoothing   = 0.85f;
    cam_config.move_speed  = 5.0f;
    cam_config.sprint_mult = 2.5f;
    app.camera = qe::renderer::Camera(cam_config);
    app.camera.set_position(qe::math::Vec3(0.0f, 1.5f, 8.0f));

    // Compile shaders
    if (!app.shader.load_from_files("shaders/basic.vert", "shaders/basic.frag")) {
        std::cerr << "Failed to compile shaders" << std::endl;
        cleanup(app);
        return 1;
    }

    // Create geometry & scene
    app.cube = qe::renderer::Mesh::create_cube();
    app.grid = qe::renderer::Mesh::create_grid(20, 1.0f);
    build_scene(app);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "QuatEngine Phase 3 Demo" << std::endl;
    std::cout << "  WASD            - Move" << std::endl;
    std::cout << "  Mouse           - Look (quaternion!)" << std::endl;
    std::cout << "  Left Shift      - Sprint" << std::endl;
    std::cout << "  Space / C       - Up / Down" << std::endl;
    std::cout << "  Tab             - Toggle FPS / TPS camera" << std::endl;
    std::cout << "  Scroll Wheel    - Zoom (TPS mode)" << std::endl;
    std::cout << "  1 / 2           - SLERP off / on" << std::endl;
    std::cout << "  F               - Toggle wireframe" << std::endl;
    std::cout << "  Escape          - Quit" << std::endl;

    // ── Main Loop ───────────────────────────────────────────────────────
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

        // FPS counter
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

// ── SDL Initialization ──────────────────────────────────────────────────────
bool init_sdl(AppState& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    app.window = SDL_CreateWindow(
        "QuatEngine — Phase 3",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!app.window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    app.gl_context = SDL_GL_CreateContext(app.window);
    if (!app.gl_context) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    return true;
}

// ── OpenGL Initialization ───────────────────────────────────────────────────
bool init_opengl(AppState& /*app*/) {
    if (!qe::renderer::gl::load()) {
        std::cerr << "Failed to load OpenGL functions" << std::endl;
        return false;
    }

    const char* renderer = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_VERSION));
    std::cout << "GPU: " << (renderer ? renderer : "unknown") << std::endl;
    std::cout << "OpenGL: " << (version ? version : "unknown") << std::endl;

    using namespace qe::renderer::gl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.03f, 0.03f, 0.08f, 1.0f);

    return true;
}

// ── Scene Construction ──────────────────────────────────────────────────────
void build_scene(AppState& app) {
    using namespace qe::math;

    // --- Center sculpture: spinning tilted cube ---
    app.objects.push_back({
        Vec3(0.0f, 1.5f, 0.0f),
        Quaternion::identity(),
        Vec3(1.5f, 1.5f, 1.5f),
        SceneObject::Anim::SpinTilted, 0.8f, 0.0f
    });

    // --- SLERP showcase cube ---
    app.objects.push_back({
        Vec3(5.0f, 1.0f, -3.0f),
        Quaternion::identity(),
        Vec3(1.0f, 1.0f, 1.0f),
        SceneObject::Anim::SlerpDemo, 0.5f, 0.0f
    });

    // --- Pillars around the center (tall, static) ---
    constexpr int PILLAR_COUNT = 8;
    constexpr float PILLAR_RADIUS = 12.0f;
    for (int i = 0; i < PILLAR_COUNT; ++i) {
        float angle = (2.0f * PI * i) / PILLAR_COUNT;
        float x = std::cos(angle) * PILLAR_RADIUS;
        float z = std::sin(angle) * PILLAR_RADIUS;
        float height = 2.0f + (i % 3) * 1.5f;

        app.objects.push_back({
            Vec3(x, height * 0.5f, z),
            Quaternion::from_axis_angle(Vec3::up(), angle),
            Vec3(0.8f, height, 0.8f),
            SceneObject::Anim::None, 0.0f, 0.0f
        });
    }

    // --- Orbiting cube ---
    app.objects.push_back({
        Vec3(0.0f, 2.5f, 0.0f),
        Quaternion::identity(),
        Vec3(0.5f, 0.5f, 0.5f),
        SceneObject::Anim::Orbit, 1.2f, 0.0f
    });

    // --- Breathing cube ---
    app.objects.push_back({
        Vec3(-5.0f, 1.0f, -3.0f),
        Quaternion::from_axis_angle(Vec3::up(), PI * 0.25f),
        Vec3(1.2f, 1.2f, 1.2f),
        SceneObject::Anim::Breathe, 2.0f, 0.0f
    });

    // --- Scattered small cubes (debris field) ---
    for (int i = 0; i < 20; ++i) {
        float x = (i * 7.13f);
        x = std::fmod(x, 30.0f) - 15.0f;
        float z = (i * 11.37f);
        z = std::fmod(z, 30.0f) - 15.0f;
        float sz = 0.2f + std::fmod(i * 3.7f, 0.4f);

        app.objects.push_back({
            Vec3(x, sz * 0.5f, z),
            Quaternion::from_axis_angle(
                Vec3(1, 0.5f, 0.3f).normalized(),
                i * 0.7f),
            Vec3(sz, sz, sz),
            (i % 4 == 0) ? SceneObject::Anim::SpinY : SceneObject::Anim::None,
            0.5f + i * 0.1f,
            i * 1.23f
        });
    }

    // --- Floating platforms at different heights ---
    for (int i = 0; i < 5; ++i) {
        float angle = (2.0f * PI * i) / 5.0f + 0.3f;
        float r = 7.0f + i * 0.5f;
        app.objects.push_back({
            Vec3(std::cos(angle) * r, 0.1f + i * 0.8f, std::sin(angle) * r),
            Quaternion::from_axis_angle(Vec3::up(), angle),
            Vec3(2.0f, 0.15f, 2.0f),
            SceneObject::Anim::None, 0.0f, 0.0f
        });
    }

    std::cout << "Scene: " << app.objects.size() << " objects" << std::endl;
}

// ── Event Processing ────────────────────────────────────────────────────────
void process_events(AppState& app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app.running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        app.running = false;
                        break;
                    case SDLK_TAB:
                        app.camera.toggle_mode();
                        std::cout << "Camera: "
                                  << (app.camera.mode() == qe::renderer::CameraMode::FirstPerson
                                          ? "First Person" : "Third Person")
                                  << std::endl;
                        break;
                    case SDLK_f:
                        app.wireframe = !app.wireframe;
                        qe::renderer::gl::glPolygonMode(
                            GL_FRONT_AND_BACK,
                            app.wireframe ? GL_LINE : GL_FILL);
                        break;
                    case SDLK_1: {
                        app.camera.config.smoothing = 0.0f;
                        app.slerp_on = false;
                        std::cout << "SLERP smoothing: OFF" << std::endl;
                        break;
                    }
                    case SDLK_2: {
                        app.camera.config.smoothing = 0.85f;
                        app.slerp_on = true;
                        std::cout << "SLERP smoothing: ON (0.85)" << std::endl;
                        break;
                    }
                    default:
                        break;
                }
                break;

            case SDL_MOUSEMOTION:
                app.camera.process_mouse(
                    static_cast<float>(event.motion.xrel),
                    static_cast<float>(event.motion.yrel));
                break;

            case SDL_MOUSEWHEEL:
                app.camera.process_scroll(static_cast<float>(event.wheel.y));
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int w = event.window.data1;
                    int h = event.window.data2;
                    qe::renderer::gl::glViewport(0, 0, w, h);
                    app.camera.config.aspect =
                        static_cast<float>(w) / static_cast<float>(h);
                }
                break;

            default:
                break;
        }
    }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(AppState& app, float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float forward = 0.0f, right_mv = 0.0f, up = 0.0f;
    bool sprinting = false;

    if (keys[SDL_SCANCODE_W]) forward  += 1.0f;
    if (keys[SDL_SCANCODE_S]) forward  -= 1.0f;
    if (keys[SDL_SCANCODE_D]) right_mv += 1.0f;
    if (keys[SDL_SCANCODE_A]) right_mv -= 1.0f;
    if (keys[SDL_SCANCODE_SPACE]) up    += 1.0f;
    if (keys[SDL_SCANCODE_C])     up    -= 1.0f;
    if (keys[SDL_SCANCODE_LSHIFT]) sprinting = true;

    app.camera.process_movement(forward, right_mv, up, sprinting, dt);
    app.camera.update(dt);

    app.time += dt;
}

// ── Render ──────────────────────────────────────────────────────────────────
void render(AppState& app) {
    using namespace qe::renderer::gl;
    using namespace qe::math;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app.shader.use();

    // Camera matrices
    Mat4 vp = app.camera.vp_matrix();
    app.shader.set_mat4("uViewProjection", vp);

    // Lighting
    Vec3 light_dir = Vec3(0.3f, 0.8f, 0.5f).normalized();
    app.shader.set_vec3("uLightDir", light_dir);
    app.shader.set_vec3("uLightColor", Vec3(1.0f, 0.95f, 0.9f));
    app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    app.shader.set_vec3("uCameraPos", app.camera.position());

    // --- Draw scene objects ---
    for (const auto& obj : app.objects) {
        Mat4 model = obj.model_matrix(app.time);
        app.shader.set_mat4("uModel", model);
        app.cube.draw();
    }

    // --- Draw TPS target indicator (small cube at target pos) ---
    if (app.camera.mode() == qe::renderer::CameraMode::ThirdPerson) {
        Vec3 target = app.camera.tps_target();
        Mat4 target_model = Mat4::trs(
            target,
            Quaternion::from_axis_angle(Vec3::up(), app.time * 2.0f),
            Vec3(0.3f, 0.3f, 0.3f)
        );
        app.shader.set_mat4("uModel", target_model);
        app.cube.draw();
    }

    // --- Draw grid floor ---
    glDisable(GL_CULL_FACE);
    app.shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

// ── Title Bar Update ────────────────────────────────────────────────────────
void update_title(AppState& app) {
    const char* mode_str =
        (app.camera.mode() == qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";

    std::ostringstream title;
    title << "QuatEngine | "
          << static_cast<int>(app.current_fps) << " FPS | "
          << "Mode: " << mode_str << " | "
          << "SLERP: " << (app.slerp_on ? "ON" : "OFF") << " | "
          << "Speed: " << static_cast<int>(app.camera.current_speed()) << " | "
          << "Pos: ("
          << static_cast<int>(app.camera.position().x) << ", "
          << static_cast<int>(app.camera.position().y) << ", "
          << static_cast<int>(app.camera.position().z) << ")";
    SDL_SetWindowTitle(app.window, title.str().c_str());
}

// ── Cleanup ─────────────────────────────────────────────────────────────────
void cleanup(AppState& app) {
    app.cube.destroy();
    app.grid.destroy();
    app.shader.destroy();

    if (app.gl_context) SDL_GL_DeleteContext(app.gl_context);
    if (app.window)     SDL_DestroyWindow(app.window);
    SDL_Quit();
}
