// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.

#include "../src/core/ConfigManager.h"
#include "test_framework.h"

#include <sstream>

void test_config_defaults() {
    qe::config::ConfigManager config;
    config.reset_to_defaults();

    ASSERT_TRUE(config.get_default_window_width() == 1280);
    ASSERT_TRUE(config.get_default_window_height() == 720);
    ASSERT_TRUE(config.get_window_title() == "QuatEngine");
    ASSERT_TRUE(config.get_gl_major_version() == 3);
    ASSERT_TRUE(config.get_gl_minor_version() == 3);
    ASSERT_TRUE(config.get_msaa_samples() == 4);
    ASSERT_FLOAT_EQ(config.get_max_delta_time(), 0.1f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_fps_update_interval(), 0.5f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_clear_r(), 0.02f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_clear_g(), 0.02f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_clear_b(), 0.06f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_clear_a(), 1.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_fog_near(), 30.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_fog_far(), 80.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_rim_power(), 3.0f, 1e-5f);
    ASSERT_TRUE(config.get_max_point_lights() == 8);
    ASSERT_FLOAT_EQ(config.get_default_camera_smoothing(), 0.85f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_camera_move_speed(), 5.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_camera_sprint_mult(), 2.5f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_camera_sensitivity(), 0.003f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_orbit_distance(), 5.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_orbit_height(), 2.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_orbit_smoothing(), 0.9f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_projectile_lifetime(), 3.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_projectile_radius(), 0.08f, 1e-5f);
    ASSERT_TRUE(config.get_kill_score() == 100);
    ASSERT_FLOAT_EQ(config.get_gamepad_look_speed(), 5.0f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_gravity(), -9.8f, 1e-5f);
}

void test_config_load_from_stream() {
    qe::config::ConfigManager config;
    config.reset_to_defaults();

    std::stringstream ss;
    ss << "# This is a comment\n"
       << "QE_DEFAULT_WINDOW_WIDTH=1920\n"
       << "QE_DEFAULT_WINDOW_HEIGHT = 1080\n"
       << "QE_WINDOW_TITLE = \"My Custom Engine\"\n"
       << "QE_MAX_DELTA_TIME = 0.05f\n"
       << "QE_DEFAULT_GRAVITY = -1.62f\n";

    config.load_from_stream(ss);

    ASSERT_TRUE(config.get_default_window_width() == 1920);
    ASSERT_TRUE(config.get_default_window_height() == 1080);
    ASSERT_TRUE(config.get_window_title() == "My Custom Engine");
    ASSERT_FLOAT_EQ(config.get_max_delta_time(), 0.05f, 1e-5f);
    ASSERT_FLOAT_EQ(config.get_default_gravity(), -1.62f, 1e-5f);
}

void test_config_dbc_validations() {
    qe::config::ConfigManager config;
    config.reset_to_defaults();

    // Verify positive/negative boundary assertions (contracts)
    std::map<std::string, std::string> bad_width = {{"QE_DEFAULT_WINDOW_WIDTH", "0"}};
    ASSERT_THROWS(config.load_from_map(bad_width));

    std::map<std::string, std::string> bad_height = {{"QE_DEFAULT_WINDOW_HEIGHT", "-10"}};
    ASSERT_THROWS(config.load_from_map(bad_height));

    std::map<std::string, std::string> bad_gl = {{"QE_GL_MAJOR_VERSION", "0"}};
    ASSERT_THROWS(config.load_from_map(bad_gl));

    std::map<std::string, std::string> bad_dt = {{"QE_MAX_DELTA_TIME", "-0.01"}};
    ASSERT_THROWS(config.load_from_map(bad_dt));

    std::map<std::string, std::string> bad_fog = {
        {"QE_FOG_NEAR", "40.0"}, {"QE_FOG_FAR", "30.0"}  // fog_far < fog_near is invalid
    };
    ASSERT_THROWS(config.load_from_map(bad_fog));

    std::map<std::string, std::string> bad_camera_smoothing = {
        {"QE_DEFAULT_CAMERA_SMOOTHING", "1.0"}};
    ASSERT_THROWS(config.load_from_map(bad_camera_smoothing));
}

void test_config_parse_errors() {
    qe::config::ConfigManager config;
    config.reset_to_defaults();

    // Invalid integer values
    std::map<std::string, std::string> bad_int_format = {{"QE_DEFAULT_WINDOW_WIDTH", "abc"}};
    ASSERT_THROWS(config.load_from_map(bad_int_format));

    // Invalid float values
    std::map<std::string, std::string> bad_float_format = {{"QE_MAX_DELTA_TIME", "0.1xyz"}};
    ASSERT_THROWS(config.load_from_map(bad_float_format));
}

int main() {
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_config_load_from_stream);
    RUN_TEST(test_config_dbc_validations);
    RUN_TEST(test_config_parse_errors);
    return TEST_REPORT();
}
