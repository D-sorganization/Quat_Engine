/**
 * @file test_wave.cpp
 * @brief Tests for WaveSystem: state machine transitions, wave config scaling,
 *        timed transitions, and game time accumulation.
 */

#include "../src/game/WaveSystem.h"

#include <cmath>
#include <iostream>
#include <string>

static int total_assertions = 0;
static int passed = 0;
static int failed = 0;

#define ASSERT_TRUE(expr) do { \
    total_assertions++; \
    if (expr) { passed++; } \
    else { failed++; std::cerr << "  FAIL: " << #expr \
           << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; } \
} while(0)

#define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(std::abs((a)-(b)) < (eps))
#define RUN_TEST(fn) do { std::cout << "  " << #fn << "... "; fn(); std::cout << "OK" << std::endl; } while(0)

// ── Initial State ───────────────────────────────────────────────────────────

void test_initial_state_is_menu() {
    qe::game::WaveSystem ws;
    ASSERT_TRUE(ws.state() == qe::game::GameState::Menu);
    ASSERT_TRUE(!ws.is_playing());
    ASSERT_TRUE(ws.current_wave() == 0);
    ASSERT_NEAR(ws.total_game_time(), 0.0f, 1e-5f);
}

// ── start_game ──────────────────────────────────────────────────────────────

void test_start_game_transitions_to_wave_intro() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);
    ASSERT_TRUE(ws.current_wave() == 1);
    ASSERT_TRUE(ws.is_playing());
    ASSERT_NEAR(ws.time_in_state(), 0.0f, 1e-5f);
}

// ── WaveIntro -> WaveActive timed transition ────────────────────────────────

void test_wave_intro_auto_transitions_after_2s() {
    qe::game::WaveSystem ws;
    ws.start_game();

    // Not yet transitioned at 1.9s
    ws.update(1.9f, 0);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);

    // Should transition at 2.0s
    ws.update(0.2f, 0);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveActive);
    ASSERT_NEAR(ws.time_in_state(), 0.0f, 0.2f);  // Just transitioned
}

// ── WaveActive -> WaveClear when targets dead ───────────────────────────────

void test_wave_active_to_clear_on_zero_targets() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ws.update(2.1f, 5);  // WaveIntro -> WaveActive
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveActive);

    // Targets still alive
    ws.update(1.0f, 3);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveActive);

    // All targets dead
    ws.update(0.1f, 0);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveClear);
}

// ── WaveClear -> WaveIntro timed transition ─────────────────────────────────

void test_wave_clear_auto_transitions_after_3s() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ws.update(2.1f, 5);   // -> WaveActive
    ws.update(0.1f, 0);   // -> WaveClear
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveClear);

    // Not yet at 2.9s
    ws.update(2.9f, 0);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveClear);

    // Should transition at 3.0s
    ws.update(0.2f, 0);
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);
    ASSERT_TRUE(ws.current_wave() == 2);  // Advanced to wave 2
}

// ── Full wave cycle ─────────────────────────────────────────────────────────

void test_full_wave_cycle() {
    qe::game::WaveSystem ws;
    ws.start_game();

    // Wave 1: Intro -> Active -> Clear -> Wave 2 Intro
    ws.update(2.1f, 5);   // -> WaveActive
    ws.update(1.0f, 0);   // -> WaveClear
    ws.update(3.1f, 0);   // -> WaveIntro (wave 2)
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);
    ASSERT_TRUE(ws.current_wave() == 2);

    // Wave 2: Intro -> Active
    ws.update(2.1f, 10);  // -> WaveActive
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveActive);
    ASSERT_TRUE(ws.current_wave() == 2);
}

// ── Wave Config Scaling ─────────────────────────────────────────────────────

void test_generate_wave_1() {
    auto cfg = qe::game::WaveSystem::generate_wave(1);
    ASSERT_TRUE(cfg.wave_number == 1);
    ASSERT_TRUE(cfg.target_count == 6);
    ASSERT_NEAR(cfg.speed_multiplier, 1.0f, 1e-5f);
    ASSERT_NEAR(cfg.health_multiplier, 1.0f, 1e-5f);
    ASSERT_TRUE(cfg.bonus_points == 100);
    ASSERT_TRUE(cfg.time_bonus_seconds == 58);
}

void test_generate_wave_5() {
    auto cfg = qe::game::WaveSystem::generate_wave(5);
    ASSERT_TRUE(cfg.wave_number == 5);
    ASSERT_TRUE(cfg.target_count == 6 + 4 * 3);           // 18
    ASSERT_NEAR(cfg.speed_multiplier, 1.0f + 4 * 0.15f, 1e-5f);  // 1.6
    ASSERT_NEAR(cfg.health_multiplier, 1.0f + 4 * 0.2f, 1e-5f);  // 1.8
    ASSERT_TRUE(cfg.bonus_points == 500);
    ASSERT_TRUE(cfg.time_bonus_seconds == 50);             // 60 - 10
}

void test_generate_wave_10() {
    auto cfg = qe::game::WaveSystem::generate_wave(10);
    ASSERT_TRUE(cfg.wave_number == 10);
    ASSERT_TRUE(cfg.target_count == 6 + 9 * 3);           // 33
    ASSERT_NEAR(cfg.speed_multiplier, 1.0f + 9 * 0.15f, 1e-5f);  // 2.35
    ASSERT_NEAR(cfg.health_multiplier, 1.0f + 9 * 0.2f, 1e-5f);  // 2.8
    ASSERT_TRUE(cfg.bonus_points == 1000);
    ASSERT_TRUE(cfg.time_bonus_seconds == 40);             // 60 - 20
}

