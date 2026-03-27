/**
 * @file test_tps_feedback.cpp
 * @brief Tests for DamageFeedback — screen shake, hit markers, damage numbers.
 *
 * Validates:
 *   - Hit events create damage numbers and hit markers
 *   - Screen shake intensity and decay
 *   - Damage direction indicators
 *   - Low health pulse
 *   - Feedback cleanup on clear
 */

#include "../src/game/tps/DamageFeedback.h"

#include <cmath>
#include <iostream>

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

using namespace qe::game::tps;
using namespace qe::math;

void test_hit_creates_damage_number() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(5, 1, 0), 25.0f, false, false);
    ASSERT_TRUE(fb.damage_numbers().size() == 1);
    ASSERT_NEAR(fb.damage_numbers()[0].value, 25.0f, 0.01f);
}

void test_critical_hit_shakes_screen() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(5, 1, 0), 50.0f, true, false);
    ASSERT_TRUE(fb.screen_shake().intensity > 0.0f);
}

void test_hit_marker_activates() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(5, 1, 0), 25.0f, false, false);
    ASSERT_TRUE(fb.has_active_hit_marker());
    ASSERT_TRUE(!fb.hit_marker().is_kill);
}

void test_kill_marker() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(5, 1, 0), 100.0f, false, true);
    ASSERT_TRUE(fb.hit_marker().is_kill);
    ASSERT_TRUE(fb.hit_marker().duration > 0.15f);  // Kill markers last longer
}

void test_screen_shake_decays() {
    DamageFeedbackSystem fb;
    fb.add_screen_shake(0.1f);
    ASSERT_TRUE(fb.screen_shake().intensity > 0.0f);

    // Decay over time
    for (int i = 0; i < 30; ++i) fb.update(0.016f, 1.0f);
    ASSERT_TRUE(fb.screen_shake().intensity < 0.1f);
}

void test_screen_shake_offset() {
    DamageFeedbackSystem fb;
    fb.add_screen_shake(0.2f);
    // Single frame update — intensity decays 8*0.016=0.128, so 0.2-0.128=0.072 remains
    fb.update(0.016f, 1.0f);
    ASSERT_TRUE(fb.screen_shake().intensity > 0.0f);
    // Verify offset computes from sin/cos oscillation
    Vec3 offset = fb.get_shake_offset();
    // Offset magnitude depends on timer*frequency — after 0.016s it's sin(0.4)≈0.389
    // So offset.x ≈ 0.389 * 0.072 ≈ 0.028, which is > 0
    ASSERT_TRUE(offset.length_squared() > 0.0f);
}

void test_player_hit_creates_indicator() {
    DamageFeedbackSystem fb;
    fb.on_player_hit(30.0f, Vec3(5, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, -1));
    ASSERT_TRUE(fb.damage_indicators().size() == 1);
    ASSERT_TRUE(fb.damage_indicators()[0].active());
}

void test_damage_indicator_fades() {
    DamageFeedbackSystem fb;
    fb.on_player_hit(30.0f, Vec3(5, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, -1));

    for (int i = 0; i < 100; ++i) fb.update(0.016f, 1.0f);
    ASSERT_TRUE(fb.damage_indicators().empty());  // Should have expired
}

void test_low_health_pulse() {
    DamageFeedbackSystem fb;
    fb.update(0.5f, 0.15f);  // 15% health
    ASSERT_TRUE(fb.low_health_pulse() > 0.0f);
}

void test_no_low_health_pulse_when_healthy() {
    DamageFeedbackSystem fb;
    fb.update(0.5f, 0.8f);  // 80% health
    ASSERT_NEAR(fb.low_health_pulse(), 0.0f, 0.01f);
}

void test_damage_numbers_expire() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(0, 0, 0), 10.0f, false, false);
    ASSERT_TRUE(fb.damage_numbers().size() == 1);

    // Advance past lifetime (1.2s)
    for (int i = 0; i < 100; ++i) fb.update(0.016f, 1.0f);
    ASSERT_TRUE(fb.damage_numbers().empty());
}

void test_hit_marker_expires() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(0, 0, 0), 10.0f, false, false);
    ASSERT_TRUE(fb.has_active_hit_marker());

    for (int i = 0; i < 20; ++i) fb.update(0.016f, 1.0f);
    ASSERT_TRUE(!fb.has_active_hit_marker());
}

void test_clear_resets_all() {
    DamageFeedbackSystem fb;
    fb.on_hit(Vec3(0, 0, 0), 50.0f, true, true);
    fb.on_player_hit(30.0f, Vec3(5, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, -1));
    fb.add_screen_shake(0.1f);

    fb.clear();
    ASSERT_TRUE(fb.damage_numbers().empty());
    ASSERT_TRUE(fb.damage_indicators().empty());
    ASSERT_TRUE(!fb.has_active_hit_marker());
    ASSERT_NEAR(fb.screen_shake().intensity, 0.0f, 0.01f);
}

void test_ground_slam_shake() {
    DamageFeedbackSystem fb;
    fb.on_ground_slam();
    ASSERT_TRUE(fb.screen_shake().intensity > 0.1f);
}

int main() {
    std::cout << "=== TPS Damage Feedback Tests ===" << std::endl;

    std::cout << "\n--- Hit Events ---" << std::endl;
    RUN_TEST(test_hit_creates_damage_number);
    RUN_TEST(test_critical_hit_shakes_screen);
    RUN_TEST(test_hit_marker_activates);
    RUN_TEST(test_kill_marker);

    std::cout << "\n--- Screen Shake ---" << std::endl;
    RUN_TEST(test_screen_shake_decays);
    RUN_TEST(test_screen_shake_offset);
    RUN_TEST(test_ground_slam_shake);

    std::cout << "\n--- Damage Indicators ---" << std::endl;
    RUN_TEST(test_player_hit_creates_indicator);
    RUN_TEST(test_damage_indicator_fades);

    std::cout << "\n--- Low Health ---" << std::endl;
    RUN_TEST(test_low_health_pulse);
    RUN_TEST(test_no_low_health_pulse_when_healthy);

    std::cout << "\n--- Expiration ---" << std::endl;
    RUN_TEST(test_damage_numbers_expire);
    RUN_TEST(test_hit_marker_expires);
    RUN_TEST(test_clear_resets_all);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total: " << total_assertions << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;

    return failed > 0 ? 1 : 0;
}
