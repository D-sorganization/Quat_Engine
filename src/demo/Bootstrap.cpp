// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Bootstrap.cpp
 * @brief Demo bootstrap and teardown.
 */

#include "demo/App.h"
#include "renderer/GLLoader.h"
#include "renderer/MeshPrimitives.h"

#include <iostream>

namespace qe::demo {

bool init_window(App& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        QE_LOG_ERROR("SDL") << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, qe::config::GL_MAJOR_VERSION());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, qe::config::GL_MINOR_VERSION());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, qe::config::MSAA_SAMPLES());

    app.window = SDL_CreateWindow(qe::config::WINDOW_TITLE().c_str(),
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  qe::config::DEFAULT_WINDOW_WIDTH(),
                                  qe::config::DEFAULT_WINDOW_HEIGHT(),
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!app.window) {
        return false;
    }

    app.gl_context = SDL_GL_CreateContext(app.window);
    if (!app.gl_context) {
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    return true;
}

bool init_gl(App& app) {
    if (!qe::renderer::gl::load()) {
        return false;
    }

    const char* gpu = qe::renderer::gl::glGetString(GL_RENDERER);
    QE_LOG_INFO("Engine") << "GPU: " << (gpu ? gpu : "?") << std::endl;

    using namespace qe::renderer::gl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(qe::config::CLEAR_R(),
                 qe::config::CLEAR_G(),
                 qe::config::CLEAR_B(),
                 qe::config::CLEAR_A());

    if (!app.world_shader.load_from_files("shaders/world.vert", "shaders/world.frag")) {
        return false;
    }
    if (!app.particle_shader.load_from_files("shaders/particle.vert", "shaders/particle.frag")) {
        return false;
    }
    if (!app.hud_shader.load_from_files("shaders/hud.vert", "shaders/hud.frag")) {
        return false;
    }

    app.post_process =
        std::make_unique<qe::renderer::PostProcess>(qe::config::DEFAULT_WINDOW_WIDTH(),
                                                    qe::config::DEFAULT_WINDOW_HEIGHT());
    app.post_process->init("shaders/post.vert", "shaders/post.frag");
    app.post_process->crtEnabled = 1;
    app.post_process->aberrationEnabled = 1;
    app.post_process->vignetteEnabled = 1;
    app.post_process->grainEnabled = 1;
    return true;
}

void init_assets(App& app) {
    app.cube = qe::renderer::create_cube();
    app.sphere = qe::renderer::create_sphere(3, 0.5f, 0.8f, 0.6f, 0.3f);
    app.floor_plane = qe::renderer::create_floor_plane(40, 12);
    app.grid = qe::renderer::create_grid(40, 1);

    app.tex_checker = qe::renderer::Texture::create_checkerboard(256, 8, 220, 220, 230, 50, 50, 60);
    app.tex_bricks = qe::renderer::Texture::create_bricks(256);
    app.tex_floor = qe::renderer::Texture::create_floor(256);
    app.textures = {&app.tex_checker, &app.tex_bricks, &app.tex_floor};

    app.hud.init_crosshair();
}

void initialize_runtime(App& app) {
    qe::renderer::Camera::Config camera_config;
    camera_config.aspect = static_cast<float>(qe::config::DEFAULT_WINDOW_WIDTH()) /
                           static_cast<float>(qe::config::DEFAULT_WINDOW_HEIGHT());
    camera_config.smoothing = qe::config::DEFAULT_CAMERA_SMOOTHING();
    camera_config.move_speed = qe::config::DEFAULT_CAMERA_MOVE_SPEED();
    camera_config.sprint_mult = qe::config::DEFAULT_CAMERA_SPRINT_MULT();
    app.camera = qe::renderer::Camera(camera_config);
    app.camera.set_position({0, 1.5f, 15});

    app.input.init();
    app.input.set_gamepad_look_speed(qe::config::GAMEPAD_LOOK_SPEED());

    app.decorations = qe::game::build_decorations();
    app.waves.start_game();
    spawn_wave_targets(app);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    app.last_time = SDL_GetPerformanceCounter();

    std::cout << "\nQuatEngine - Quaternion-Powered FPS Shooter\n"
              << "===========================================\n"
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
              << "===========================================\n";
}

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
    if (app.gl_context) {
        SDL_GL_DeleteContext(app.gl_context);
    }
    if (app.window) {
        SDL_DestroyWindow(app.window);
    }
    SDL_Quit();
}

}  // namespace qe::demo
