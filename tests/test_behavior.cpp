/**
 * @file test_behavior.cpp
 * @brief Tests for AI target behaviors driven by quaternion SLERP interpolation.
 *
 * Validates:
 *   - Orbit: correct radius, non-static position, tangent facing
 *   - Figure8: vertical oscillation above and below center
 *   - Zigzag: oscillation around center, SLERP-produced unit quaternion facing
 *   - Spiral: time-varying radius
 *   - Patrol: position near expected radius
 *   - All behaviors produce unit quaternions for rotation
 *   - Static behavior returns original position
 *   - Alert triggers dodge state
 */

#include "../src/game/TargetBehavior.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (!(expr)) {                                                       \
            std::cerr << "  FAIL: " << #expr << " (" << __FILE__ << ":"      \
                      << __LINE__ << ")" << std::endl;                       \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                           \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (std::abs((a) - (b)) > (eps)) {                                   \
            std::cerr << "  FAIL: " << #a << " == " << #b                    \
                      << " (got " << (a) << " vs " << (b)                    \
                      << ", eps=" << (eps) << ") at " << __FILE__             \
                      << ":" << __LINE__ << std::endl;                       \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define RUN_TEST(test_fn)                                                    \
    do {                                                                     \
        std::cout << "  " << #test_fn << "... ";                             \
        int before_fail = g_tests_failed;                                    \
        test_fn();                                                           \
        std::cout << (g_tests_failed == before_fail ? "OK" : "FAILED")       \
                  << std::endl;                                              \
    } while (0)

using namespace qe::math;
using namespace qe::game;

constexpr float PI = 3.14159265358979f;
constexpr float EPS = 1e-4f;

// ── Orbit Tests ─────────────────────────────────────────────────────────────

void test_orbit_radius() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    float radius = 5.0f;
    auto b = TargetBehavior::create_orbit(center, radius, 1.0f, 0.0f);

    // Check that position stays at correct radius from center at various times
    for (float t = 0.0f; t < 10.0f; t += 0.7f) {
        Vec3 pos = b.compute_position(t);
        float dist = (pos - center).length();
        ASSERT_FLOAT_EQ(dist, radius, 0.01f);
    }
}

void test_orbit_position_changes() {
    Vec3 center(1.0f, 2.0f, 3.0f);
    auto b = TargetBehavior::create_orbit(center, 5.0f, 1.0f, 0.0f);

    Vec3 pos_a = b.compute_position(0.0f);
    Vec3 pos_b = b.compute_position(1.0f);

    // Positions at different times must differ (not static)
    float delta = (pos_a - pos_b).length();
    ASSERT_TRUE(delta > 0.01f);
}

void test_orbit_facing_tangent() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    float radius = 5.0f;
    auto b = TargetBehavior::create_orbit(center, radius, 1.0f, 0.0f);

    // At t=0: position is at (radius, 0, 0), tangent should point along +Z or -Z
    // The facing quaternion is rotated 90 degrees ahead of position angle.
    // At t=0 the position angle is 0 (phase=0), so facing angle = PI/2.
    // Rotating (1,0,0) by PI/2 around Y gives (0,0,-1), so facing should
    // point in the -Z direction when we rotate the forward vector.

    float t = 0.0f;
    Vec3 pos = b.compute_position(t);
    Quaternion rot = b.compute_rotation(t);

    // The facing direction: rotate +X (or use the quaternion to determine
    // the direction the target "looks"). The key check is that the facing
    // is perpendicular to the radius vector (tangent to the circle).
    Vec3 radius_dir = (pos - center).normalized();
    Vec3 face_dir = rot.rotate(Vec3(0.0f, 0.0f, -1.0f)); // forward is -Z

    // Tangent should be perpendicular to radius in the XZ plane
    float dot = radius_dir.x * face_dir.x + radius_dir.z * face_dir.z;
    ASSERT_FLOAT_EQ(std::abs(dot), 0.0f, 0.15f);
}

// ── Figure-8 Tests ──────────────────────────────────────────────────────────

void test_figure8_vertical_oscillation() {
    Vec3 center(0.0f, 5.0f, 0.0f);
    auto b = TargetBehavior::create_figure8(center, 4.0f, 1.0f, 0.0f);

    // Sample many points and check that y goes both above and below center.y
    bool went_above = false;
    bool went_below = false;
    for (float t = 0.0f; t < 20.0f; t += 0.1f) {
        Vec3 pos = b.compute_position(t);
        if (pos.y > center.y + 0.1f) went_above = true;
        if (pos.y < center.y - 0.1f) went_below = true;
    }
    ASSERT_TRUE(went_above);
    ASSERT_TRUE(went_below);
}

// ── Zigzag Tests ────────────────────────────────────────────────────────────

