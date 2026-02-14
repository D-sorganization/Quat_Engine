/**
 * @file main.cpp
 * @brief QuatEngine Phase 4 Demo — Textures, OBJ Loading, Spheres, Floor.
 *
 * Phase 4 adds:
 *   - Procedural textures (checkerboard, bricks, metal floor)
 *   - OBJ mesh file loading and generation
 *   - Textured floor plane with tiling
 *   - Icosphere primitive
 *   - All Phase 3 features (FPS/TPS camera, sprint, etc.)
 *
 * Controls:
 *   WASD / Mouse / Shift / Space / C — Movement
 *   Tab          — Toggle FPS / TPS camera
 *   Scroll       — Zoom (TPS mode)
 *   1 / 2        — SLERP off / on
 *   F            — Toggle wireframe
 *   Escape       — Quit
 */

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

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ── Constants ───────────────────────────────────────────────────────────────
constexpr int   WINDOW_WIDTH  = 1280;
constexpr int   WINDOW_HEIGHT = 720;
constexpr float PI = 3.14159265358979f;

// ── Scene Object (with optional texture) ────────────────────────────────────
struct SceneObject {
    qe::math::Vec3       position;
    qe::math::Quaternion rotation;
    qe::math::Vec3       scale;

    enum class Anim { None, SpinY, SpinTilted, Orbit, Breathe, SlerpDemo };
    Anim anim = Anim::None;
    float anim_speed = 1.0f;
    float anim_phase = 0.0f;

    // Rendering
    enum class MeshType { Cube, Sphere, Floor, OBJ };
    MeshType mesh_type = MeshType::Cube;
    int texture_id = -1;   // Index into texture array, -1 = no texture

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
            default: break;
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

    // Meshes
    qe::renderer::Mesh cube;
    qe::renderer::Mesh sphere;
    qe::renderer::Mesh floor_plane;
    qe::renderer::Mesh grid;
    qe::renderer::Mesh obj_mesh;  // Loaded from .obj file

    // Textures
    qe::renderer::Texture tex_checker;
    qe::renderer::Texture tex_bricks;
    qe::renderer::Texture tex_floor;
    qe::renderer::Texture tex_white;  // No-op texture
    std::vector<qe::renderer::Texture*> textures;  // Indexed by SceneObject::texture_id

    std::vector<SceneObject> objects;

    // Timing
    float  time       = 0.0f;
    Uint64 last_time  = 0;
    int    frame_count = 0;
    float  fps_timer   = 0.0f;
    float  current_fps = 0.0f;
};

bool init_sdl(AppState& app);
bool init_opengl(AppState& app);
void create_textures(AppState& app);
void generate_sample_obj(AppState& app);
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

    // Camera
    qe::renderer::Camera::Config cam_config;
    cam_config.aspect      = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    cam_config.smoothing   = 0.85f;
    cam_config.move_speed  = 5.0f;
    cam_config.sprint_mult = 2.5f;
    app.camera = qe::renderer::Camera(cam_config);
    app.camera.set_position(qe::math::Vec3(0.0f, 1.5f, 10.0f));

    // Shaders
    if (!app.shader.load_from_files("shaders/basic.vert", "shaders/basic.frag")) {
        std::cerr << "Failed to compile shaders" << std::endl;
        cleanup(app);
        return 1;
    }

    // Textures
    create_textures(app);

    // Meshes
    app.cube = qe::renderer::Mesh::create_cube();
    app.sphere = qe::renderer::Mesh::create_sphere(3, 0.5f, 0.8f, 0.6f, 0.3f);
    app.floor_plane = qe::renderer::Mesh::create_floor_plane(25.0f, 8.0f);
    app.grid = qe::renderer::Mesh::create_grid(25, 1.0f);

    // Generate and load an OBJ model
    generate_sample_obj(app);

    // Build scene
    build_scene(app);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "\nQuatEngine Phase 4 Demo" << std::endl;
    std::cout << "  WASD / Mouse    - Move & Look" << std::endl;
    std::cout << "  Shift           - Sprint" << std::endl;
    std::cout << "  Space / C       - Up / Down" << std::endl;
    std::cout << "  Tab             - Toggle FPS / TPS" << std::endl;
    std::cout << "  Scroll          - Zoom (TPS)" << std::endl;
    std::cout << "  1 / 2           - SLERP off / on" << std::endl;
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

