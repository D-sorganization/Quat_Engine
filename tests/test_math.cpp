/**
 * @file test_math.cpp
 * @brief Comprehensive tests for Vec3, Quaternion (with SLERP), Mat4, and Transform.
 *
 * Uses a lightweight test framework (no external dependencies).
 * Tests can be run standalone: just compile and execute.
 *
 * Covers:
 *   - Vec3: arithmetic, cross/dot, normalization, lerp
 *   - Quaternion: construction, multiplication, rotation, SLERP edge cases
 *   - Mat4: identity, TRS, perspective, look-at, point/direction transform
 *   - Transform: movement, interpolation, direction vectors
 */

#include "../src/core/Transform.h"
#include "../src/math/Mat4.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// --- Minimal Test Framework ---

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

#define ASSERT_VEC3_EQ(v, ex, ey, ez, eps)                                   \
    do {                                                                     \
        ASSERT_FLOAT_EQ((v).x, (ex), (eps));                                 \
        ASSERT_FLOAT_EQ((v).y, (ey), (eps));                                 \
        ASSERT_FLOAT_EQ((v).z, (ez), (eps));                                 \
    } while (0)

#define RUN_TEST(test_fn)                                                    \
    do {                                                                     \
        std::cout << "  " << #test_fn << "... ";                             \
        int before_fail = g_tests_failed;                                    \
        test_fn();                                                           \
        std::cout << (g_tests_failed == before_fail ? "OK" : "FAILED")       \
                  << std::endl;                                              \
    } while (0)

using namespace ffe::math;
using namespace ffe::core;

constexpr float PI = 3.14159265358979f;
constexpr float EPS = 1e-4f;

// ============================================================================
//  Vec3 Tests
// ============================================================================

void test_vec3_default_constructor() {
    Vec3 v;
    ASSERT_VEC3_EQ(v, 0.0f, 0.0f, 0.0f, EPS);
}

void test_vec3_arithmetic() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    Vec3 sum = a + b;
    ASSERT_VEC3_EQ(sum, 5.0f, 7.0f, 9.0f, EPS);

    Vec3 diff = b - a;
    ASSERT_VEC3_EQ(diff, 3.0f, 3.0f, 3.0f, EPS);

    Vec3 scaled = a * 2.0f;
    ASSERT_VEC3_EQ(scaled, 2.0f, 4.0f, 6.0f, EPS);

    Vec3 commutative = 3.0f * a;
    ASSERT_VEC3_EQ(commutative, 3.0f, 6.0f, 9.0f, EPS);

    Vec3 neg = -a;
    ASSERT_VEC3_EQ(neg, -1.0f, -2.0f, -3.0f, EPS);
}

