/**
 * @file main.cpp
 * @brief QuatEngine demo composition root.
 */

#include "demo/App.h"

#include <SDL.h>

int main(int /*argc*/, char* /*argv*/[]) {
    qe::demo::App app;

    if (!qe::demo::init_window(app)) {
        return 1;
    }
    if (!qe::demo::init_gl(app)) {
        return 1;
    }

    qe::demo::init_assets(app);
    qe::demo::initialize_runtime(app);

    while (app.running) {
        const Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - app.last_time) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        app.last_time = now;
        if (dt > qe::config::MAX_DELTA_TIME) {
            dt = qe::config::MAX_DELTA_TIME;
        }

        qe::demo::handle_events(app);
        qe::demo::update(app, dt);

        if (app.post_process) {
            app.post_process->bind();
        }
        qe::demo::render_world(app);
        qe::demo::render_particles(app);
        if (app.post_process) {
            app.post_process->unbind();
            app.post_process->render(app.time);
        }

        qe::demo::render_hud(app);
        SDL_GL_SwapWindow(app.window);

        ++app.frame_count;
        app.fps_timer += dt;
        if (app.fps_timer >= qe::config::FPS_UPDATE_INTERVAL) {
            app.current_fps = static_cast<float>(app.frame_count) / app.fps_timer;
            qe::demo::update_title(app);
            app.frame_count = 0;
            app.fps_timer = 0.0f;
        }
    }

    qe::demo::cleanup(app);
    return 0;
}