// ── SDL / OpenGL Init ───────────────────────────────────────────────────────
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
        "QuatEngine — Phase 4",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!app.window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    app.gl_context = SDL_GL_CreateContext(app.window);
    if (!app.gl_context) {
        std::cerr << "GL context failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    return true;
}

bool init_opengl(AppState& /*app*/) {
    if (!qe::renderer::gl::load()) {
        std::cerr << "Failed to load GL functions" << std::endl;
        return false;
    }

    const char* renderer = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(
        qe::renderer::gl::glGetString(GL_VERSION));
    std::cout << "GPU: " << (renderer ? renderer : "?") << std::endl;
    std::cout << "GL:  " << (version ? version : "?") << std::endl;

    using namespace qe::renderer::gl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.03f, 0.03f, 0.08f, 1.0f);

    return true;
}

// ── Texture Creation ────────────────────────────────────────────────────────
void create_textures(AppState& app) {
    app.tex_checker = qe::renderer::Texture::create_checkerboard(
        256, 8, 220, 220, 230, 50, 50, 60);
    app.tex_bricks = qe::renderer::Texture::create_bricks(256);
    app.tex_floor = qe::renderer::Texture::create_floor(256);
    app.tex_white = qe::renderer::Texture::create_solid(255, 255, 255);

    // Build indexed texture list
    app.textures.push_back(&app.tex_checker);   // 0
    app.textures.push_back(&app.tex_bricks);    // 1
    app.textures.push_back(&app.tex_floor);     // 2
    app.textures.push_back(&app.tex_white);     // 3

    std::cout << "Textures: " << app.textures.size() << " created" << std::endl;
}

// ── Sample OBJ Model ───────────────────────────────────────────────────────
void generate_sample_obj(AppState& app) {
    // Generate a pyramid OBJ file
    std::vector<qe::renderer::Vertex> verts = {
        // Base (Y=0)
        {{-1, 0, -1}, {0,-1,0}, {0.7f,0.3f,0.2f}, {0,0}},
        {{ 1, 0, -1}, {0,-1,0}, {0.7f,0.3f,0.2f}, {1,0}},
        {{ 1, 0,  1}, {0,-1,0}, {0.8f,0.4f,0.3f}, {1,1}},
        {{-1, 0,  1}, {0,-1,0}, {0.8f,0.4f,0.3f}, {0,1}},
        // Apex
        {{ 0, 1.8f, 0}, {0,1,0}, {1.0f,0.8f,0.2f}, {0.5f,0.5f}},
    };

    std::vector<unsigned int> idxs = {
        // Base
        0,2,1, 0,3,2,
        // Front face
        0,1,4,
        // Right face
        1,2,4,
        // Back face
        2,3,4,
        // Left face
        3,0,4,
    };

    // Try to write and load it
    std::string obj_path = "assets/pyramid.obj";
    SDL_RWops* test = SDL_RWFromFile("assets/", "r");
    if (!test) {
        // Create assets directory
        #ifdef _WIN32
        system("mkdir assets 2>nul");
        #else
        system("mkdir -p assets");
        #endif
    } else {
        SDL_RWclose(test);
    }

    if (qe::renderer::OBJLoader::write_obj(obj_path, verts, idxs)) {
        app.obj_mesh = qe::renderer::OBJLoader::load(obj_path, 0.9f, 0.7f, 0.3f);
        std::cout << "Generated and loaded: " << obj_path << std::endl;
    } else {
        std::cerr << "Failed to write OBJ file" << std::endl;
        app.obj_mesh = qe::renderer::Mesh::create_cube();  // Fallback
    }
}