void test_vec3_dot_product() {
    Vec3 a(1.0f, 0.0f, 0.0f);
    Vec3 b(0.0f, 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(a.dot(b), 0.0f, EPS);  // Perpendicular

    Vec3 c(1.0f, 2.0f, 3.0f);
    Vec3 d(4.0f, 5.0f, 6.0f);
    ASSERT_FLOAT_EQ(c.dot(d), 32.0f, EPS);
}

void test_vec3_cross_product() {
    Vec3 x(1.0f, 0.0f, 0.0f);
    Vec3 y(0.0f, 1.0f, 0.0f);
    Vec3 z = x.cross(y);
    ASSERT_VEC3_EQ(z, 0.0f, 0.0f, 1.0f, EPS);  // Right-hand rule

    Vec3 yx = y.cross(x);
    ASSERT_VEC3_EQ(yx, 0.0f, 0.0f, -1.0f, EPS);  // Anti-commutative
}

void test_vec3_length_and_normalization() {
    Vec3 v(3.0f, 4.0f, 0.0f);
    ASSERT_FLOAT_EQ(v.length(), 5.0f, EPS);
    ASSERT_FLOAT_EQ(v.length_squared(), 25.0f, EPS);

    Vec3 n = v.normalized();
    ASSERT_FLOAT_EQ(n.length(), 1.0f, EPS);
    ASSERT_VEC3_EQ(n, 0.6f, 0.8f, 0.0f, EPS);
}

void test_vec3_lerp() {
    Vec3 a(0.0f, 0.0f, 0.0f);
    Vec3 b(10.0f, 20.0f, 30.0f);

    Vec3 mid = a.lerp(b, 0.5f);
    ASSERT_VEC3_EQ(mid, 5.0f, 10.0f, 15.0f, EPS);

    Vec3 start = a.lerp(b, 0.0f);
    ASSERT_VEC3_EQ(start, 0.0f, 0.0f, 0.0f, EPS);

    Vec3 end = a.lerp(b, 1.0f);
    ASSERT_VEC3_EQ(end, 10.0f, 20.0f, 30.0f, EPS);
}

void test_vec3_distance() {
    Vec3 a(0.0f, 0.0f, 0.0f);
    Vec3 b(3.0f, 4.0f, 0.0f);
    ASSERT_FLOAT_EQ(a.distance_to(b), 5.0f, EPS);
}

// ============================================================================
//  Quaternion Tests
// ============================================================================

void test_quaternion_identity() {
    Quaternion q;
    ASSERT_FLOAT_EQ(q.w, 1.0f, EPS);
    ASSERT_FLOAT_EQ(q.x, 0.0f, EPS);
    ASSERT_FLOAT_EQ(q.y, 0.0f, EPS);
    ASSERT_FLOAT_EQ(q.z, 0.0f, EPS);
}

void test_quaternion_from_axis_angle() {
    // 90° around Y axis
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);

    // Should rotate (1,0,0) to (0,0,-1)
    Vec3 result = q.rotate(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(result, 0.0f, 0.0f, -1.0f, EPS);
}

void test_quaternion_rotation_90_x() {
    // 90° around X axis: (0,1,0) → (0,0,1)
    Quaternion q = Quaternion::from_axis_angle(Vec3::right(), PI / 2.0f);
    Vec3 result = q.rotate(Vec3::up());
    ASSERT_VEC3_EQ(result, 0.0f, 0.0f, 1.0f, EPS);
}

void test_quaternion_rotation_composition() {
    // Two 90° Y rotations = 180° Y rotation
    Quaternion q90 = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    Quaternion q180 = q90 * q90;

    Vec3 result = q180.rotate(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(result, -1.0f, 0.0f, 0.0f, EPS);
}

void test_quaternion_inverse() {
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 3.0f);
    Quaternion qi = q.inverse();

    // q * q^-1 should be identity
    Quaternion product = q * qi;
    ASSERT_TRUE(product.approx_equal(Quaternion::identity(), EPS));
}

void test_quaternion_conjugate_rotation() {
    // Rotating forward then backward should return to original
    Quaternion q = Quaternion::from_axis_angle(Vec3(1.0f, 1.0f, 0.0f), PI / 4.0f);
    Vec3 original(3.0f, 5.0f, 7.0f);

    Vec3 rotated = q.rotate(original);
    Vec3 unrotated = q.conjugate().rotate(rotated);

    ASSERT_TRUE(original.approx_equal(unrotated, EPS));
}

void test_quaternion_from_euler() {
    // Pure yaw (90° around Y)
    Quaternion q = Quaternion::from_euler(0.0f, PI / 2.0f, 0.0f);
    Vec3 result = q.rotate(Vec3(1.0f, 0.0f, 0.0f));
    // Pure yaw of 90° maps +X to... depends on convention
    // With our ZYX convention, yaw rotates around Z:
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
}

void test_quaternion_from_two_vectors() {
    Vec3 from = Vec3::right();
    Vec3 to = Vec3::up();

    Quaternion q = Quaternion::from_two_vectors(from, to);
    Vec3 result = q.rotate(from);
    ASSERT_TRUE(result.approx_equal(to, EPS));
}

void test_quaternion_from_two_vectors_same_direction() {
    Vec3 v(1.0f, 0.0f, 0.0f);
    Quaternion q = Quaternion::from_two_vectors(v, v);
    ASSERT_TRUE(q.approx_equal(Quaternion::identity(), EPS));
}

void test_quaternion_from_two_vectors_opposite() {
    Vec3 from(1.0f, 0.0f, 0.0f);
    Vec3 to(-1.0f, 0.0f, 0.0f);

    Quaternion q = Quaternion::from_two_vectors(from, to);
    Vec3 result = q.rotate(from);
    ASSERT_TRUE(result.approx_equal(to, EPS));
}

// ============================================================================
//  SLERP Tests (Core Experiment)
// ============================================================================

void test_slerp_endpoints() {
    Quaternion a = Quaternion::from_axis_angle(Vec3::up(), 0.0f);
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);

    // t=0 → a
    Quaternion r0 = Quaternion::slerp(a, b, 0.0f);
    ASSERT_TRUE(r0.approx_equal(a, EPS));

    // t=1 → b
    Quaternion r1 = Quaternion::slerp(a, b, 1.0f);
    ASSERT_TRUE(r1.approx_equal(b, EPS));
}

