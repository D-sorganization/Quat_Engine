// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file ConfigManager.h
 * @brief Runtime configuration manager with dotenv and environment loading support.
 */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace qe {
namespace config {

class ConfigManager {
private:
    int m_default_window_width = 1280;
    int m_default_window_height = 720;
    std::string m_window_title = "QuatEngine";
    int m_gl_major_version = 3;
    int m_gl_minor_version = 3;
    int m_msaa_samples = 4;
    float m_max_delta_time = 0.1f;
    float m_fps_update_interval = 0.5f;
    float m_clear_r = 0.02f;
    float m_clear_g = 0.02f;
    float m_clear_b = 0.06f;
    float m_clear_a = 1.0f;
    float m_fog_near = 30.0f;
    float m_fog_far = 80.0f;
    float m_rim_power = 3.0f;
    int m_max_point_lights = 8;
    float m_default_camera_smoothing = 0.85f;
    float m_default_camera_move_speed = 5.0f;
    float m_default_camera_sprint_mult = 2.5f;
    float m_default_camera_sensitivity = 0.003f;
    float m_default_orbit_distance = 5.0f;
    float m_default_orbit_height = 2.0f;
    float m_default_orbit_smoothing = 0.9f;
    float m_projectile_lifetime = 3.0f;
    float m_projectile_radius = 0.08f;
    int m_kill_score = 100;
    float m_gamepad_look_speed = 5.0f;
    float m_default_gravity = -9.8f;

    static inline std::string trim(const std::string& str) {
        auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        });
        auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
                       return !std::isspace(ch);
                   }).base();
        return (start < end) ? std::string(start, end) : "";
    }

    static inline int parse_int(const std::string& str, const std::string& name) {
        try {
            size_t idx = 0;
            int val = std::stoi(str, &idx);
            if (idx < str.size()) {
                throw std::invalid_argument("Invalid integer suffix: " + str);
            }
            return val;
        } catch (const std::exception& e) {
            throw std::invalid_argument("Failed to parse integer for variable " + name + ": \"" +
                                        str + "\" (" + e.what() + ")");
        }
    }

    static inline float parse_float(const std::string& str, const std::string& name) {
        try {
            size_t idx = 0;
            float val = std::stof(str, &idx);
            if (idx < str.size()) {
                std::string suffix = trim(str.substr(idx));
                if (suffix != "f" && suffix != "F" && !suffix.empty()) {
                    throw std::invalid_argument("Invalid float suffix: " + str);
                }
            }
            return val;
        } catch (const std::exception& e) {
            throw std::invalid_argument("Failed to parse float for variable " + name + ": \"" +
                                        str + "\" (" + e.what() + ")");
        }
    }

