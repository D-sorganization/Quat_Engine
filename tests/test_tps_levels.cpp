// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_tps_levels.cpp
 * @brief Tests for LevelSystem — all 12 levels, progression, objectives, state machine.
 *
 * Validates:
 *   - All 12 levels produce valid data (invariants pass)
 *   - Level progression and difficulty scaling
 *   - Objective types and completion conditions
 *   - Level state machine transitions
 *   - Weapon unlock progression
 *   - Score and par time bonuses
 */

#include "test_framework.h"

#include "../src/game/tps/LevelSystem.h"

#include <cmath>
#include <iostream>
#include <string>


using namespace qe::game::tps;

// ── All Levels Valid Tests ───────────────────────────────────────────────────

void test_all_levels_valid() {
    for (int i = 1; i <= TOTAL_LEVELS; ++i) {
        auto ld = make_level(i);
        ld.check_invariants();
        ASSERT_TRUE(ld.level_number == i);
        ASSERT_TRUE(!ld.name.empty());
        ASSERT_TRUE(!ld.description.empty());
        ASSERT_TRUE(!ld.briefing.empty());
        ASSERT_TRUE(ld.total_enemy_count > 0);
        ASSERT_TRUE(!ld.spawn_table.empty());
    }
}

void test_total_levels_constant() {
    ASSERT_TRUE(TOTAL_LEVELS == 12);
}

// ── Difficulty Scaling Tests ─────────────────────────────────────────────────

void test_difficulty_increases() {
    auto l1 = make_level(1);
    auto l12 = make_level(12);

    ASSERT_TRUE(l12.enemy_health_mult > l1.enemy_health_mult);
    ASSERT_TRUE(l12.enemy_damage_mult > l1.enemy_damage_mult);
    ASSERT_TRUE(l12.completion_score > l1.completion_score);
}

void test_progressive_score_rewards() {
    int prev_score = 0;
    for (int i = 1; i <= TOTAL_LEVELS; ++i) {
        auto ld = make_level(i);
        ASSERT_TRUE(ld.completion_score > prev_score);
        prev_score = ld.completion_score;
    }
}

// ── Specific Level Tests ─────────────────────────────────────────────────────

void test_level1_is_tutorial() {
    auto l = make_level(1);
    ASSERT_TRUE(l.name == "Ruined Outpost");
    ASSERT_TRUE(l.primary_objective.type == ObjectiveType::KillAll);
    // Tutorial: fewer enemies
    ASSERT_TRUE(l.total_enemy_count <= 15);
}

void test_level12_is_boss() {
    auto l = make_level(12);
    ASSERT_TRUE(l.name == "Ground Zero");
    ASSERT_TRUE(l.primary_objective.type == ObjectiveType::BossKill);
    ASSERT_TRUE(l.count_enemies_of_type(MutantType::Apex) == 1);
}

void test_level2_reach_exit() {
    auto l = make_level(2);
    ASSERT_TRUE(l.primary_objective.type == ObjectiveType::ReachExit);
}

void test_level7_defend_point() {
    auto l = make_level(7);
    ASSERT_TRUE(l.primary_objective.type == ObjectiveType::DefendPoint);
    ASSERT_TRUE(l.primary_objective.duration > 0.0f);
}

void test_level10_survive() {
    auto l = make_level(10);
    ASSERT_TRUE(l.primary_objective.type == ObjectiveType::Survive);
    ASSERT_TRUE(l.primary_objective.duration > 0.0f);
    // Horde level has many enemies
    ASSERT_TRUE(l.total_enemy_count > 30);
}

// ── Weapon Unlock Tests ──────────────────────────────────────────────────────

void test_weapon_unlocks() {
    int unlock_count = 0;
    for (int i = 1; i <= TOTAL_LEVELS; ++i) {
        auto ld = make_level(i);
        if (ld.unlocks_weapon) unlock_count++;
    }
    ASSERT_TRUE(unlock_count >= 5);  // At least 5 weapon unlocks
}

void test_level2_unlocks_shotgun() {
    auto l = make_level(2);
    ASSERT_TRUE(l.unlocks_weapon);
    ASSERT_TRUE(l.weapon_unlock == TPSWeaponType::CombatShotgun);
}

void test_level11_unlocks_tesla() {
    auto l = make_level(11);
    ASSERT_TRUE(l.unlocks_weapon);
    ASSERT_TRUE(l.weapon_unlock == TPSWeaponType::TeslaCoil);
}

// ── Environment Tests ────────────────────────────────────────────────────────

void test_each_level_has_unique_environment() {
    std::vector<EnvironmentType> envs;
    for (int i = 1; i <= TOTAL_LEVELS; ++i) {
        auto ld = make_level(i);
        envs.push_back(ld.environment.type);
    }
    // All 12 environments should be different types
    for (size_t i = 0; i < envs.size(); ++i) {
        for (size_t j = i + 1; j < envs.size(); ++j) {
            ASSERT_TRUE(envs[i] != envs[j]);
        }
    }
}

void test_underground_level_dim_lighting() {
    auto l = make_level(2);  // Ashen Subway
    ASSERT_TRUE(l.environment.type == EnvironmentType::Underground);
    ASSERT_TRUE(l.environment.light_intensity < 0.5f);
}

// ── Level Manager State Machine Tests ────────────────────────────────────────

