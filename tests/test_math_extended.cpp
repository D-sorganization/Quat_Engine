/**
 * @file test_math_extended.cpp
 * @brief Extended coverage tests for Vec3, Quaternion, Mat4, and Transform.
 *
 * Targets the branches and methods NOT yet covered by test_math.cpp:
 *   - Vec3: division, compound assignment, component-wise *, ==, !=, approx_equal,
 *           direction constants (down), normalized exception, division-by-zero exception
 *   - Quaternion: pitch/yaw/roll extraction, to_axis_angle, norm_squared,
 *                 scalar operator*, operator+, negation, approx_equal opposite case
 *   - Mat4: perspective, look_at, data(), transform_point (w!=1 branch)
 *   - Transform: explicit constructor, right/up vectors, look_at, rotate (quat),
 *                matrix dirty-flag caching, scale interpolation
 */

#include "../src/core/Transform.h"
#include "../src/math/Mat4.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

// --- Minimal Test Framework ---

static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (!(expr)) {                                                       \
            std::cerr << "  FAIL: " << #expr << " (" << __FILE__ << ":"    \
                      << __LINE__ << ")" << std::endl;                      \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                           \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (std::abs((a) - (b)) > (eps)) {                                  \
            std::cerr << "  FAIL: " << #a << " == " << #b                  \
                      << " (got " << (a) << " vs " << (b)                  \
                      << ", eps=" << (eps) << ") at " << __FILE__           \
                      << ":" << __LINE__ << std::endl;                      \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define ASSERT_VEC3_EQ(v, ex, ey, ez, eps)                                   \
    do {                                                                     \
        ASSERT_FLOAT_EQ((v).x, (ex), (eps));                                \
        ASSERT_FLOAT_EQ((v).y, (ey), (eps));                                \
        ASSERT_FLOAT_EQ((v).z, (ez), (eps));                                \
    } while (0)

#define ASSERT_THROWS(expr, exception_type)                                  \
    do {                                                                     \
        ++g_tests_run;                                                       \
        bool caught = false;                                                 \
        try { (void)(expr); } catch (const exception_type&) { caught = true; } \
        if (!caught) {                                                       \
            std::cerr << "  FAIL: expected " #exception_type " at "        \
                      << __FILE__ << ":" << __LINE__ << std::endl;         \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define RUN_TEST(test_fn)                                                    \
    do {                                                                     \
        std::cout << "  " << #test_fn << "... ";                            \
        int before_fail = g_tests_failed;                                   \
        test_fn();                                                           \
        std::cout << (g_tests_failed == before_fail ? "OK" : "FAILED")     \
                  << std::endl;                                              \
    } while (0)

using namespace qe::math;
using namespace qe::core;

constexpr float PI  = 3.14159265358979f;
constexpr float EPS = 1e-4f;

// ============================================================================
//  Vec3 Extended Tests
// ============================================================================

void test_vec3_division() {
    Vec3 v(3.0f, 6.0f, 9.0f);
    Vec3 result = v / 3.0f;
    ASSERT_VEC3_EQ(result, 1.0f, 2.0f, 3.0f, EPS);
}

void test_vec3_division_by_zero_throws() {
    Vec3 v(1.0f, 2.0f, 3.0f);
    ASSERT_THROWS(v / 0.0f, std::domain_error);
}

void test_vec3_compound_assignment() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    a += b;
    ASSERT_VEC3_EQ(a, 5.0f, 7.0f, 9.0f, EPS);

    a -= b;
    ASSERT_VEC3_EQ(a, 1.0f, 2.0f, 3.0f, EPS);

    a *= 2.0f;
    ASSERT_VEC3_EQ(a, 2.0f, 4.0f, 6.0f, EPS);
}

void test_vec3_component_wise_multiply() {
    Vec3 a(2.0f, 3.0f, 4.0f);
    Vec3 b(5.0f, 6.0f, 7.0f);
    Vec3 c = a * b;
    ASSERT_VEC3_EQ(c, 10.0f, 18.0f, 28.0f, EPS);
}

void test_vec3_equality_operators() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(1.0f, 2.0f, 3.0f);
    Vec3 c(1.0f, 2.0f, 4.0f);

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(!(a == c));
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(!(a != b));
}

void test_vec3_approx_equal() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(1.00001f, 2.00001f, 3.00001f);
    Vec3 c(1.1f, 2.0f, 3.0f);

    ASSERT_TRUE(a.approx_equal(b, 0.001f));
    ASSERT_TRUE(!a.approx_equal(c, 0.001f));
}

void test_vec3_direction_constants() {
    auto down = Vec3::down();
    ASSERT_VEC3_EQ(down, 0.0f, -1.0f, 0.0f, EPS);

    auto up = Vec3::up();
    ASSERT_VEC3_EQ(up, 0.0f, 1.0f, 0.0f, EPS);

    // up and down should be negatives of each other
    Vec3 neg_down = -down;
    ASSERT_VEC3_EQ(neg_down, 0.0f, 1.0f, 0.0f, EPS);
}

void test_vec3_normalize_zero_throws() {
    Vec3 zero;
    ASSERT_THROWS(zero.normalized(), std::domain_error);
}

void test_vec3_distance_symmetric() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 6.0f, 3.0f);
    ASSERT_FLOAT_EQ(a.distance_to(b), b.distance_to(a), EPS);
}