public:
    ConfigManager() = default;

    void reset_to_defaults() {
        m_default_window_width = 1280;
        m_default_window_height = 720;
        m_window_title = "QuatEngine";
        m_gl_major_version = 3;
        m_gl_minor_version = 3;
        m_msaa_samples = 4;
        m_max_delta_time = 0.1f;
        m_fps_update_interval = 0.5f;
        m_clear_r = 0.02f;
        m_clear_g = 0.02f;
        m_clear_b = 0.06f;
        m_clear_a = 1.0f;
        m_fog_near = 30.0f;
        m_fog_far = 80.0f;
        m_rim_power = 3.0f;
        m_max_point_lights = 8;
        m_default_camera_smoothing = 0.85f;
        m_default_camera_move_speed = 5.0f;
        m_default_camera_sprint_mult = 2.5f;
        m_default_camera_sensitivity = 0.003f;
        m_default_orbit_distance = 5.0f;
        m_default_orbit_height = 2.0f;
        m_default_orbit_smoothing = 0.9f;
        m_projectile_lifetime = 3.0f;
        m_projectile_radius = 0.08f;
        m_kill_score = 100;
        m_gamepad_look_speed = 5.0f;
        m_default_gravity = -9.8f;
    }

    void validate() const {
        if (m_default_window_width <= 0)
            throw std::invalid_argument("DEFAULT_WINDOW_WIDTH must be positive");
        if (m_default_window_height <= 0)
            throw std::invalid_argument("DEFAULT_WINDOW_HEIGHT must be positive");
        if (m_gl_major_version < 1)
            throw std::invalid_argument("GL_MAJOR_VERSION must be at least 1");
        if (m_gl_minor_version < 0)
            throw std::invalid_argument("GL_MINOR_VERSION must be non-negative");
        if (m_msaa_samples < 0)
            throw std::invalid_argument("MSAA_SAMPLES must be non-negative");
        if (m_max_delta_time <= 0.0f)
            throw std::invalid_argument("MAX_DELTA_TIME must be positive");
        if (m_fps_update_interval <= 0.0f)
            throw std::invalid_argument("FPS_UPDATE_INTERVAL must be positive");
        if (m_fog_near < 0.0f)
            throw std::invalid_argument("FOG_NEAR must be non-negative");
        if (m_fog_far < m_fog_near)
            throw std::invalid_argument("FOG_FAR must be greater than or equal to FOG_NEAR");
        if (m_rim_power <= 0.0f)
            throw std::invalid_argument("RIM_POWER must be positive");
        if (m_max_point_lights <= 0)
            throw std::invalid_argument("MAX_POINT_LIGHTS must be positive");
        if (m_default_camera_smoothing < 0.0f || m_default_camera_smoothing >= 1.0f)
            throw std::invalid_argument("DEFAULT_CAMERA_SMOOTHING must be in [0, 1)");
        if (m_default_camera_move_speed <= 0.0f)
            throw std::invalid_argument("DEFAULT_CAMERA_MOVE_SPEED must be positive");
        if (m_default_camera_sprint_mult <= 0.0f)
            throw std::invalid_argument("DEFAULT_CAMERA_SPRINT_MULT must be positive");
        if (m_default_camera_sensitivity <= 0.0f)
            throw std::invalid_argument("DEFAULT_CAMERA_SENSITIVITY must be positive");
        if (m_default_orbit_distance <= 0.0f)
            throw std::invalid_argument("DEFAULT_ORBIT_DISTANCE must be positive");
        if (m_default_orbit_height < 0.0f)
            throw std::invalid_argument("DEFAULT_ORBIT_HEIGHT must be non-negative");
        if (m_default_orbit_smoothing < 0.0f || m_default_orbit_smoothing >= 1.0f)
            throw std::invalid_argument("DEFAULT_ORBIT_SMOOTHING must be in [0, 1)");
        if (m_projectile_lifetime <= 0.0f)
            throw std::invalid_argument("PROJECTILE_LIFETIME must be positive");
        if (m_projectile_radius <= 0.0f)
            throw std::invalid_argument("PROJECTILE_RADIUS must be positive");
        if (m_kill_score < 0)
            throw std::invalid_argument("KILL_SCORE must be non-negative");
        if (m_gamepad_look_speed <= 0.0f)
            throw std::invalid_argument("GAMEPAD_LOOK_SPEED must be positive");
    }

    void load_from_map(const std::map<std::string, std::string>& settings) {
        for (const auto& pair : settings) {
            std::string key = pair.first;
            std::string val = pair.second;
            if (key == "QE_DEFAULT_WINDOW_WIDTH" || key == "DEFAULT_WINDOW_WIDTH") {
                m_default_window_width = parse_int(val, key);
            } else if (key == "QE_DEFAULT_WINDOW_HEIGHT" || key == "DEFAULT_WINDOW_HEIGHT") {
                m_default_window_height = parse_int(val, key);
            } else if (key == "QE_WINDOW_TITLE" || key == "WINDOW_TITLE") {
                m_window_title = val;
            } else if (key == "QE_GL_MAJOR_VERSION" || key == "GL_MAJOR_VERSION") {
                m_gl_major_version = parse_int(val, key);
            } else if (key == "QE_GL_MINOR_VERSION" || key == "GL_MINOR_VERSION") {
                m_gl_minor_version = parse_int(val, key);
            } else if (key == "QE_MSAA_SAMPLES" || key == "MSAA_SAMPLES") {
                m_msaa_samples = parse_int(val, key);
            } else if (key == "QE_MAX_DELTA_TIME" || key == "MAX_DELTA_TIME") {
                m_max_delta_time = parse_float(val, key);
            } else if (key == "QE_FPS_UPDATE_INTERVAL" || key == "FPS_UPDATE_INTERVAL") {
                m_fps_update_interval = parse_float(val, key);
            } else if (key == "QE_CLEAR_R" || key == "CLEAR_R") {
                m_clear_r = parse_float(val, key);
            } else if (key == "QE_CLEAR_G" || key == "CLEAR_G") {
                m_clear_g = parse_float(val, key);
            } else if (key == "QE_CLEAR_B" || key == "CLEAR_B") {
                m_clear_b = parse_float(val, key);
            } else if (key == "QE_CLEAR_A" || key == "CLEAR_A") {
                m_clear_a = parse_float(val, key);
            } else if (key == "QE_FOG_NEAR" || key == "FOG_NEAR") {
                m_fog_near = parse_float(val, key);
            } else if (key == "QE_FOG_FAR" || key == "FOG_FAR") {
                m_fog_far = parse_float(val, key);
            } else if (key == "QE_RIM_POWER" || key == "RIM_POWER") {
                m_rim_power = parse_float(val, key);
            } else if (key == "QE_MAX_POINT_LIGHTS" || key == "MAX_POINT_LIGHTS") {
                m_max_point_lights = parse_int(val, key);
            } else if (key == "QE_DEFAULT_CAMERA_SMOOTHING" || key == "DEFAULT_CAMERA_SMOOTHING") {
                m_default_camera_smoothing = parse_float(val, key);
            } else if (key == "QE_DEFAULT_CAMERA_MOVE_SPEED" ||
                       key == "DEFAULT_CAMERA_MOVE_SPEED") {
                m_default_camera_move_speed = parse_float(val, key);
            } else if (key == "QE_DEFAULT_CAMERA_SPRINT_MULT" ||
                       key == "DEFAULT_CAMERA_SPRINT_MULT") {
                m_default_camera_sprint_mult = parse_float(val, key);
            } else if (key == "QE_DEFAULT_CAMERA_SENSITIVITY" ||
                       key == "DEFAULT_CAMERA_SENSITIVITY") {
                m_default_camera_sensitivity = parse_float(val, key);
            } else if (key == "QE_DEFAULT_ORBIT_DISTANCE" || key == "DEFAULT_ORBIT_DISTANCE") {
                m_default_orbit_distance = parse_float(val, key);
            } else if (key == "QE_DEFAULT_ORBIT_HEIGHT" || key == "DEFAULT_ORBIT_HEIGHT") {
                m_default_orbit_height = parse_float(val, key);
            } else if (key == "QE_DEFAULT_ORBIT_SMOOTHING" || key == "DEFAULT_ORBIT_SMOOTHING") {
                m_default_orbit_smoothing = parse_float(val, key);
            } else if (key == "QE_PROJECTILE_LIFETIME" || key == "PROJECTILE_LIFETIME") {
                m_projectile_lifetime = parse_float(val, key);
            } else if (key == "QE_PROJECTILE_RADIUS" || key == "PROJECTILE_RADIUS") {
                m_projectile_radius = parse_float(val, key);
            } else if (key == "QE_KILL_SCORE" || key == "KILL_SCORE") {
                m_kill_score = parse_int(val, key);
            } else if (key == "QE_GAMEPAD_LOOK_SPEED" || key == "GAMEPAD_LOOK_SPEED") {
                m_gamepad_look_speed = parse_float(val, key);
            } else if (key == "QE_DEFAULT_GRAVITY" || key == "DEFAULT_GRAVITY") {
                m_default_gravity = parse_float(val, key);
            }
        }
        validate();
    }

    void load_from_env_vars() {
        std::map<std::string, std::string> settings;
        const char* keys[] = {"QE_DEFAULT_WINDOW_WIDTH",
                              "QE_DEFAULT_WINDOW_HEIGHT",
                              "QE_WINDOW_TITLE",
                              "QE_GL_MAJOR_VERSION",
                              "QE_GL_MINOR_VERSION",
                              "QE_MSAA_SAMPLES",
                              "QE_MAX_DELTA_TIME",
                              "QE_FPS_UPDATE_INTERVAL",
                              "QE_CLEAR_R",
                              "QE_CLEAR_G",
                              "QE_CLEAR_B",
                              "QE_CLEAR_A",
                              "QE_FOG_NEAR",
                              "QE_FOG_FAR",
                              "QE_RIM_POWER",
                              "QE_MAX_POINT_LIGHTS",
                              "QE_DEFAULT_CAMERA_SMOOTHING",
                              "QE_DEFAULT_CAMERA_MOVE_SPEED",
                              "QE_DEFAULT_CAMERA_SPRINT_MULT",
                              "QE_DEFAULT_CAMERA_SENSITIVITY",
                              "QE_DEFAULT_ORBIT_DISTANCE",
                              "QE_DEFAULT_ORBIT_HEIGHT",
                              "QE_DEFAULT_ORBIT_SMOOTHING",
                              "QE_PROJECTILE_LIFETIME",
                              "QE_PROJECTILE_RADIUS",
                              "QE_KILL_SCORE",
                              "QE_GAMEPAD_LOOK_SPEED",
                              "QE_DEFAULT_GRAVITY"};
        for (const char* key : keys) {
            if (const char* val = std::getenv(key)) {
                settings[key] = val;
            }
        }
        if (!settings.empty()) {
            load_from_map(settings);
        }
    }

    void load_from_stream(std::istream& stream) {
        std::map<std::string, std::string> settings;
        std::string line;
        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            size_t eq_idx = line.find('=');
            if (eq_idx == std::string::npos) {
                continue;
            }
            std::string key = trim(line.substr(0, eq_idx));
            std::string val = trim(line.substr(eq_idx + 1));
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                val = val.substr(1, val.size() - 2);
            } else if (val.size() >= 2 && val.front() == '\'' && val.back() == '\'') {
                val = val.substr(1, val.size() - 2);
            }
            settings[key] = val;
        }
        load_from_map(settings);
    }

    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        load_from_stream(file);
        return true;
    }

    void load(const std::string& env_filepath = ".env") {
        reset_to_defaults();
        load_from_file(env_filepath);
        load_from_env_vars();
    }

    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    int get_default_window_width() const { return m_default_window_width; }
    int get_default_window_height() const { return m_default_window_height; }
    std::string get_window_title() const { return m_window_title; }
    int get_gl_major_version() const { return m_gl_major_version; }
    int get_gl_minor_version() const { return m_gl_minor_version; }
    int get_msaa_samples() const { return m_msaa_samples; }
    float get_max_delta_time() const { return m_max_delta_time; }
    float get_fps_update_interval() const { return m_fps_update_interval; }
    float get_clear_r() const { return m_clear_r; }
    float get_clear_g() const { return m_clear_g; }
    float get_clear_b() const { return m_clear_b; }
    float get_clear_a() const { return m_clear_a; }
    float get_fog_near() const { return m_fog_near; }
    float get_fog_far() const { return m_fog_far; }
    float get_rim_power() const { return m_rim_power; }
    int get_max_point_lights() const { return m_max_point_lights; }
    float get_default_camera_smoothing() const { return m_default_camera_smoothing; }
    float get_default_camera_move_speed() const { return m_default_camera_move_speed; }
    float get_default_camera_sprint_mult() const { return m_default_camera_sprint_mult; }
    float get_default_camera_sensitivity() const { return m_default_camera_sensitivity; }
    float get_default_orbit_distance() const { return m_default_orbit_distance; }
    float get_default_orbit_height() const { return m_default_orbit_height; }
    float get_default_orbit_smoothing() const { return m_default_orbit_smoothing; }
    float get_projectile_lifetime() const { return m_projectile_lifetime; }
    float get_projectile_radius() const { return m_projectile_radius; }
    int get_kill_score() const { return m_kill_score; }
    float get_gamepad_look_speed() const { return m_gamepad_look_speed; }
    float get_default_gravity() const { return m_default_gravity; }
};

}  // namespace config
}  // namespace qe
