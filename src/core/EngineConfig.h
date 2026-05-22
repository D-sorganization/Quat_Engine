// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file EngineConfig.h
 * @brief Centralized engine configuration variables resolved at runtime.
 *
 * All hardcoded "magic numbers" from main.cpp, tps_main.cpp, and subsystems
 * are resolved dynamically via ConfigManager.
 *
 * Categories:
 *   - Window:     Default resolution, title, OpenGL version
 *   - Rendering:  Clear color, fog distances, rim-light parameters
 *   - Camera:     Default smoothing, speed, sprint multiplier
 *   - Gameplay:   Projectile defaults, FPS display interval
 *   - Physics:    Max delta-time cap, gravity
 */

#include "ConfigManager.h"

#include <string>

namespace qe {
namespace config {

// ── Window Defaults ────────────────────────────────────────────────────────

inline int DEFAULT_WINDOW_WIDTH() {
    return ConfigManager::instance().get_default_window_width();
}
inline int DEFAULT_WINDOW_HEIGHT() {
    return ConfigManager::instance().get_default_window_height();
}
inline std::string WINDOW_TITLE() {
    return ConfigManager::instance().get_window_title();
}
inline int GL_MAJOR_VERSION() {
    return ConfigManager::instance().get_gl_major_version();
}
inline int GL_MINOR_VERSION() {
    return ConfigManager::instance().get_gl_minor_version();
}
inline int MSAA_SAMPLES() {
    return ConfigManager::instance().get_msaa_samples();
}

// ── Timing ─────────────────────────────────────────────────────────────────

/** Maximum frame delta-time (seconds).  Prevents physics explosions on lag. */
inline float MAX_DELTA_TIME() {
    return ConfigManager::instance().get_max_delta_time();
}

/** Interval (seconds) between FPS counter updates in the title bar. */
inline float FPS_UPDATE_INTERVAL() {
    return ConfigManager::instance().get_fps_update_interval();
}

// ── Rendering ──────────────────────────────────────────────────────────────

inline float CLEAR_R() {
    return ConfigManager::instance().get_clear_r();
}
inline float CLEAR_G() {
    return ConfigManager::instance().get_clear_g();
}
inline float CLEAR_B() {
    return ConfigManager::instance().get_clear_b();
}
inline float CLEAR_A() {
    return ConfigManager::instance().get_clear_a();
}

inline float FOG_NEAR() {
    return ConfigManager::instance().get_fog_near();
}
inline float FOG_FAR() {
    return ConfigManager::instance().get_fog_far();
}

inline float RIM_POWER() {
    return ConfigManager::instance().get_rim_power();
}

inline int MAX_POINT_LIGHTS() {
    return ConfigManager::instance().get_max_point_lights();
}

// ── Camera Defaults ────────────────────────────────────────────────────────

inline float DEFAULT_CAMERA_SMOOTHING() {
    return ConfigManager::instance().get_default_camera_smoothing();
}
inline float DEFAULT_CAMERA_MOVE_SPEED() {
    return ConfigManager::instance().get_default_camera_move_speed();
}
inline float DEFAULT_CAMERA_SPRINT_MULT() {
    return ConfigManager::instance().get_default_camera_sprint_mult();
}
inline float DEFAULT_CAMERA_SENSITIVITY() {
    return ConfigManager::instance().get_default_camera_sensitivity();
}

// TPS orbit
inline float DEFAULT_ORBIT_DISTANCE() {
    return ConfigManager::instance().get_default_orbit_distance();
}
inline float DEFAULT_ORBIT_HEIGHT() {
    return ConfigManager::instance().get_default_orbit_height();
}
inline float DEFAULT_ORBIT_SMOOTHING() {
    return ConfigManager::instance().get_default_orbit_smoothing();
}

// ── Gameplay ───────────────────────────────────────────────────────────────

inline float PROJECTILE_LIFETIME() {
    return ConfigManager::instance().get_projectile_lifetime();
}
inline float PROJECTILE_RADIUS() {
    return ConfigManager::instance().get_projectile_radius();
}
inline int KILL_SCORE() {
    return ConfigManager::instance().get_kill_score();
}
inline float GAMEPAD_LOOK_SPEED() {
    return ConfigManager::instance().get_gamepad_look_speed();
}

// ── Physics ────────────────────────────────────────────────────────────────

inline float DEFAULT_GRAVITY() {
    return ConfigManager::instance().get_default_gravity();
}

}  // namespace config
}  // namespace qe
