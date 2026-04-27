// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file App.h
 * @brief Shared demo application state and subsystem declarations.
 */

#ifndef QE_DEMO_APP_H
#define QE_DEMO_APP_H

#include "core/EngineConfig.h"
#include "core/Logger.h"
#include "game/Combat.h"
#include "game/PowerUp.h"
#include "game/Scene.h"
#include "game/Scoring.h"
#include "game/TargetBehavior.h"
#include "game/WaveSystem.h"
#include "game/Weapons.h"
#include "input/InputManager.h"
#include "renderer/Camera.h"
#include "renderer/HUD.h"
#include "renderer/Mesh.h"
#include "renderer/ParticleSystem.h"
#include "renderer/PostProcess.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"

#include <SDL.h>

#include <memory>
#include <vector>

namespace qe::demo {

struct App {
    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    bool running = true;

    qe::input::InputManager input;
    qe::renderer::Camera camera;
    qe::renderer::Shader world_shader;
    qe::renderer::Shader particle_shader;
    qe::renderer::Shader hud_shader;
    qe::renderer::HUD hud;

    qe::renderer::Mesh cube;
    qe::renderer::Mesh sphere;
    qe::renderer::Mesh floor_plane;
    qe::renderer::Mesh grid;

    qe::renderer::Texture tex_checker;
    qe::renderer::Texture tex_bricks;
    qe::renderer::Texture tex_floor;
    std::vector<qe::renderer::Texture*> textures;

    qe::game::WaveSystem waves;
    qe::game::WeaponManager weapons;
    qe::game::ScoreTracker score;
    qe::game::PowerUpManager powerups;
    qe::renderer::ParticleSystem particles;
    std::unique_ptr<qe::renderer::PostProcess> post_process;

    std::vector<qe::game::Decoration> decorations;
    std::vector<qe::core::Entity> entities;
    std::vector<qe::game::TargetBehavior> behaviors;
    std::vector<qe::core::Projectile> projectiles;

    qe::game::CombatConfig combat_cfg;

    bool wireframe = false;
    bool slerp_on = true;

    float time = 0.0f;
    Uint64 last_time = 0;
    int frame_count = 0;
    float fps_timer = 0.0f;
    float current_fps = 0.0f;
};

bool init_window(App& app);
bool init_gl(App& app);
void init_assets(App& app);
void initialize_runtime(App& app);
void handle_events(App& app);
void update(App& app, float dt);
void render_world(App& app);
void render_particles(App& app);
void render_hud(App& app);
void update_title(App& app);
void cleanup(App& app);
void spawn_wave_targets(App& app);

}  // namespace qe::demo

#endif  // QE_DEMO_APP_H
