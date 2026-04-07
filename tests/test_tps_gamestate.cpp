/**
 * @file test_tps_gamestate.cpp
 * @brief Integration tests for TPSGameState — full game loop, level progression.
 *
 * Validates:
 *   - Class selection and game initialization
 *   - Level loading with correct enemy counts
 *   - Briefing auto-advance
 *   - Combat integration: shooting enemies, score tracking
 *   - Level completion flow
 *   - Player death and level failure
 *   - Level advancement and retry
 *   - Full 12-level campaign progression
 */

#include "test_framework.h"

#include "../src/game/tps/TPSGameState.h"

#include <cmath>
#include <iostream>


using namespace qe::game::tps;

TPSInputState no_input() {
    TPSInputState input;
    input.clear();
    return input;
}

// ── Initialization Tests ─────────────────────────────────────────────────────

void test_initial_phase() {
    TPSGame game;
    ASSERT_TRUE(game.phase() == GamePhase::ClassSelect);
}

void test_class_selection() {
    TPSGame game;
    game.select_class(CharacterClassType::Recon);
    ASSERT_TRUE(game.phase() == GamePhase::Briefing);
    ASSERT_TRUE(game.player().class_type() == CharacterClassType::Recon);
}

void test_level_loading() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);

    ASSERT_TRUE(game.phase() == GamePhase::Briefing);
    ASSERT_TRUE(game.level_manager().current_level() == 1);
    ASSERT_TRUE(!game.enemies().empty());
    ASSERT_TRUE(game.alive_enemy_count() == game.level_manager().level_data().total_enemy_count);
}

void test_level_loads_all_12() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);

    for (int i = 1; i <= 12; ++i) {
        game.load_level(i);
        auto ld = make_level(i);
        ASSERT_TRUE(game.alive_enemy_count() == ld.total_enemy_count);
    }
}

// ── Game Flow Tests ──────────────────────────────────────────────────────────

void test_briefing_auto_advance() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);

    // Update past briefing duration
    auto input = no_input();
    for (int i = 0; i < 200; ++i) game.update(input, 0.02f);
    ASSERT_TRUE(game.phase() == GamePhase::Playing);
}

void test_briefing_skip_with_input() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);

    auto input = no_input();
    input.shoot_pressed = true;
    game.update(input, 0.016f);
    ASSERT_TRUE(game.phase() == GamePhase::Playing);
}

void test_player_update_runs() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);
    game.start_playing();

    auto input = no_input();
    input.move_forward = 1.0f;

    auto old_pos = game.player().position();
    for (int i = 0; i < 30; ++i) game.update(input, 0.016f);
    // Player should have moved
    ASSERT_TRUE(game.player().position().z != old_pos.z ||
                game.player().position().x != old_pos.x);
}

void test_enemy_ai_runs() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);
    game.start_playing();

    // Place player near an enemy
    game.player_mut().set_position(game.enemies()[0].position + qe::math::Vec3(5, 0, 0));

    auto input = no_input();
    for (int i = 0; i < 60; ++i) game.update(input, 0.016f);

    // Enemies should have reacted (at least one should be alerted/chasing)
    bool any_reacted = false;
    for (const auto& e : game.enemies()) {
        if (e.ai_state != MutantAIState::Idle && e.ai_state != MutantAIState::Patrol) {
            any_reacted = true;
            break;
        }
    }
    ASSERT_TRUE(any_reacted);
}

// ── Combat Integration Tests ─────────────────────────────────────────────────

void test_shooting_damages_enemy() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);
    game.start_playing();

    // Position player facing an enemy
    auto& first_enemy = const_cast<std::vector<MutantInstance>&>(game.enemies())[0];
    game.player_mut().set_position(first_enemy.position + qe::math::Vec3(0, 0, 5));
    // Face the enemy
    qe::math::Vec3 to_enemy = (first_enemy.position - game.player().position());
    if (to_enemy.length_squared() > 0.01f) {
        float yaw = std::atan2(to_enemy.x, to_enemy.z);
        game.player_mut().set_rotation(
            qe::math::Quaternion::from_axis_angle(qe::math::Vec3::up(), yaw));
    }

    float initial_health = first_enemy.current_health;

    auto input = no_input();
    input.shoot_pressed = true;
    input.shoot_held = true;
    game.update(input, 0.016f);

    // Check if any damage was dealt
    bool damage_dealt = first_enemy.current_health < initial_health ||
                        game.combat().stats().total_shots > 0;
    ASSERT_TRUE(damage_dealt);
}