void test_level_manager_initial_state() {
    LevelManager mgr;
    ASSERT_TRUE(mgr.current_level() == 1);
    ASSERT_TRUE(mgr.state() == LevelState::Briefing);
}

void test_level_start_transition() {
    LevelManager mgr;
    mgr.start_level();
    ASSERT_TRUE(mgr.state() == LevelState::Active);
}

void test_kill_all_completes_level() {
    LevelManager mgr;
    mgr.load_level(1);
    mgr.start_level();

    int total = mgr.level_data().total_enemy_count;
    for (int i = 0; i < total; ++i) {
        mgr.on_enemy_killed(50);
    }
    mgr.update(0.1f);
    ASSERT_TRUE(mgr.state() == LevelState::ObjectiveComplete);
    ASSERT_TRUE(mgr.is_level_complete(1));
}

void test_survive_objective_completes() {
    LevelManager mgr;
    mgr.load_level(10);  // Survive level
    mgr.start_level();

    float duration = mgr.level_data().primary_objective.duration;
    // Simulate surviving for the full duration
    for (float t = 0; t < duration + 1.0f; t += 1.0f) {
        mgr.update(1.0f);
    }
    ASSERT_TRUE(mgr.state() == LevelState::ObjectiveComplete);
}

void test_player_death_fails_level() {
    LevelManager mgr;
    mgr.load_level(1);
    mgr.start_level();
    mgr.on_player_death();
    ASSERT_TRUE(mgr.state() == LevelState::Failed);
}

void test_advance_to_next_level() {
    LevelManager mgr;
    mgr.load_level(1);
    mgr.start_level();

    // Complete level 1
    int total = mgr.level_data().total_enemy_count;
    for (int i = 0; i < total; ++i) mgr.on_enemy_killed(50);
    mgr.update(0.1f);

    mgr.advance_to_next();
    ASSERT_TRUE(mgr.current_level() == 2);
    ASSERT_TRUE(mgr.state() == LevelState::Briefing);
}

void test_retry_level() {
    LevelManager mgr;
    mgr.load_level(5);
    mgr.start_level();
    mgr.on_player_death();
    mgr.retry_level();
    ASSERT_TRUE(mgr.current_level() == 5);
    ASSERT_TRUE(mgr.state() == LevelState::Briefing);
    ASSERT_TRUE(mgr.enemies_alive() == mgr.level_data().total_enemy_count);
}

void test_final_level_victory() {
    LevelManager mgr;
    mgr.load_level(12);
    mgr.start_level();

    int total = mgr.level_data().total_enemy_count;
    for (int i = 0; i < total; ++i) mgr.on_enemy_killed(100);
    mgr.update(0.1f);

    ASSERT_TRUE(mgr.state() == LevelState::Victory);
    ASSERT_TRUE(mgr.is_final_level());
}

void test_objective_progress() {
    LevelManager mgr;
    mgr.load_level(1);
    mgr.start_level();

    ASSERT_NEAR(mgr.objective_progress(), 0.0f, 0.01f);
    int total = mgr.level_data().total_enemy_count;
    for (int i = 0; i < total / 2; ++i) mgr.on_enemy_killed(50);
    float progress = mgr.objective_progress();
    ASSERT_TRUE(progress > 0.3f && progress < 0.7f);
}

void test_par_time_bonus() {
    LevelManager mgr;
    mgr.load_level(1);
    mgr.start_level();

    // Complete quickly (under par)
    int total = mgr.level_data().total_enemy_count;
    for (int i = 0; i < total; ++i) mgr.on_enemy_killed(50);
    mgr.update(0.1f);

    // Score should include par time bonus
    int expected_min = mgr.level_data().completion_score;
    ASSERT_TRUE(mgr.total_score() >= expected_min);
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Level System Tests ===" << std::endl;

    std::cout << "\n--- Level Data ---" << std::endl;
    RUN_TEST(test_all_levels_valid);
    RUN_TEST(test_total_levels_constant);
    RUN_TEST(test_difficulty_increases);
    RUN_TEST(test_progressive_score_rewards);

    std::cout << "\n--- Specific Levels ---" << std::endl;
    RUN_TEST(test_level1_is_tutorial);
    RUN_TEST(test_level12_is_boss);
    RUN_TEST(test_level2_reach_exit);
    RUN_TEST(test_level7_defend_point);
    RUN_TEST(test_level10_survive);

    std::cout << "\n--- Weapon Unlocks ---" << std::endl;
    RUN_TEST(test_weapon_unlocks);
    RUN_TEST(test_level2_unlocks_shotgun);
    RUN_TEST(test_level11_unlocks_tesla);

    std::cout << "\n--- Environments ---" << std::endl;
    RUN_TEST(test_each_level_has_unique_environment);
    RUN_TEST(test_underground_level_dim_lighting);

    std::cout << "\n--- Level Manager ---" << std::endl;
    RUN_TEST(test_level_manager_initial_state);
    RUN_TEST(test_level_start_transition);
    RUN_TEST(test_kill_all_completes_level);
    RUN_TEST(test_survive_objective_completes);
    RUN_TEST(test_player_death_fails_level);
    RUN_TEST(test_advance_to_next_level);
    RUN_TEST(test_retry_level);
    RUN_TEST(test_final_level_victory);
    RUN_TEST(test_objective_progress);
    RUN_TEST(test_par_time_bonus);

    return TEST_REPORT();
}