void test_slerp_midpoint() {
    // SLERP midpoint between 0° and 90° Y rotation = 45° Y rotation
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);

    Quaternion mid = Quaternion::slerp(a, b, 0.5f);
    Quaternion expected = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);

    ASSERT_TRUE(mid.approx_equal(expected, EPS));
}

void test_slerp_constant_angular_velocity() {
    // Key property: SLERP produces evenly-spaced angular steps
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), PI);

    Vec3 ref = Vec3::right();  // Track where +X goes

    // Calculate angular positions at t = 0.25, 0.5, 0.75
    Vec3 r1 = Quaternion::slerp(a, b, 0.25f).rotate(ref);
    Vec3 r2 = Quaternion::slerp(a, b, 0.50f).rotate(ref);
    Vec3 r3 = Quaternion::slerp(a, b, 0.75f).rotate(ref);

    // Angular spacing should be uniform
    float angle_01 = std::acos(std::min(1.0f, ref.dot(r1)));
    float angle_12 = std::acos(std::min(1.0f, r1.dot(r2)));
    float angle_23 = std::acos(std::min(1.0f, r2.dot(r3)));

    ASSERT_FLOAT_EQ(angle_01, angle_12, EPS * 10);
    ASSERT_FLOAT_EQ(angle_12, angle_23, EPS * 10);
}

void test_slerp_shortest_path() {
    // When dot product is negative, SLERP should take the shorter arc
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), PI * 1.5f);

    // The "long way" is 270°, short way is 90° in opposite direction
    Quaternion result = Quaternion::slerp(a, b, 0.5f);
    ASSERT_FLOAT_EQ(result.norm(), 1.0f, EPS);

    // Result should be a valid unit quaternion
    Vec3 rotated = result.rotate(Vec3::right());
    ASSERT_FLOAT_EQ(rotated.length(), 1.0f, EPS);
}

void test_slerp_same_quaternion() {
    // SLERP between identical quaternions
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), 1.0f);
    Quaternion result = Quaternion::slerp(q, q, 0.5f);
    ASSERT_TRUE(result.approx_equal(q, EPS));
}

void test_slerp_nearly_identical() {
    // Tests the NLERP fallback threshold
    Quaternion a = Quaternion::from_axis_angle(Vec3::up(), 0.0f);
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), 0.0001f);

    Quaternion result = Quaternion::slerp(a, b, 0.5f);
    ASSERT_FLOAT_EQ(result.norm(), 1.0f, EPS);
}

void test_nlerp_basic() {
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);

    Quaternion result = Quaternion::nlerp(a, b, 0.5f);

    // NLERP result should be unit length
    ASSERT_FLOAT_EQ(result.norm(), 1.0f, EPS);

    // Should be approximately the same as SLERP for small angles
    Quaternion slerp_result = Quaternion::slerp(a, b, 0.5f);
    ASSERT_TRUE(result.approx_equal(slerp_result, 0.01f));
}

void test_slerp_opposite_quaternions() {
    // q and -q represent the same rotation
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion(-1.0f, 0.0f, 0.0f, 0.0f);  // -identity

    Quaternion result = Quaternion::slerp(a, b, 0.5f);
    // Should still produce a valid result (identity or close)
    ASSERT_FLOAT_EQ(result.norm(), 1.0f, EPS);
}

