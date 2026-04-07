#pragma once
/**
 * @file EngineConfig.h
 * @brief Centralized engine configuration constants.
 *
 * All hardcoded "magic numbers" from main.cpp, tps_main.cpp, and subsystems
 * are collected here so they can be tuned from a single location.
 *
 * Categories:
 *   - Window:     Default resolution, title, OpenGL version
 *   - Rendering:  Clear color, fog distances, rim-light parameters
 *   - Camera:     Default smoothing, speed, sprint multiplier
 *   - Gameplay:   Projectile defaults, FPS display interval
 *   - Physics:    Max delta-time cap, gravity
 */

namespace qe {
namespace config {

// ── Window Defaults ────────────────────────────────────────────────────────

constexpr int    DEFAULT_WINDOW_WIDTH   = 1280;
constexpr int    DEFAULT_WINDOW_HEIGHT  = 720;
constexpr char   WINDOW_TITLE[]         = "QuatEngine";
constexpr int    GL_MAJOR_VERSION       = 3;
constexpr int    GL_MINOR_VERSION       = 3;
constexpr int    MSAA_SAMPLES           = 4;

// ── Timing ─────────────────────────────────────────────────────────────────

/** Maximum frame delta-time (seconds).  Prevents physics explosions on lag. */
constexpr float  MAX_DELTA_TIME         = 0.1f;

/** Interval (seconds) between FPS counter updates in the title bar. */
constexpr float  FPS_UPDATE_INTERVAL    = 0.5f;

// ── Rendering ──────────────────────────────────────────────────────────────

constexpr float  CLEAR_R = 0.02f;
constexpr float  CLEAR_G = 0.02f;
constexpr float  CLEAR_B = 0.06f;
constexpr float  CLEAR_A = 1.0f;

constexpr float  FOG_NEAR  = 30.0f;
constexpr float  FOG_FAR   = 80.0f;

constexpr float  RIM_POWER = 3.0f;

constexpr int    MAX_POINT_LIGHTS = 8;

// ── Camera Defaults ────────────────────────────────────────────────────────

constexpr float  DEFAULT_CAMERA_SMOOTHING    = 0.85f;
constexpr float  DEFAULT_CAMERA_MOVE_SPEED   = 5.0f;
constexpr float  DEFAULT_CAMERA_SPRINT_MULT  = 2.5f;
constexpr float  DEFAULT_CAMERA_SENSITIVITY  = 0.003f;

// TPS orbit
constexpr float  DEFAULT_ORBIT_DISTANCE      = 5.0f;
constexpr float  DEFAULT_ORBIT_HEIGHT         = 2.0f;
constexpr float  DEFAULT_ORBIT_SMOOTHING      = 0.9f;

// ── Gameplay ───────────────────────────────────────────────────────────────

constexpr float  PROJECTILE_LIFETIME   = 3.0f;
constexpr float  PROJECTILE_RADIUS     = 0.08f;
constexpr int    KILL_SCORE            = 100;
constexpr float  GAMEPAD_LOOK_SPEED    = 5.0f;

// ── Physics ────────────────────────────────────────────────────────────────

constexpr float  DEFAULT_GRAVITY       = -9.8f;

} // namespace config
} // namespace qe