// ── Scene Construction ──────────────────────────────────────────────────────
void build_scene(AppState& app) {
    using namespace qe::math;

    // --- Textured floor ---
    app.objects.push_back({
        Vec3(0, -0.01f, 0), Quaternion::identity(), Vec3::one(),
        SceneObject::Anim::None, 0, 0,
        SceneObject::MeshType::Floor, 2  // Floor texture
    });

    // --- Center sculpture: spinning tilted cube ---
    app.objects.push_back({
        Vec3(0, 1.5f, 0), Quaternion::identity(), Vec3(1.5f, 1.5f, 1.5f),
        SceneObject::Anim::SpinTilted, 0.8f, 0, SceneObject::MeshType::Cube, 0
    });

    // --- SLERP demo cube (checkerboard textured) ---
    app.objects.push_back({
        Vec3(5, 1, -3), Quaternion::identity(), Vec3::one(),
        SceneObject::Anim::SlerpDemo, 0.5f, 0, SceneObject::MeshType::Cube, 0
    });

    // --- Spheres at various heights ---
    app.objects.push_back({
        Vec3(-5, 1.5f, -3), Quaternion::identity(), Vec3(2, 2, 2),
        SceneObject::Anim::Breathe, 1.5f, 0, SceneObject::MeshType::Sphere, -1
    });

    app.objects.push_back({
        Vec3(-3, 0.8f, 5), Quaternion::identity(), Vec3(1.5f, 1.5f, 1.5f),
        SceneObject::Anim::SpinY, 0.3f, 0, SceneObject::MeshType::Sphere, 0
    });

    // --- Brick-textured pillars ---
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
            SceneObject::Anim::None, 0, 0,
            SceneObject::MeshType::Cube, 1  // Brick texture
        });
    }

    // --- Orbiting sphere ---
    app.objects.push_back({
        Vec3(0, 2.5f, 0), Quaternion::identity(), Vec3(0.6f, 0.6f, 0.6f),
        SceneObject::Anim::Orbit, 1.2f, 0, SceneObject::MeshType::Sphere, -1
    });

    // --- Pyramid (loaded from OBJ) ---
    app.objects.push_back({
        Vec3(7, 0, 0), Quaternion::identity(), Vec3(1.5f, 1.5f, 1.5f),
        SceneObject::Anim::SpinY, 0.4f, 0, SceneObject::MeshType::OBJ, -1
    });

    app.objects.push_back({
        Vec3(-7, 0, 0),
        Quaternion::from_axis_angle(Vec3::up(), PI * 0.5f),
        Vec3(2, 2, 2),
        SceneObject::Anim::None, 0, 0, SceneObject::MeshType::OBJ, 0
    });

    // --- Scattered small objects ---
    for (int i = 0; i < 15; ++i) {
        float x = std::fmod(i * 7.13f, 30.0f) - 15.0f;
        float z = std::fmod(i * 11.37f, 30.0f) - 15.0f;
        float sz = 0.2f + std::fmod(i * 3.7f, 0.4f);

        SceneObject::MeshType type = (i % 3 == 0)
            ? SceneObject::MeshType::Sphere : SceneObject::MeshType::Cube;
        int tex = (i % 2 == 0) ? 0 : -1;

        app.objects.push_back({
            Vec3(x, sz * 0.5f, z),
            Quaternion::from_axis_angle(Vec3(1,0.5f,0.3f).normalized(), i * 0.7f),
            Vec3(sz, sz, sz),
            (i % 4 == 0) ? SceneObject::Anim::SpinY : SceneObject::Anim::None,
            0.5f + i * 0.1f, i * 1.23f,
            type, tex
        });
    }

    // --- Floating platforms ---
    for (int i = 0; i < 5; ++i) {
        float angle = (2.0f * PI * i) / 5.0f + 0.3f;
        float r = 7.0f + i * 0.5f;
        app.objects.push_back({
            Vec3(std::cos(angle)*r, 0.1f + i*0.8f, std::sin(angle)*r),
            Quaternion::from_axis_angle(Vec3::up(), angle),
            Vec3(2, 0.15f, 2),
            SceneObject::Anim::None, 0, 0,
            SceneObject::MeshType::Cube, 2  // Floor texture on platforms
        });
    }

    std::cout << "Scene: " << app.objects.size() << " objects" << std::endl;
}

// ── Events ──────────────────────────────────────────────────────────────────
void process_events(AppState& app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: app.running = false; break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: app.running = false; break;
                    case SDLK_TAB:
                        app.camera.toggle_mode();
                        std::cout << "Camera: "
                            << (app.camera.mode() == qe::renderer::CameraMode::FirstPerson
                                ? "FPS" : "TPS") << std::endl;
                        break;
                    case SDLK_f:
                        app.wireframe = !app.wireframe;
                        qe::renderer::gl::glPolygonMode(
                            GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
                        break;
                    case SDLK_1:
                        app.camera.config.smoothing = 0.0f;
                        app.slerp_on = false;
                        std::cout << "SLERP: OFF" << std::endl;
                        break;
                    case SDLK_2:
                        app.camera.config.smoothing = 0.85f;
                        app.slerp_on = true;
                        std::cout << "SLERP: ON" << std::endl;
                        break;
                    default: break;
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
                    qe::renderer::gl::glViewport(0, 0, event.window.data1, event.window.data2);
                    app.camera.config.aspect =
                        static_cast<float>(event.window.data1) /
                        static_cast<float>(event.window.data2);
                }
                break;
            default: break;
        }
    }
}