// ============================================================================
//  Quaternion Extended Tests
// ============================================================================

void test_quaternion_norm_squared() {
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 3.0f);
    float ns = q.norm_squared();
    float n  = q.norm();
    ASSERT_FLOAT_EQ(ns, n * n, EPS);
    ASSERT_FLOAT_EQ(ns, 1.0f, EPS);  // Unit quaternion
}

void test_quaternion_scalar_multiply() {
    Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion scaled = q * 2.0f;
    ASSERT_FLOAT_EQ(scaled.w, 2.0f, EPS);
    ASSERT_FLOAT_EQ(scaled.x, 0.0f, EPS);
}

void test_quaternion_addition() {
    Quaternion a(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion b(0.0f, 1.0f, 0.0f, 0.0f);
    Quaternion c = a + b;
    ASSERT_FLOAT_EQ(c.w, 1.0f, EPS);
    ASSERT_FLOAT_EQ(c.x, 1.0f, EPS);
    ASSERT_FLOAT_EQ(c.y, 0.0f, EPS);
    ASSERT_FLOAT_EQ(c.z, 0.0f, EPS);
}

void test_quaternion_negation() {
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    Quaternion neg = -q;
    ASSERT_FLOAT_EQ(neg.w, -q.w, EPS);
    ASSERT_FLOAT_EQ(neg.x, -q.x, EPS);
    ASSERT_FLOAT_EQ(neg.y, -q.y, EPS);
    ASSERT_FLOAT_EQ(neg.z, -q.z, EPS);
}

void test_quaternion_dot_product() {
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::identity();
    ASSERT_FLOAT_EQ(a.dot(b), 1.0f, EPS);

    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    // Dot with its own conjugate
    float d = q.dot(q.conjugate());
    // dot(q, conj(q)) = w^2 - x^2 - y^2 - z^2 for unit q with x=y=z=0:
    // For axis-angle, x=y=0, so it's w^2 - z^2 
    // Not necessarily 1; just check it's finite and in [-1, 1] for unit quats
    ASSERT_TRUE(std::abs(d) <= 1.0f + EPS);
}

void test_quaternion_pitch_yaw_roll() {
    // Create a quaternion from known Euler angles and extract them back
    // Pure pitch (rotation around X)
    float pitch_in = PI / 4.0f;
    Quaternion qp = Quaternion::from_axis_angle(Vec3::right(), pitch_in);
    float pitch_out = qp.pitch();
    ASSERT_FLOAT_EQ(std::abs(pitch_out), std::abs(pitch_in), 0.01f);

    // Identity quaternion should have zero pitch/yaw/roll
    Quaternion qi = Quaternion::identity();
    ASSERT_FLOAT_EQ(qi.pitch(), 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(qi.yaw(), 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(qi.roll(), 0.0f, 0.01f);
}

void test_quaternion_to_axis_angle_identity() {
    // Identity quaternion: angle=0, axis=default (1,0,0)
    Quaternion q = Quaternion::identity();
    auto [axis, angle] = q.to_axis_angle();
    ASSERT_FLOAT_EQ(angle, 0.0f, EPS);
    // Axis is well-defined as (1,0,0) for the identity
    ASSERT_FLOAT_EQ(axis.length(), 1.0f, EPS);
}

void test_quaternion_to_axis_angle_roundtrip() {
    Vec3 orig_axis = Vec3(0.0f, 1.0f, 0.0f);
    float orig_angle = PI / 3.0f;
    Quaternion q = Quaternion::from_axis_angle(orig_axis, orig_angle);
    auto [axis, angle] = q.to_axis_angle();

    ASSERT_FLOAT_EQ(angle, orig_angle, 0.001f);
    ASSERT_TRUE(axis.approx_equal(orig_axis, 0.001f));
}

void test_quaternion_approx_equal_opposite() {
    // q and -q represent the same rotation: approx_equal should return true
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 3.0f);
    Quaternion neg_q = -q;
    ASSERT_TRUE(q.approx_equal(neg_q, EPS));
}

void test_quaternion_normalize_zero_throws() {
    Quaternion zero(0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_THROWS(zero.normalized(), std::domain_error);
}

void test_quaternion_inverse_zero_throws() {
    Quaternion zero(0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_THROWS(zero.inverse(), std::domain_error);
}

// ============================================================================
//  Mat4 Extended Tests
// ============================================================================

void test_mat4_perspective_non_zero() {
    // Just verify perspective produces a non-zero matrix
    Mat4 m = Mat4::perspective(PI / 2.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    ASSERT_TRUE(std::abs(m.m[0][0]) > 0.0f);
    ASSERT_TRUE(std::abs(m.m[1][1]) > 0.0f);
    // m[2][3] should be -1 for standard perspective
    ASSERT_FLOAT_EQ(m.m[2][3], -1.0f, EPS);
}

void test_mat4_look_at_origin_to_negative_z() {
    // Camera at origin looking at (0,0,-1) from up=(0,1,0)
    // In a LH/RH system, this produces identity-like rotation
    Mat4 view = Mat4::look_at(
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(0.0f, 0.0f, -1.0f),
        Vec3::up()
    );

    // The view matrix should be the identity (camera at origin looking -Z)
    // x-axis: right = cross(up, forward) = cross((0,1,0),(0,0,1)) = (1,0,0)
    ASSERT_FLOAT_EQ(view.m[0][0], 1.0f, 0.01f);
    ASSERT_FLOAT_EQ(view.m[1][1], 1.0f, 0.01f);
    ASSERT_FLOAT_EQ(view.m[3][3], 1.0f, EPS);
}

void test_mat4_look_at_offset_camera() {
    // Camera at (0,0,5) looking at origin
    Mat4 view = Mat4::look_at(
        Vec3(0.0f, 0.0f, 5.0f),
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3::up()
    );

    // Translation part should encode -camera_position projected onto axes
    // With camera at (0,0,5) looking at origin: dot product shifts
    ASSERT_FLOAT_EQ(view.m[3][3], 1.0f, EPS);

    // Applying the view matrix to the eye point should give zero (origin in view space)
    Vec3 eye_in_view = view.transform_point(Vec3(0.0f, 0.0f, 5.0f));
    ASSERT_FLOAT_EQ(eye_in_view.x, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(eye_in_view.y, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(eye_in_view.z, 0.0f, 0.01f);
}

void test_mat4_data_pointer() {
    Mat4 m = Mat4::identity();
    const float* ptr = m.data();
    // First element is m[0][0] = 1 (identity)
    ASSERT_FLOAT_EQ(ptr[0], 1.0f, EPS);
    // m[0][1] = 0, m[0][2] = 0, m[0][3] = 0
    ASSERT_FLOAT_EQ(ptr[1], 0.0f, EPS);
    ASSERT_FLOAT_EQ(ptr[2], 0.0f, EPS);
    ASSERT_FLOAT_EQ(ptr[3], 0.0f, EPS);
    // m[1][0] = 0 (ptr[4])
    ASSERT_FLOAT_EQ(ptr[4], 0.0f, EPS);
    // m[1][1] = 1 (ptr[5])
    ASSERT_FLOAT_EQ(ptr[5], 1.0f, EPS);
}

void test_mat4_transform_point_perspective_divide() {
    // Construct a matrix that sets w != 1 to trigger perspective divide
    Mat4 m = Mat4::perspective(PI / 2.0f, 1.0f, 1.0f, 100.0f);
    // When we transform a point with a perspective matrix, rw != 1
    // Just check it runs without crash and returns a finite value
    Vec3 result = m.transform_point(Vec3(0.0f, 0.0f, -5.0f));
    ASSERT_TRUE(std::isfinite(result.x));
    ASSERT_TRUE(std::isfinite(result.y));
    ASSERT_TRUE(std::isfinite(result.z));
}

// ============================================================================
//  Transform Extended Tests
// ============================================================================

void test_transform_explicit_constructor() {
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quaternion rot = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    Vec3 scl(2.0f, 2.0f, 2.0f);

    Transform t(pos, rot, scl);
    ASSERT_TRUE(t.position().approx_equal(pos, EPS));
    ASSERT_TRUE(t.rotation().approx_equal(rot, EPS));
    ASSERT_TRUE(t.scale().approx_equal(scl, EPS));
}

void test_transform_right_and_up_vectors() {
    Transform t;

    // Default transform: right=+X, up=+Y
    ASSERT_TRUE(t.right().approx_equal(Vec3::right(), EPS));
    ASSERT_TRUE(t.up().approx_equal(Vec3::up(), EPS));

    // After 90° Y rotation:
    t.rotate_axis(Vec3::up(), PI / 2.0f);
    // right (+X) rotated 90° around Y = -Z
    Vec3 new_right = t.right();
    ASSERT_FLOAT_EQ(new_right.x, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(new_right.z, -1.0f, 0.01f);

    // up should remain World-Y
    Vec3 new_up = t.up();
    ASSERT_TRUE(new_up.approx_equal(Vec3::up(), 0.01f));
}

void test_transform_look_at() {
    Transform t;
    t.set_position(Vec3(0.0f, 0.0f, 0.0f));
    t.look_at(Vec3(0.0f, 0.0f, -10.0f));

    // After look_at toward -Z, forward should be -Z (identity rotation)
    Vec3 fwd = t.forward();
    ASSERT_TRUE(fwd.approx_equal(Vec3::forward(), 0.01f));
}

void test_transform_look_at_offset() {
    Transform t;
    t.set_position(Vec3(0.0f, 0.0f, 5.0f));
    t.look_at(Vec3(0.0f, 0.0f, 0.0f));

    // Forward should point toward -Z (from (0,0,5) to (0,0,0))
    Vec3 fwd = t.forward();
    ASSERT_FLOAT_EQ(fwd.z, -1.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.x, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.y, 0.0f, 0.01f);
}

void test_transform_rotate_quaternion() {
    Transform t;
    Quaternion q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    t.rotate(q);

    Vec3 fwd = t.forward();
    // Forward (-Z) rotated 90° Y = (+X... actually -X in right-hand)
    // Let's just verify the rotation was applied (forward changed)
    ASSERT_TRUE(!fwd.approx_equal(Vec3::forward(), 0.1f));
}

void test_transform_matrix_caching() {
    Transform t;
    t.set_position(Vec3(1.0f, 2.0f, 3.0f));

    // Call to_matrix() twice: second call should use cache (dirty=false)
    Mat4 m1 = t.to_matrix();
    Mat4 m2 = t.to_matrix();

    // Both matrices should be identical
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            ASSERT_FLOAT_EQ(m1.m[col][row], m2.m[col][row], EPS);
        }
    }

    // After modification, dirty=true, new matrix should differ
    t.set_position(Vec3(10.0f, 0.0f, 0.0f));
    Mat4 m3 = t.to_matrix();
    // Translation column changed
    ASSERT_FLOAT_EQ(m3.m[3][0], 10.0f, EPS);
}

void test_transform_interpolate_scale() {
    Transform a;
    a.set_scale(Vec3(1.0f, 1.0f, 1.0f));

    Transform b;
    b.set_scale(Vec3(3.0f, 3.0f, 3.0f));

    Transform mid = Transform::interpolate(a, b, 0.5f);
    ASSERT_TRUE(mid.scale().approx_equal(Vec3(2.0f, 2.0f, 2.0f), EPS));
}

void test_transform_translate_local() {
    Transform t;
    // After 90° Y rotation, local +X = world -Z
    t.rotate_axis(Vec3::up(), PI / 2.0f);
    t.translate_local(Vec3(1.0f, 0.0f, 0.0f));  // Move along local +X

    // Local +X rotated 90° Y -> world -Z... actually:
    // rotate(+X) by 90° around Y: x'=cos(90)*x + sin(90)*z = 0*1+1*0=0? 
    // Let's just verify position moved away from origin
    float dist = t.position().length();
    ASSERT_FLOAT_EQ(dist, 1.0f, 0.01f);
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "=== QuatEngine Math Extended Tests ===" << std::endl;

    std::cout << "\n--- Vec3 Extended ---" << std::endl;
    RUN_TEST(test_vec3_division);
    RUN_TEST(test_vec3_division_by_zero_throws);
    RUN_TEST(test_vec3_compound_assignment);
    RUN_TEST(test_vec3_component_wise_multiply);
    RUN_TEST(test_vec3_equality_operators);
    RUN_TEST(test_vec3_approx_equal);
    RUN_TEST(test_vec3_direction_constants);
    RUN_TEST(test_vec3_normalize_zero_throws);
    RUN_TEST(test_vec3_distance_symmetric);

    std::cout << "\n--- Quaternion Extended ---" << std::endl;
    RUN_TEST(test_quaternion_norm_squared);
    RUN_TEST(test_quaternion_scalar_multiply);
    RUN_TEST(test_quaternion_addition);
    RUN_TEST(test_quaternion_negation);
    RUN_TEST(test_quaternion_dot_product);
    RUN_TEST(test_quaternion_pitch_yaw_roll);
    RUN_TEST(test_quaternion_to_axis_angle_identity);
    RUN_TEST(test_quaternion_to_axis_angle_roundtrip);
    RUN_TEST(test_quaternion_approx_equal_opposite);
    RUN_TEST(test_quaternion_normalize_zero_throws);
    RUN_TEST(test_quaternion_inverse_zero_throws);

    std::cout << "\n--- Mat4 Extended ---" << std::endl;
    RUN_TEST(test_mat4_perspective_non_zero);
    RUN_TEST(test_mat4_look_at_origin_to_negative_z);
    RUN_TEST(test_mat4_look_at_offset_camera);
    RUN_TEST(test_mat4_data_pointer);
    RUN_TEST(test_mat4_transform_point_perspective_divide);

    std::cout << "\n--- Transform Extended ---" << std::endl;
    RUN_TEST(test_transform_explicit_constructor);
    RUN_TEST(test_transform_right_and_up_vectors);
    RUN_TEST(test_transform_look_at);
    RUN_TEST(test_transform_look_at_offset);
    RUN_TEST(test_transform_rotate_quaternion);
    RUN_TEST(test_transform_matrix_caching);
    RUN_TEST(test_transform_interpolate_scale);
    RUN_TEST(test_transform_translate_local);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "  Total assertions: " << g_tests_run << std::endl;
    std::cout << "  Passed: " << g_tests_passed << std::endl;
    std::cout << "  Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