void test_zigzag_oscillation() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = TargetBehavior::create_zigzag(center, 3.0f, 1.0f, 0.0f);

    // Position should oscillate: check it goes to positive and negative x
    bool positive_x = false;
    bool negative_x = false;
    for (float t = 0.0f; t < 20.0f; t += 0.1f) {
        Vec3 pos = b.compute_position(t);
        if (pos.x > 1.0f) positive_x = true;
        if (pos.x < -1.0f) negative_x = true;
    }
    ASSERT_TRUE(positive_x);
    ASSERT_TRUE(negative_x);
}

void test_zigzag_slerp_unit_quaternion() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = TargetBehavior::create_zigzag(center, 3.0f, 1.0f, 0.0f);

    // Facing quaternion from SLERP should always be a unit quaternion
    for (float t = 0.0f; t < 10.0f; t += 0.3f) {
        Quaternion q = b.compute_rotation(t);
        ASSERT_FLOAT_EQ(q.norm(), 1.0f, 0.001f);
    }
}

// ── Spiral Tests ────────────────────────────────────────────────────────────

void test_spiral_radius_varies() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = TargetBehavior::create_spiral(center, 6.0f, 1.0f, 0.0f);

    // Collect distances from center over time; they should not all be equal
    float min_dist = 1e9f;
    float max_dist = -1e9f;
    for (float t = 0.0f; t < 30.0f; t += 0.2f) {
        Vec3 pos = b.compute_position(t);
        // Distance in XZ plane (spiral has vertical component too)
        float dist_xz = std::sqrt(
            (pos.x - center.x) * (pos.x - center.x) +
            (pos.z - center.z) * (pos.z - center.z));
        if (dist_xz < min_dist) min_dist = dist_xz;
        if (dist_xz > max_dist) max_dist = dist_xz;
    }

    // The radius varies between radius*0.0 and radius*1.0 (due to the sin)
    // so min and max should be noticeably different
    ASSERT_TRUE(max_dist - min_dist > 1.0f);
}

// ── Patrol Tests ────────────────────────────────────────────────────────────

void test_patrol_stays_near_radius() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    float radius = 8.0f;
    auto b = TargetBehavior::create_patrol(center, radius, 1.0f, 0.0f);

    // Patrol moves along a pentagon at the given radius.
    // Position should always be within radius (on the polygon inscribed in the circle).
    for (float t = 0.0f; t < 20.0f; t += 0.25f) {
        Vec3 pos = b.compute_position(t);
        float dist = (pos - center).length();
        // On a regular pentagon, the minimum distance from center to an edge is
        // radius * cos(PI/5) ~ radius * 0.809. Allow some margin.
        ASSERT_TRUE(dist <= radius + 0.01f);
        ASSERT_TRUE(dist >= radius * 0.7f);
    }
}

void test_patrol_negative_time_wraps_consistently() {
    Vec3 center(0.0f, 0.0f, 0.0f);
    auto b = TargetBehavior::create_patrol(center, 8.0f, 1.0f, 0.0f);

    Vec3 pos_negative = b.compute_position(-0.25f);
    Vec3 pos_wrapped = b.compute_position(4.75f);
    ASSERT_TRUE(pos_negative.approx_equal(pos_wrapped, 1e-4f));

    Quaternion rot_negative = b.compute_rotation(-0.25f);
    Quaternion rot_wrapped = b.compute_rotation(4.75f);
    ASSERT_FLOAT_EQ(rot_negative.w, rot_wrapped.w, 1e-4f);
    ASSERT_FLOAT_EQ(rot_negative.x, rot_wrapped.x, 1e-4f);
    ASSERT_FLOAT_EQ(rot_negative.y, rot_wrapped.y, 1e-4f);
    ASSERT_FLOAT_EQ(rot_negative.z, rot_wrapped.z, 1e-4f);
}

void test_factory_methods_preserve_shared_defaults() {
    TargetBehavior orbit = TargetBehavior::create_orbit(Vec3(1.0f, 2.0f, 3.0f), 4.0f, 1.5f, 0.25f);
    TargetBehavior zigzag = TargetBehavior::create_zigzag(Vec3(-1.0f, 0.5f, 2.0f), 6.0f, 2.0f, -0.5f);
    TargetBehavior dodge = TargetBehavior::create_dodge(Vec3(0.0f, 0.0f, 0.0f), 3.0f);

    ASSERT_TRUE(orbit.base_orientation.approx_equal(Quaternion::identity(), EPS));
    ASSERT_TRUE(orbit.facing.approx_equal(Quaternion::identity(), EPS));
    ASSERT_TRUE(!orbit.is_alerted);
    ASSERT_FLOAT_EQ(orbit.alert_timer, 0.0f, EPS);

    ASSERT_FLOAT_EQ(zigzag.amplitude, 6.0f, EPS);
    ASSERT_TRUE(zigzag.dodge_offset.approx_equal(Vec3::zero(), EPS));

    ASSERT_FLOAT_EQ(dodge.radius, 3.0f, EPS);
    ASSERT_TRUE(dodge.dodge_offset.approx_equal(Vec3::zero(), EPS));
}