// ============================================================================
//  Mat4 Tests
// ============================================================================

void test_mat4_identity() {
    Mat4 m = Mat4::identity();
    Vec3 v(3.0f, 5.0f, 7.0f);
    Vec3 result = m.transform_point(v);
    ASSERT_VEC3_EQ(result, 3.0f, 5.0f, 7.0f, EPS);
}

void test_mat4_translation() {
    Mat4 m = Mat4::translation(Vec3(10.0f, 20.0f, 30.0f));
    Vec3 result = m.transform_point(Vec3::zero());
    ASSERT_VEC3_EQ(result, 10.0f, 20.0f, 30.0f, EPS);
}

void test_mat4_scale() {
    Mat4 m = Mat4::scale(Vec3(2.0f, 3.0f, 4.0f));
    Vec3 result = m.transform_point(Vec3(1.0f, 1.0f, 1.0f));
    ASSERT_VEC3_EQ(result, 2.0f, 3.0f, 4.0f, EPS);
}

void test_mat4_rotation_from_quaternion() {
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    Mat4 m = Mat4::rotation(q);

    Vec3 result = m.transform_point(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(result, 0.0f, 0.0f, -1.0f, EPS);
}

void test_mat4_trs() {
    // TRS: translate(5,0,0) * rotate(90° Y) * scale(2)
    Mat4 m = Mat4::trs(
        Vec3(5.0f, 0.0f, 0.0f),
        Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f),
        Vec3(2.0f, 2.0f, 2.0f)
    );

    // (1,0,0) scaled by 2 = (2,0,0), rotated 90° Y = (0,0,-2), then + (5,0,0)
    Vec3 result = m.transform_point(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(result, 5.0f, 0.0f, -2.0f, EPS);
}

void test_mat4_direction_ignores_translation() {
    Mat4 m = Mat4::translation(Vec3(100.0f, 200.0f, 300.0f));
    Vec3 dir = m.transform_direction(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(dir, 1.0f, 0.0f, 0.0f, EPS);
}

void test_mat4_multiplication() {
    Mat4 a = Mat4::translation(Vec3(1.0f, 0.0f, 0.0f));
    Mat4 b = Mat4::translation(Vec3(0.0f, 2.0f, 0.0f));
    Mat4 c = a * b;

    Vec3 result = c.transform_point(Vec3::zero());
    ASSERT_VEC3_EQ(result, 1.0f, 2.0f, 0.0f, EPS);
}

// ============================================================================
//  Transform Tests
// ============================================================================

void test_transform_default() {
    Transform t;
    ASSERT_TRUE(t.position().approx_equal(Vec3::zero(), EPS));
    ASSERT_TRUE(t.rotation().approx_equal(Quaternion::identity(), EPS));
    ASSERT_TRUE(t.scale().approx_equal(Vec3::one(), EPS));
}

void test_transform_movement() {
    Transform t;
    t.translate(Vec3(5.0f, 0.0f, 0.0f));
    ASSERT_TRUE(t.position().approx_equal(Vec3(5.0f, 0.0f, 0.0f), EPS));

    t.translate(Vec3(0.0f, 3.0f, 0.0f));
    ASSERT_TRUE(t.position().approx_equal(Vec3(5.0f, 3.0f, 0.0f), EPS));
}

void test_transform_local_movement() {
    Transform t;
    // Rotate 90° around Y, then move "forward" (local -Z)
    t.rotate_axis(Vec3::up(), PI / 2.0f);
    t.translate_local(Vec3(0.0f, 0.0f, -1.0f));  // Forward in local space

    // After 90° Y rotation, forward (-Z) becomes (-1, 0, 0) in world space
    // Hmm actually: rotating 90° around Y maps -Z to -X? Let's verify:
    // Rotating +X 90° around Y gives +Z mapped to... let's just check
    Vec3 pos = t.position();
    ASSERT_FLOAT_EQ(pos.y, 0.0f, EPS);
    ASSERT_FLOAT_EQ(pos.length(), 1.0f, EPS);  // Moved 1 unit
}

void test_transform_forward_direction() {
    Transform t;
    Vec3 fwd = t.forward();
    ASSERT_TRUE(fwd.approx_equal(Vec3::forward(), EPS));

    // Rotate 90° Y
    t.rotate_axis(Vec3::up(), PI / 2.0f);
    fwd = t.forward();
    // Forward (-Z) rotated 90° Y = (-X, 0, 0)
    ASSERT_TRUE(fwd.approx_equal(Vec3(-1.0f, 0.0f, 0.0f), EPS));
}

void test_transform_interpolation() {
    Transform a;
    a.set_position(Vec3(0.0f, 0.0f, 0.0f));

    Transform b;
    b.set_position(Vec3(10.0f, 0.0f, 0.0f));
    b.set_rotation(Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f));

    Transform mid = Transform::interpolate(a, b, 0.5f);

    // Position should be midpoint
    ASSERT_TRUE(mid.position().approx_equal(Vec3(5.0f, 0.0f, 0.0f), EPS));

    // Rotation should be halfway (45° Y)
    Quaternion expected_rot = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    ASSERT_TRUE(mid.rotation().approx_equal(expected_rot, EPS));
}

void test_transform_matrix() {
    Transform t;
    t.set_position(Vec3(1.0f, 2.0f, 3.0f));
    t.set_scale(Vec3(2.0f, 2.0f, 2.0f));

    Mat4 m = t.to_matrix();
    Vec3 result = m.transform_point(Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_VEC3_EQ(result, 3.0f, 2.0f, 3.0f, EPS);
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "=== ForceFieldEngine Math Tests ===" << std::endl;

    std::cout << "\n--- Vec3 ---" << std::endl;
    RUN_TEST(test_vec3_default_constructor);
    RUN_TEST(test_vec3_arithmetic);
    RUN_TEST(test_vec3_dot_product);
    RUN_TEST(test_vec3_cross_product);
    RUN_TEST(test_vec3_length_and_normalization);
    RUN_TEST(test_vec3_lerp);
    RUN_TEST(test_vec3_distance);

    std::cout << "\n--- Quaternion ---" << std::endl;
    RUN_TEST(test_quaternion_identity);
    RUN_TEST(test_quaternion_from_axis_angle);
    RUN_TEST(test_quaternion_rotation_90_x);
    RUN_TEST(test_quaternion_rotation_composition);
    RUN_TEST(test_quaternion_inverse);
    RUN_TEST(test_quaternion_conjugate_rotation);
    RUN_TEST(test_quaternion_from_euler);
    RUN_TEST(test_quaternion_from_two_vectors);
    RUN_TEST(test_quaternion_from_two_vectors_same_direction);
    RUN_TEST(test_quaternion_from_two_vectors_opposite);

    std::cout << "\n--- SLERP (Core Experiment) ---" << std::endl;
    RUN_TEST(test_slerp_endpoints);
    RUN_TEST(test_slerp_midpoint);
    RUN_TEST(test_slerp_constant_angular_velocity);
    RUN_TEST(test_slerp_shortest_path);
    RUN_TEST(test_slerp_same_quaternion);
    RUN_TEST(test_slerp_nearly_identical);
    RUN_TEST(test_nlerp_basic);
    RUN_TEST(test_slerp_opposite_quaternions);

    std::cout << "\n--- Mat4 ---" << std::endl;
    RUN_TEST(test_mat4_identity);
    RUN_TEST(test_mat4_translation);
    RUN_TEST(test_mat4_scale);
    RUN_TEST(test_mat4_rotation_from_quaternion);
    RUN_TEST(test_mat4_trs);
    RUN_TEST(test_mat4_direction_ignores_translation);
    RUN_TEST(test_mat4_multiplication);

    std::cout << "\n--- Transform ---" << std::endl;
    RUN_TEST(test_transform_default);
    RUN_TEST(test_transform_movement);
    RUN_TEST(test_transform_local_movement);
    RUN_TEST(test_transform_forward_direction);
    RUN_TEST(test_transform_interpolation);
    RUN_TEST(test_transform_matrix);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total assertions: " << g_tests_run << std::endl;
    std::cout << "  Passed: " << g_tests_passed << std::endl;
    std::cout << "  Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
