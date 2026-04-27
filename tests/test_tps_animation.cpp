// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_tps_animation.cpp
 * @brief Tests for AnimationSystem — state machine, blending, root motion.
 *
 * Validates:
 *   - Animation clip durations and properties
 *   - State transitions (locomotion, combat, aerial)
 *   - Upper/lower body layering
 *   - Blend weight progression
 *   - Root motion extraction for melee
 *   - Normalized time tracking
 */

#include "test_framework.h"

#include "../src/game/tps/AnimationSystem.h"

#include <cmath>
#include <iostream>


using namespace qe::game::tps;

// ── Clip Config Tests ────────────────────────────────────────────────────────

void test_idle_clip_looping() {
    auto clip = make_anim_clip(AnimState::Idle);
    ASSERT_TRUE(clip.looping);
    ASSERT_TRUE(clip.duration > 0.0f);
    ASSERT_TRUE(!clip.upper_body_only);
}

void test_melee_clips_not_looping() {
    auto l1 = make_anim_clip(AnimState::MeleeLight1);
    auto l2 = make_anim_clip(AnimState::MeleeLight2);
    auto heavy = make_anim_clip(AnimState::MeleeHeavy);

    ASSERT_TRUE(!l1.looping);
    ASSERT_TRUE(!l2.looping);
    ASSERT_TRUE(!heavy.looping);
}

void test_shooting_is_upper_body() {
    auto clip = make_anim_clip(AnimState::Shooting);
    ASSERT_TRUE(clip.upper_body_only);
    ASSERT_TRUE(clip.looping);
}

void test_reloading_is_upper_body() {
    auto clip = make_anim_clip(AnimState::Reloading);
    ASSERT_TRUE(clip.upper_body_only);
    ASSERT_TRUE(!clip.looping);
}

void test_death_clip_long() {
    auto clip = make_anim_clip(AnimState::Death);
    ASSERT_TRUE(!clip.looping);
    ASSERT_TRUE(clip.duration > 1.0f);
}

void test_locomotion_clips_loop() {
    ASSERT_TRUE(make_anim_clip(AnimState::Walk).looping);
    ASSERT_TRUE(make_anim_clip(AnimState::Run).looping);
    ASSERT_TRUE(make_anim_clip(AnimState::Sprint).looping);
}

// ── Controller State Tests ───────────────────────────────────────────────────

void test_initial_state_idle() {
    AnimationController ctrl;
    ASSERT_TRUE(ctrl.current_lower_state() == AnimState::Idle);
}

void test_set_locomotion() {
    AnimationController ctrl;
    ctrl.set_locomotion(AnimState::Run, AnimTransition::smooth());
    ctrl.update(0.3f);  // Past blend
    ASSERT_TRUE(ctrl.current_lower_state() == AnimState::Run);
}

void test_set_full_body_overrides() {
    AnimationController ctrl;
    ctrl.set_locomotion(AnimState::Run, AnimTransition::instant());
    ctrl.set_full_body(AnimState::MeleeLight1, AnimTransition::quick());
    ASSERT_TRUE(ctrl.current_lower_state() == AnimState::MeleeLight1);
    ASSERT_TRUE(ctrl.current_upper_state() == AnimState::MeleeLight1);
}

void test_upper_body_overlay() {
    AnimationController ctrl;
    ctrl.set_locomotion(AnimState::Run, AnimTransition::instant());
    ctrl.set_upper_body(AnimState::Shooting, AnimTransition::instant());

    ASSERT_TRUE(ctrl.current_lower_state() == AnimState::Run);
    ASSERT_TRUE(ctrl.current_upper_state() == AnimState::Shooting);
}

void test_clear_upper_body() {
    AnimationController ctrl;
    ctrl.set_locomotion(AnimState::Run, AnimTransition::instant());
    ctrl.set_upper_body(AnimState::Shooting, AnimTransition::instant());
    ctrl.clear_upper_body();

    ASSERT_TRUE(ctrl.current_upper_state() == AnimState::Run);
}

// ── Blend Tests ──────────────────────────────────────────────────────────────

void test_instant_transition() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::DodgeRoll, AnimTransition::instant());
    ASSERT_NEAR(ctrl.blend_weight(), 1.0f, 0.01f);
    ASSERT_TRUE(!ctrl.is_blending());
}