// ── Unit Quaternion Tests (All Behaviors) ───────────────────────────────────

void test_all_behaviors_unit_quaternion() {
    Vec3 center(0.0f, 0.0f, 0.0f);

    TargetBehavior behaviors[] = {
        TargetBehavior::create_orbit(center, 5.0f, 1.0f, 0.0f),
        TargetBehavior::create_figure8(center, 4.0f, 1.0f, 0.5f),
        TargetBehavior::create_zigzag(center, 3.0f, 1.0f, 0.0f),
        TargetBehavior::create_spiral(center, 6.0f, 1.0f, 0.0f),
        TargetBehavior::create_patrol(center, 8.0f, 1.0f, 0.0f),
    };

    for (auto& b : behaviors) {
        for (float t = 0.0f; t < 10.0f; t += 1.3f) {
            Quaternion q = b.compute_rotation(t);
            ASSERT_FLOAT_EQ(q.norm(), 1.0f, 0.01f);
        }
    }
}

// ── Static Behavior ─────────────────────────────────────────────────────────

void test_static_returns_center() {
    Vec3 center(3.0f, 7.0f, -2.0f);
    TargetBehavior b;
    b.type = BehaviorType::Static;
    b.center = center;

    Vec3 pos0 = b.compute_position(0.0f);
    Vec3 pos1 = b.compute_position(5.0f);
    Vec3 pos2 = b.compute_position(100.0f);

    ASSERT_TRUE(pos0.approx_equal(center, EPS));
    ASSERT_TRUE(pos1.approx_equal(center, EPS));
    ASSERT_TRUE(pos2.approx_equal(center, EPS));
}

// ── Dodge / Alert Tests ─────────────────────────────────────────────────────

void test_alert_triggers_dodge() {
    auto b = TargetBehavior::create_dodge(Vec3(0.0f, 0.0f, 0.0f), 1.0f);

    ASSERT_TRUE(!b.is_alerted);
    ASSERT_FLOAT_EQ(b.alert_timer, 0.0f, EPS);

    b.alert();

    ASSERT_TRUE(b.is_alerted);
    ASSERT_TRUE(b.alert_timer > 0.0f);

    // Position should have moved away from center
    Vec3 pos = b.compute_position(0.0f);
    float dist = pos.length();
    ASSERT_TRUE(dist > 0.1f);
}

void test_dodge_alert_expires() {
    auto b = TargetBehavior::create_dodge(Vec3(0.0f, 0.0f, 0.0f), 1.0f);

    b.alert();
    ASSERT_TRUE(b.is_alerted);

    // Update past the alert duration
    b.update(2.0f);
    ASSERT_TRUE(!b.is_alerted);
    ASSERT_FLOAT_EQ(b.alert_timer, 0.0f, EPS);

    // Position should return to center
    Vec3 pos = b.compute_position(0.0f);
    ASSERT_TRUE(pos.approx_equal(Vec3::zero(), EPS));
}

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== QuatEngine Behavior Tests ===" << std::endl;

    std::cout << "\n--- Orbit ---" << std::endl;
    RUN_TEST(test_orbit_radius);
    RUN_TEST(test_orbit_position_changes);
    RUN_TEST(test_orbit_facing_tangent);

    std::cout << "\n--- Figure-8 ---" << std::endl;
    RUN_TEST(test_figure8_vertical_oscillation);

    std::cout << "\n--- Zigzag ---" << std::endl;
    RUN_TEST(test_zigzag_oscillation);
    RUN_TEST(test_zigzag_slerp_unit_quaternion);

    std::cout << "\n--- Spiral ---" << std::endl;
    RUN_TEST(test_spiral_radius_varies);

    std::cout << "\n--- Patrol ---" << std::endl;
    RUN_TEST(test_patrol_stays_near_radius);
    RUN_TEST(test_patrol_negative_time_wraps_consistently);

    std::cout << "\n--- All Behaviors: Unit Quaternion ---" << std::endl;
    RUN_TEST(test_all_behaviors_unit_quaternion);

    std::cout << "\n--- Static ---" << std::endl;
    RUN_TEST(test_static_returns_center);

    std::cout << "\n--- Dodge / Alert ---" << std::endl;
    RUN_TEST(test_alert_triggers_dodge);
    RUN_TEST(test_dodge_alert_expires);

    std::cout << "\n--- Factory Defaults ---" << std::endl;
    RUN_TEST(test_factory_methods_preserve_shared_defaults);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total assertions: " << g_tests_run << std::endl;
    std::cout << "  Passed: " << g_tests_passed << std::endl;
    std::cout << "  Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