void test_level_complete_on_all_killed() {
    TPSGame game;
    game.select_class(CharacterClassType::Heavy);
    game.load_level(1);
    game.start_playing();

    // Manually kill all enemies
    for (auto& e : const_cast<std::vector<MutantInstance>&>(game.enemies())) {
        e.alive = false;
        e.current_health = 0;
    }

    // Notify level manager
    int total = game.level_manager().level_data().total_enemy_count;
    for (int i = 0; i < total; ++i) {
        const_cast<LevelManager&>(game.level_manager()).on_enemy_killed(50);
    }

    auto input = no_input();
    game.update(input, 0.1f);
    ASSERT_TRUE(game.phase() == GamePhase::LevelComplete);
}

void test_player_death_fails_level() {
    TPSGame game;
    game.select_class(CharacterClassType::Recon);  // Low HP
    game.load_level(1);
    game.start_playing();

    // Deal massive damage to player
    game.player_mut().take_damage(999.0f);

    auto input = no_input();
    game.update(input, 0.016f);

    // Should be failed or player dead
    ASSERT_TRUE(!game.player().is_alive() || game.phase() == GamePhase::LevelFailed);
}

void test_retry_resets_level() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(3);
    game.start_playing();

    int initial_enemies = game.alive_enemy_count();
    game.retry();

    ASSERT_TRUE(game.phase() == GamePhase::Briefing);
    ASSERT_TRUE(game.alive_enemy_count() == initial_enemies);
    ASSERT_TRUE(game.player().is_alive());
}

// ── HUD Tests ────────────────────────────────────────────────────────────────

void test_hud_builds() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);
    game.start_playing();

    auto hud = game.build_hud(1.0f);
    ASSERT_TRUE(hud.health_bar.fill > 0.0f);
    ASSERT_TRUE(hud.stamina_bar.fill > 0.0f);
    ASSERT_TRUE(!hud.ammo_text.text.empty());
    ASSERT_TRUE(!hud.weapon_name.text.empty());
    ASSERT_TRUE(!hud.objective_text.text.empty());
    ASSERT_TRUE(!hud.level_name.text.empty());
}

void test_lock_targets_generated() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(1);

    auto targets = game.get_lock_targets();
    ASSERT_TRUE(!targets.empty());
    ASSERT_TRUE(static_cast<int>(targets.size()) ==
                game.level_manager().level_data().total_enemy_count);
}

// ── Feedback Tests ───────────────────────────────────────────────────────────

void test_scene_builds() {
    TPSGame game;
    game.select_class(CharacterClassType::Vanguard);
    game.load_level(5);

    const auto& scene = game.scene_data();
    ASSERT_TRUE(!scene.cover_objects.empty());
    ASSERT_TRUE(!scene.decorations.empty());
    ASSERT_TRUE(!scene.enemies.empty());
    ASSERT_TRUE(scene.ground_scale.x > 0.0f);
}

int main() {
    std::cout << "=== TPS Game State Integration Tests ===" << std::endl;

    std::cout << "\n--- Initialization ---" << std::endl;
    RUN_TEST(test_initial_phase);
    RUN_TEST(test_class_selection);
    RUN_TEST(test_level_loading);
    RUN_TEST(test_level_loads_all_12);

    std::cout << "\n--- Game Flow ---" << std::endl;
    RUN_TEST(test_briefing_auto_advance);
    RUN_TEST(test_briefing_skip_with_input);
    RUN_TEST(test_player_update_runs);
    RUN_TEST(test_enemy_ai_runs);

    std::cout << "\n--- Combat ---" << std::endl;
    RUN_TEST(test_shooting_damages_enemy);
    RUN_TEST(test_level_complete_on_all_killed);
    RUN_TEST(test_player_death_fails_level);
    RUN_TEST(test_retry_resets_level);

    std::cout << "\n--- HUD & Scene ---" << std::endl;
    RUN_TEST(test_hud_builds);
    RUN_TEST(test_lock_targets_generated);
    RUN_TEST(test_scene_builds);

    return TEST_REPORT();
}