void test_smooth_transition_blends() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::DodgeRoll, AnimTransition::smooth());
    ASSERT_TRUE(ctrl.is_blending());

    ctrl.update(0.1f);  // Halfway through 0.2s blend
    ASSERT_TRUE(ctrl.blend_weight() > 0.0f);
    ASSERT_TRUE(ctrl.blend_weight() < 1.0f);

    ctrl.update(0.15f);  // Past blend duration
    ASSERT_NEAR(ctrl.blend_weight(), 1.0f, 0.01f);
}

// ── Root Motion Tests ────────────────────────────────────────────────────────

void test_melee_generates_root_motion() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::MeleeLight1, AnimTransition::instant());
    ctrl.update(0.016f);

    auto motion = ctrl.consume_root_motion();
    // Melee should generate forward motion (negative Z)
    ASSERT_TRUE(motion.z < 0.0f);
}

void test_idle_no_root_motion() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::Idle, AnimTransition::instant());
    ctrl.update(0.016f);

    auto motion = ctrl.consume_root_motion();
    ASSERT_NEAR(motion.x, 0.0f, 0.01f);
    ASSERT_NEAR(motion.y, 0.0f, 0.01f);
    ASSERT_NEAR(motion.z, 0.0f, 0.01f);
}

void test_consume_root_motion_clears() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::MeleeLight1, AnimTransition::instant());
    ctrl.update(0.016f);

    ctrl.consume_root_motion();  // First call gets motion
    auto second = ctrl.consume_root_motion();  // Second should be zero
    ASSERT_NEAR(second.z, 0.0f, 0.01f);
}

// ── Normalized Time Tests ────────────────────────────────────────────────────

void test_normalized_time_progresses() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::MeleeLight1, AnimTransition::instant());

    ASSERT_NEAR(ctrl.normalized_time(), 0.0f, 0.01f);
    ctrl.update(0.1f);
    ASSERT_TRUE(ctrl.normalized_time() > 0.0f);
}

void test_full_body_finished() {
    AnimationController ctrl;
    auto clip = make_anim_clip(AnimState::MeleeLight1);
    ctrl.set_full_body(AnimState::MeleeLight1, AnimTransition::instant());

    // Run past the clip duration
    ctrl.update(clip.duration + 0.1f);
    ASSERT_TRUE(ctrl.is_full_body_finished());
}

void test_dodge_generates_root_motion() {
    AnimationController ctrl;
    ctrl.set_full_body(AnimState::DodgeRoll, AnimTransition::instant());
    ctrl.update(0.016f);

    auto motion = ctrl.consume_root_motion();
    ASSERT_TRUE(motion.z < 0.0f);  // Forward motion during dodge
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== TPS Animation System Tests ===" << std::endl;

    std::cout << "\n--- Clip Configs ---" << std::endl;
    RUN_TEST(test_idle_clip_looping);
    RUN_TEST(test_melee_clips_not_looping);
    RUN_TEST(test_shooting_is_upper_body);
    RUN_TEST(test_reloading_is_upper_body);
    RUN_TEST(test_death_clip_long);
    RUN_TEST(test_locomotion_clips_loop);

    std::cout << "\n--- Controller State ---" << std::endl;
    RUN_TEST(test_initial_state_idle);
    RUN_TEST(test_set_locomotion);
    RUN_TEST(test_set_full_body_overrides);
    RUN_TEST(test_upper_body_overlay);
    RUN_TEST(test_clear_upper_body);

    std::cout << "\n--- Blending ---" << std::endl;
    RUN_TEST(test_instant_transition);
    RUN_TEST(test_smooth_transition_blends);

    std::cout << "\n--- Root Motion ---" << std::endl;
    RUN_TEST(test_melee_generates_root_motion);
    RUN_TEST(test_idle_no_root_motion);
    RUN_TEST(test_consume_root_motion_clears);

    std::cout << "\n--- Normalized Time ---" << std::endl;
    RUN_TEST(test_normalized_time_progresses);
    RUN_TEST(test_full_body_finished);
    RUN_TEST(test_dodge_generates_root_motion);

    return TEST_REPORT();
}
