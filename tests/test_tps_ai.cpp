// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_tps_ai.cpp
 * @brief Tests for MutantAI — patrol, detection, chasing, attacking, retreat.
 *
 * Validates:
 *   - Idle mutants patrol near their origin
 *   - Detection range triggers alert state
 *   - Chasing moves toward player
 *   - Attack triggers within attack range
 *   - Type-specific behaviors (Spitter retreat, Stalker circle, etc.)
 *   - Staggered mutants don't act
 */

#include "test_framework.h"

#include "../src/game/tps/MutantAI.h"
#include "../src/game/tps/MutantTypes.h"

#include <cmath>
#include <iostream>
#include <vector>


using namespace qe::game::tps;
using namespace qe::math;

void test_idle_patrol() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(10, 0, 10), 1);
    m.ai_state = MutantAIState::Idle;
    qe::core::Rng rng(42);

    // Player far away -> should stay idle or patrol
    auto decision = update_mutant_ai(m, Vec3(100, 0, 100), true, 0.016f, rng);
    ASSERT_TRUE(m.ai_state == MutantAIState::Patrol);
    ASSERT_TRUE(!decision.wants_to_attack);
}

void test_detection_triggers_alert() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Idle;
    qe::core::Rng rng(42);

    // Player within detection range
    Vec3 player_pos(5, 0, 0);  // Within 20 unit detection range
    update_mutant_ai(m, player_pos, true, 0.016f, rng);
    ASSERT_TRUE(m.ai_state == MutantAIState::Alerted);
}

void test_alert_transitions_to_chase() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Alerted;
    m.state_timer = 0.0f;
    qe::core::Rng rng(42);

    Vec3 player_pos(5, 0, 0);
    // Update past alert duration (0.5s)
    for (int i = 0; i < 40; ++i) {
        update_mutant_ai(m, player_pos, true, 0.016f, rng);
    }
    ASSERT_TRUE(m.ai_state == MutantAIState::Chasing);
}

void test_chasing_moves_toward_player() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Chasing;
    qe::core::Rng rng(42);

    Vec3 player_pos(10, 0, 0);
    auto decision = update_mutant_ai(m, player_pos, true, 0.016f, rng);
    ASSERT_TRUE(decision.move_direction.x > 0.0f);  // Moving toward player
    ASSERT_TRUE(!decision.wants_to_attack);  // Not in attack range

    // Apply movement and verify position change
    Vec3 old_pos = m.position;
    apply_ai_movement(m, decision, 0.5f);
    ASSERT_TRUE(m.position.x > old_pos.x);  // Moved toward player
}

void test_attack_triggers_in_range() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Chasing;
    m.attack_cooldown = 0.0f;
    qe::core::Rng rng(42);

    Vec3 player_pos(1.5f, 0, 0);  // Within 2.0 attack range
    update_mutant_ai(m, player_pos, true, 0.016f, rng);
    ASSERT_TRUE(m.ai_state == MutantAIState::Attacking);
}

void test_attack_deals_damage() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Attacking;
    m.state_timer = m.config.primary_attack.windup_time + 0.01f;

    AIDecision decision;
    decision.wants_to_attack = true;
    decision.using_primary = true;

    Vec3 player_pos(1.0f, 0, 0);
    float dmg = process_mutant_attack(m, decision, player_pos, false);
    ASSERT_TRUE(dmg > 0.0f);
}

void test_iframes_block_damage() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    AIDecision decision;
    decision.wants_to_attack = true;
    decision.using_primary = true;

    float dmg = process_mutant_attack(m, decision, Vec3(1, 0, 0), true);
    ASSERT_TRUE(dmg == 0.0f);  // i-frames block damage
}

void test_staggered_no_action() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Staggered;
    m.stagger_timer = 0.5f;
    qe::core::Rng rng(42);

    auto decision = update_mutant_ai(m, Vec3(1, 0, 0), true, 0.016f, rng);
    ASSERT_TRUE(!decision.wants_to_attack);
    ASSERT_TRUE(decision.move_direction.length_squared() < 0.01f);
}

void test_spitter_maintains_distance() {
    auto m = spawn_mutant(MutantType::Spitter, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Chasing;
    m.attack_cooldown = 10.0f;  // Prevent attack transition
    qe::core::Rng rng(42);

    // Player within spitter ideal range * 0.5 -> should move away
    // Spitter attack_range is 25, ideal = 25*0.7=17.5, half-ideal = 8.75
    Vec3 close_player(5, 0, 0);  // Distance 5 < 8.75
    auto decision = update_mutant_ai(m, close_player, true, 0.016f, rng);
    ASSERT_TRUE(decision.move_direction.x < 0.0f);  // Moving away from player
}

void test_dead_player_makes_idle() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Chasing;
    qe::core::Rng rng(42);

    update_mutant_ai(m, Vec3(5, 0, 0), false, 0.016f, rng);
    ASSERT_TRUE(m.ai_state == MutantAIState::Idle);
}

void test_out_of_range_returns_idle() {
    auto m = spawn_mutant(MutantType::Grunt, Vec3(0, 0, 0), 1);
    m.ai_state = MutantAIState::Chasing;
    qe::core::Rng rng(42);

    // Player very far
    update_mutant_ai(m, Vec3(100, 0, 100), true, 0.016f, rng);
    ASSERT_TRUE(m.ai_state == MutantAIState::Idle);
}

int main() {
    std::cout << "=== TPS Mutant AI Tests ===" << std::endl;

    std::cout << "\n--- Patrol & Detection ---" << std::endl;
    RUN_TEST(test_idle_patrol);
    RUN_TEST(test_detection_triggers_alert);
    RUN_TEST(test_alert_transitions_to_chase);

    std::cout << "\n--- Chasing & Movement ---" << std::endl;
    RUN_TEST(test_chasing_moves_toward_player);
    RUN_TEST(test_out_of_range_returns_idle);

    std::cout << "\n--- Attacking ---" << std::endl;
    RUN_TEST(test_attack_triggers_in_range);
    RUN_TEST(test_attack_deals_damage);
    RUN_TEST(test_iframes_block_damage);
    RUN_TEST(test_staggered_no_action);

    std::cout << "\n--- Type-Specific ---" << std::endl;
    RUN_TEST(test_spitter_maintains_distance);
    RUN_TEST(test_dead_player_makes_idle);

    return TEST_REPORT();
}
