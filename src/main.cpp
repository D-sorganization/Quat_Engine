/**
 * @file main.cpp
 * @brief QuatEngine Phase 2 Demo — SDL2 + OpenGL with Quaternion Camera.
 *
 * Demonstrates:
 *   - SDL2 window with OpenGL 3.3 core context
 *   - Quaternion-based FPS camera (no gimbal lock!)
 *   - SLERP-smoothed camera rotation
 *   - Blinn-Phong lit spinning cube
 *   - Ground grid for spatial reference
 *   - WASD + mouse look controls
 *
 * Controls:
 *   WASD      - Move forward/left/backward/right
 *   Space     - Move up
 *   LShift    - Move down
 *   Mouse     - Look around (quaternion rotation)
 *   1/2       - Toggle SLERP smoothing off/on
 *   F         - Toggle wireframe mode
 *   Escape    - Quit
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
constexpr int WINDOW_WIDTH  = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr float PI = 3.14159265358979f;

// ── Forward Declarations ────────────────────────────────────────────────────
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

    // Cube animation
    float cube_angle = 0.0f;

    // Timing
    Uint64 last_time  = 0;
    int    frame_count = 0;
    float  fps_timer   = 0.0f;
    float  current_fps = 0.0f;
};

bool init_sdl(AppState& app);
bool init_opengl(AppState& app);
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

    // Set up camera with SLERP smoothing
    qe::renderer::Camera::Config cam_config;
    cam_config.aspect    = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    cam_config.smoothing = 0.85f;  // Smooth camera by default
    cam_config.move_speed = 4.0f;
    app.camera = qe::renderer::Camera(cam_config);
    app.camera.set_position(qe::math::Vec3(0.0f, 1.5f, 5.0f));

    // Compile shaders
    if (!app.shader.load_from_files("shaders/basic.vert", "shaders/basic.frag")) {
        std::cerr << "Failed to compile shaders" << std::endl;
        cleanup(app);
        return 1;
    }

    // Create geometry
    app.cube = qe::renderer::Mesh::create_cube();
    app.grid = qe::renderer::Mesh::create_grid(15, 1.0f);

    // Lock mouse to window
    SDL_SetRelativeMouseMode(SDL_TRUE);

    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "QuatEngine Phase 2 Demo" << std::endl;
    std::cout << "  WASD       - Move" << std::endl;
    std::cout << "  Mouse      - Look (quaternion!)" << std::endl;
    std::cout << "  Space/Shift - Up/Down" << std::endl;
    std::cout << "  1/2        - SLERP off/on" << std::endl;
    std::cout << "  F          - Toggle wireframe" << std::endl;
    std::cout << "  Escape     - Quit" << std::endl;

    // ── Main Loop ───────────────────────────────────────────────────────────
    while (app.running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - app.last_time) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        app.last_time = now;

        // Clamp delta time to avoid spiral of death
        if (dt > 0.1f) dt = 0.1f;

        process_events(app);
        update(app, dt);
        render(app);

        SDL_GL_SwapWindow(app.window);

        // FPS counter
        app.frame_count++;
        app.fps_timer += dt;
        if (app.fps_timer >= 1.0f) {
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

    // Request OpenGL 3.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    app.window = SDL_CreateWindow(
        "QuatEngine — Phase 2 Demo",
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

    SDL_GL_SetSwapInterval(1);  // VSync
    return true;
}

// ── OpenGL Initialization ───────────────────────────────────────────────────
bool init_opengl(AppState& app) {
    (void)app;

    if (!qe::renderer::gl::load()) {
        std::cerr << "Failed to load OpenGL functions" << std::endl;
        return false;
    }

    // Print GPU info
    const char* renderer = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_VERSION));
    std::cout << "GPU: " << (renderer ? renderer : "unknown") << std::endl;
    std::cout << "OpenGL: " << (version ? version : "unknown") << std::endl;

    // Set up default state
    using namespace qe::renderer::gl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);  // Dark blue-grey background

    return true;
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
                    case SDLK_f:
                        app.wireframe = !app.wireframe;
                        qe::renderer::gl::glPolygonMode(
                            GL_FRONT_AND_BACK,
                            app.wireframe ? GL_LINE : GL_FILL);
                        break;
                    case SDLK_1: {
                        auto cfg = app.camera.config;
                        cfg.smoothing = 0.0f;
                        app.camera.config = cfg;
                        app.slerp_on = false;
                        std::cout << "SLERP smoothing: OFF (instant)" << std::endl;
                        break;
                    }
                    case SDLK_2: {
                        auto cfg = app.camera.config;
                        cfg.smoothing = 0.85f;
                        app.camera.config = cfg;
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

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int w = event.window.data1;
                    int h = event.window.data2;
                    qe::renderer::gl::glViewport(0, 0, w, h);
                    app.camera.config.aspect = static_cast<float>(w) / static_cast<float>(h);
                }
                break;

            default:
                break;
        }
    }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(AppState& app, float dt) {
    // Keyboard movement
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float forward = 0.0f, right_mv = 0.0f, up = 0.0f;

    if (keys[SDL_SCANCODE_W]) forward += 1.0f;
    if (keys[SDL_SCANCODE_S]) forward -= 1.0f;
    if (keys[SDL_SCANCODE_D]) right_mv += 1.0f;
    if (keys[SDL_SCANCODE_A]) right_mv -= 1.0f;
    if (keys[SDL_SCANCODE_SPACE])  up += 1.0f;
    if (keys[SDL_SCANCODE_LSHIFT]) up -= 1.0f;

    app.camera.process_movement(forward, right_mv, up, dt);
    app.camera.update(dt);

    // Spin the cube
    app.cube_angle += dt * 0.8f;  // Slow rotation
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
    app.shader.set_vec3("uLightDir", Vec3(0.3f, 0.8f, 0.5f).normalized());
    app.shader.set_vec3("uLightColor", Vec3(1.0f, 0.95f, 0.9f));
    app.shader.set_vec3("uAmbient", Vec3(0.15f, 0.15f, 0.2f));
    app.shader.set_vec3("uCameraPos", app.camera.position());

    // --- Draw spinning cube ---
    // Create cube rotation using quaternion (the whole point!)
    Quaternion cube_rot = Quaternion::from_axis_angle(
        Vec3(0.0f, 1.0f, 0.3f).normalized(), app.cube_angle);
    Mat4 cube_model = Mat4::trs(
        Vec3(0.0f, 1.0f, 0.0f),  // Floating above ground
        cube_rot,
        Vec3(1.5f, 1.5f, 1.5f)   // Slightly larger
    );
    app.shader.set_mat4("uModel", cube_model);
    app.cube.draw();

    // --- Draw a second cube using SLERP-interpolated rotation ---
    float t = (std::sin(app.cube_angle * 0.5f) + 1.0f) * 0.5f;  // Oscillate 0→1
    Quaternion rot_a = Quaternion::from_axis_angle(Vec3::up(), 0.0f);
    Quaternion rot_b = Quaternion::from_axis_angle(Vec3::right(), PI);
    Quaternion slerped = Quaternion::slerp(rot_a, rot_b, t);

    Mat4 cube2_model = Mat4::trs(
        Vec3(4.0f, 1.0f, -2.0f),
        slerped,
        Vec3::one()
    );
    app.shader.set_mat4("uModel", cube2_model);
    app.cube.draw();

    // --- Draw a third cube with NLERP for comparison ---
    Quaternion nlerped = Quaternion::nlerp(rot_a, rot_b, t);
    Mat4 cube3_model = Mat4::trs(
        Vec3(-4.0f, 1.0f, -2.0f),
        nlerped,
        Vec3::one()
    );
    app.shader.set_mat4("uModel", cube3_model);
    app.cube.draw();

    // --- Draw grid floor ---
    glDisable(GL_CULL_FACE);  // Grid is one-sided
    app.shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

// ── Title Bar Update ────────────────────────────────────────────────────────
void update_title(AppState& app) {
    std::ostringstream title;
    title << "QuatEngine | "
          << static_cast<int>(app.current_fps) << " FPS | "
          << "SLERP: " << (app.slerp_on ? "ON" : "OFF") << " | "
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