void test_generate_wave_high_clamps_time_bonus() {
    // Wave 25: 60 - 50 = 10, but clamped to 20
    auto cfg = qe::game::WaveSystem::generate_wave(25);
    ASSERT_TRUE(cfg.time_bonus_seconds == 20);
}

// ── wave_config reflects current wave ───────────────────────────────────────

void test_wave_config_updates_on_progression() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ASSERT_TRUE(ws.wave_config().wave_number == 1);
    ASSERT_TRUE(ws.wave_config().target_count == 6);

    // Progress to wave 2
    ws.update(2.1f, 5);   // -> WaveActive
    ws.update(0.1f, 0);   // -> WaveClear
    ws.update(3.1f, 0);   // -> WaveIntro (wave 2)
    ASSERT_TRUE(ws.wave_config().wave_number == 2);
    ASSERT_TRUE(ws.wave_config().target_count == 9);
}

// ── GameOver ────────────────────────────────────────────────────────────────

void test_game_over_from_wave_active() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ws.update(2.1f, 5);   // -> WaveActive
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveActive);

    ws.game_over();
    ASSERT_TRUE(ws.state() == qe::game::GameState::GameOver);
    ASSERT_TRUE(!ws.is_playing());
}

void test_game_over_from_wave_intro() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);

    ws.game_over();
    ASSERT_TRUE(ws.state() == qe::game::GameState::GameOver);
}

void test_return_to_menu_from_game_over() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ws.update(2.1f, 5);   // -> WaveActive
    ws.game_over();
    ws.return_to_menu();
    ASSERT_TRUE(ws.state() == qe::game::GameState::Menu);
}

// ── Total game time accumulates only while playing ──────────────────────────

void test_total_game_time_accumulates_while_playing() {
    qe::game::WaveSystem ws;

    // Time should not accumulate in Menu
    ws.update(5.0f, 0);
    ASSERT_NEAR(ws.total_game_time(), 0.0f, 1e-5f);

    ws.start_game();

    // Time accumulates during WaveIntro
    ws.update(1.0f, 0);
    ASSERT_NEAR(ws.total_game_time(), 1.0f, 1e-5f);

    // Time accumulates during WaveActive
    ws.update(1.1f, 5);  // -> WaveActive
    ws.update(2.0f, 5);
    ASSERT_NEAR(ws.total_game_time(), 4.1f, 1e-3f);

    // Stop accumulating after GameOver
    ws.game_over();
    float time_at_game_over = ws.total_game_time();
    ws.update(10.0f, 0);
    ASSERT_NEAR(ws.total_game_time(), time_at_game_over, 1e-5f);
}

// ── Best wave tracking ──────────────────────────────────────────────────────

void test_best_wave_tracking() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ASSERT_TRUE(ws.best_wave() == 1);

    // Progress through waves 1, 2, 3
    for (int w = 1; w <= 3; ++w) {
        ws.update(2.1f, 5);   // -> WaveActive
        ws.update(0.1f, 0);   // -> WaveClear
        ws.update(3.1f, 0);   // -> WaveIntro (next wave)
    }
    ASSERT_TRUE(ws.current_wave() == 4);
    ASSERT_TRUE(ws.best_wave() == 4);
}

// ── Spawn radius in config ──────────────────────────────────────────────────

void test_wave_config_spawn_radius() {
    auto cfg = qe::game::WaveSystem::generate_wave(1);
    ASSERT_NEAR(cfg.spawn_radius_min, 8.0f, 1e-5f);
    ASSERT_NEAR(cfg.spawn_radius_max, 20.0f, 1e-5f);
}

// ── Restart game from GameOver ──────────────────────────────────────────────

void test_restart_game_from_game_over() {
    qe::game::WaveSystem ws;
    ws.start_game();
    ws.update(2.1f, 5);   // -> WaveActive
    ws.game_over();
    ASSERT_TRUE(ws.state() == qe::game::GameState::GameOver);

    // Can start a new game from GameOver
    ws.start_game();
    ASSERT_TRUE(ws.state() == qe::game::GameState::WaveIntro);
    ASSERT_TRUE(ws.current_wave() == 1);
    ASSERT_NEAR(ws.total_game_time(), 0.0f, 1e-5f);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== QuatEngine Wave System Tests ===" << std::endl;

    std::cout << "\n--- Initial State ---" << std::endl;
    RUN_TEST(test_initial_state_is_menu);

    std::cout << "\n--- State Transitions ---" << std::endl;
    RUN_TEST(test_start_game_transitions_to_wave_intro);
    RUN_TEST(test_wave_intro_auto_transitions_after_2s);
    RUN_TEST(test_wave_active_to_clear_on_zero_targets);
    RUN_TEST(test_wave_clear_auto_transitions_after_3s);
    RUN_TEST(test_full_wave_cycle);

    std::cout << "\n--- Wave Config Scaling ---" << std::endl;
    RUN_TEST(test_generate_wave_1);
    RUN_TEST(test_generate_wave_5);
    RUN_TEST(test_generate_wave_10);
    RUN_TEST(test_generate_wave_high_clamps_time_bonus);
    RUN_TEST(test_wave_config_updates_on_progression);
    RUN_TEST(test_wave_config_spawn_radius);

    std::cout << "\n--- GameOver ---" << std::endl;
    RUN_TEST(test_game_over_from_wave_active);
    RUN_TEST(test_game_over_from_wave_intro);
    RUN_TEST(test_return_to_menu_from_game_over);
    RUN_TEST(test_restart_game_from_game_over);

    std::cout << "\n--- Game Time ---" << std::endl;
    RUN_TEST(test_total_game_time_accumulates_while_playing);
    RUN_TEST(test_best_wave_tracking);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total: " << total_assertions << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;

    return failed > 0 ? 1 : 0;
}