// ── Update ──────────────────────────────────────────────────────────────────
void update(AppState& app, float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float fw = 0, rt = 0, up = 0;
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
    app.time += dt;
}

// ── Render ──────────────────────────────────────────────────────────────────
void render(AppState& app) {
    using namespace qe::renderer::gl;
    using namespace qe::math;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    app.shader.use();

    // Camera
    Mat4 vp = app.camera.vp_matrix();
    app.shader.set_mat4("uViewProjection", vp);

    // Lighting
    app.shader.set_vec3("uLightDir", Vec3(0.3f, 0.8f, 0.5f).normalized());
    app.shader.set_vec3("uLightColor", Vec3(1.0f, 0.95f, 0.9f));
    app.shader.set_vec3("uAmbient", Vec3(0.12f, 0.12f, 0.18f));
    app.shader.set_vec3("uCameraPos", app.camera.position());
    app.shader.set_int("uTexture0", 0);

    // --- Draw scene objects ---
    for (const auto& obj : app.objects) {
        Mat4 model = obj.model_matrix(app.time);
        app.shader.set_mat4("uModel", model);

        // Texture binding
        if (obj.texture_id >= 0 &&
            obj.texture_id < static_cast<int>(app.textures.size())) {
            app.textures[obj.texture_id]->bind(0);
            app.shader.set_int("uUseTexture", 1);
        } else {
            app.shader.set_int("uUseTexture", 0);
        }

        // Mesh selection
        switch (obj.mesh_type) {
            case SceneObject::MeshType::Sphere: app.sphere.draw(); break;
            case SceneObject::MeshType::Floor:
                glDisable(GL_CULL_FACE);
                app.floor_plane.draw();
                glEnable(GL_CULL_FACE);
                break;
            case SceneObject::MeshType::OBJ: app.obj_mesh.draw(); break;
            case SceneObject::MeshType::Cube:
            default: app.cube.draw(); break;
        }
    }

    // --- TPS target indicator ---
    if (app.camera.mode() == qe::renderer::CameraMode::ThirdPerson) {
        app.shader.set_int("uUseTexture", 0);
        Vec3 target = app.camera.tps_target();
        app.shader.set_mat4("uModel", Mat4::trs(target,
            Quaternion::from_axis_angle(Vec3::up(), app.time * 2.0f),
            Vec3(0.3f, 0.3f, 0.3f)));
        app.cube.draw();
    }

    // --- Grid overlay ---
    glDisable(GL_CULL_FACE);
    app.shader.set_int("uUseTexture", 0);
    app.shader.set_mat4("uModel", Mat4::identity());
    glBindVertexArray(app.grid.vao);
    glDrawElements(GL_LINES, app.grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

// ── Title ───────────────────────────────────────────────────────────────────
void update_title(AppState& app) {
    const char* mode =
        (app.camera.mode() == qe::renderer::CameraMode::FirstPerson) ? "FPS" : "TPS";
    std::ostringstream t;
    t << "QuatEngine | " << static_cast<int>(app.current_fps) << " FPS"
      << " | " << mode
      << " | SLERP:" << (app.slerp_on ? "ON" : "OFF")
      << " | Objs:" << app.objects.size()
      << " | Pos:(" << static_cast<int>(app.camera.position().x)
      << "," << static_cast<int>(app.camera.position().y)
      << "," << static_cast<int>(app.camera.position().z) << ")";
    SDL_SetWindowTitle(app.window, t.str().c_str());
}

// ── Cleanup ─────────────────────────────────────────────────────────────────
void cleanup(AppState& app) {
    app.cube.destroy();
    app.sphere.destroy();
    app.floor_plane.destroy();
    app.grid.destroy();
    app.obj_mesh.destroy();
    app.shader.destroy();
    app.tex_checker.destroy();
    app.tex_bricks.destroy();
    app.tex_floor.destroy();
    app.tex_white.destroy();

    if (app.gl_context) SDL_GL_DeleteContext(app.gl_context);
    if (app.window)     SDL_DestroyWindow(app.window);
    SDL_Quit();
}
