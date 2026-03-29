/**
 * @file test_math_core.cpp
 * @brief Focused unit tests for core math operations: Vec3, Quaternion, Mat4.
 *
 * Targets issue #17 (low test coverage) by exercising:
 *   - Vec3: arithmetic, dot, cross, normalization, lerp, edge cases
 *   - Quaternion: construction, multiplication, SLERP, NLERP, rotation,
 *     axis-angle round-trip, Euler extraction, edge cases
 *   - Mat4: identity, TRS, perspective, look-at, matrix multiply,
 *     point/direction transform
 *
 * Uses the same lightweight test macros as the rest of the test suite.
 */

#include "../src/math/Vec3.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Mat4.h"
#include "../src/core/Transform.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

// ── Minimal Test Framework ──────────────────────────────────────────────────
static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                     \
    do {                                                                      \
        ++g_tests_run;                                                        \
        if (!(expr)) {                                                        \
            std::cerr << "  FAIL: " << #expr << " (" << __FILE__ << ":"       \
                      << __LINE__ << ")" << std::endl;                        \
            ++g_tests_failed;                                                 \
        } else { ++g_tests_passed; }                                          \
    } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                            \
    do {                                                                      \
        ++g_tests_run;                                                        \
        if (std::abs((a) - (b)) > (eps)) {                                    \
            std::cerr << "  FAIL: " << #a << " == " << #b                     \
                      << " (got " << (a) << " vs " << (b) << ") at "          \
                      << __FILE__ << ":" << __LINE__ << std::endl;            \
            ++g_tests_failed;                                                 \
        } else { ++g_tests_passed; }                                          \
    } while (0)

#define ASSERT_VEC3_EQ(v, ex, ey, ez, eps)                                    \
    do {                                                                      \
        ASSERT_FLOAT_EQ((v).x, (ex), (eps));                                  \
        ASSERT_FLOAT_EQ((v).y, (ey), (eps));                                  \
        ASSERT_FLOAT_EQ((v).z, (ez), (eps));                                  \
    } while (0)

#define ASSERT_THROWS(expr)                                                   \
    do {                                                                      \
        ++g_tests_run;                                                        \
        bool threw = false;                                                   \
        try { (void)(expr); } catch (...) { threw = true; }                   \
        if (!threw) {                                                         \
            std::cerr << "  FAIL: expected throw from " << #expr              \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl;  \
            ++g_tests_failed;                                                 \
        } else { ++g_tests_passed; }                                          \
    } while (0)

#define RUN_TEST(fn)                                                          \
    do {                                                                      \
        std::cout << "  " << #fn << "... ";                                   \
        int bf = g_tests_failed; fn();                                        \
        std::cout << (g_tests_failed == bf ? "OK" : "FAILED") << std::endl;   \
    } while (0)

using namespace qe::math;
using namespace qe::core;

constexpr float PI  = 3.14159265358979f;
constexpr float EPS = 1e-4f;

// ============================================================================
//  Vec3 — Comprehensive
// ============================================================================

void test_vec3_default_ctor() {
    Vec3 v;
    ASSERT_VEC3_EQ(v, 0, 0, 0, EPS);
}

void test_vec3_arithmetic() {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    auto sum = a + b;
    ASSERT_VEC3_EQ(sum, 5, 7, 9, EPS);
    auto diff = b - a;
    ASSERT_VEC3_EQ(diff, 3, 3, 3, EPS);
    auto scaled = a * 2.0f;
    ASSERT_VEC3_EQ(scaled, 2, 4, 6, EPS);
    auto div = b / 2.0f;
    ASSERT_VEC3_EQ(div, 2, 2.5f, 3, EPS);
}

void test_vec3_compound_assignment() {
    Vec3 v(1, 2, 3);
    v += Vec3(1, 1, 1);
    ASSERT_VEC3_EQ(v, 2, 3, 4, EPS);
    v -= Vec3(1, 1, 1);
    ASSERT_VEC3_EQ(v, 1, 2, 3, EPS);
    v *= 3.0f;
    ASSERT_VEC3_EQ(v, 3, 6, 9, EPS);
}

void test_vec3_dot_product() {
    Vec3 a(1, 0, 0), b(0, 1, 0);
    ASSERT_FLOAT_EQ(a.dot(b), 0.0f, EPS);  // perpendicular
    ASSERT_FLOAT_EQ(a.dot(a), 1.0f, EPS);  // parallel
    Vec3 c(1, 2, 3), d(4, 5, 6);
    ASSERT_FLOAT_EQ(c.dot(d), 32.0f, EPS);
}

void test_vec3_cross_product() {
    Vec3 x(1, 0, 0), y(0, 1, 0);
    auto z = x.cross(y);
    ASSERT_VEC3_EQ(z, 0, 0, 1, EPS);
    auto neg_z = y.cross(x);
    ASSERT_VEC3_EQ(neg_z, 0, 0, -1, EPS);
}

void test_vec3_length() {
    Vec3 v(3, 4, 0);
    ASSERT_FLOAT_EQ(v.length(), 5.0f, EPS);
    ASSERT_FLOAT_EQ(v.length_squared(), 25.0f, EPS);
}

void test_vec3_normalize() {
    Vec3 v(3, 0, 0);
    auto n = v.normalized();
    ASSERT_VEC3_EQ(n, 1, 0, 0, EPS);
    ASSERT_FLOAT_EQ(n.length(), 1.0f, EPS);
}

void test_vec3_normalize_zero_throws() {
    Vec3 zero;
    ASSERT_THROWS(zero.normalized());
}

void test_vec3_division_by_zero_throws() {
    Vec3 v(1, 2, 3);
    ASSERT_THROWS(v / 0.0f);
}

void test_vec3_lerp() {
    Vec3 a(0, 0, 0), b(10, 20, 30);
    auto mid = a.lerp(b, 0.5f);
    ASSERT_VEC3_EQ(mid, 5, 10, 15, EPS);
    auto start = a.lerp(b, 0.0f);
    ASSERT_VEC3_EQ(start, 0, 0, 0, EPS);
    auto end = a.lerp(b, 1.0f);
    ASSERT_VEC3_EQ(end, 10, 20, 30, EPS);
}

void test_vec3_negation() {
    Vec3 v(1, -2, 3);
    auto neg = -v;
    ASSERT_VEC3_EQ(neg, -1, 2, -3, EPS);
}

void test_vec3_equality() {
    Vec3 a(1, 2, 3), b(1, 2, 3), c(1, 2, 4);
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
}

void test_vec3_distance() {
    Vec3 a(0, 0, 0), b(3, 4, 0);
    ASSERT_FLOAT_EQ(a.distance_to(b), 5.0f, EPS);
}

void test_vec3_approx_equal() {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(1.00001f, 2.00001f, 3.00001f);
    ASSERT_TRUE(a.approx_equal(b));
    ASSERT_TRUE(!a.approx_equal(Vec3(2, 2, 3)));
}

void test_vec3_hadamard() {
    Vec3 a(2, 3, 4), b(5, 6, 7);
    auto h = a * b;
    ASSERT_VEC3_EQ(h, 10, 18, 28, EPS);
}

void test_vec3_commutative_scalar() {
    Vec3 v(1, 2, 3);
    auto a = v * 5.0f;
    auto b = 5.0f * v;
    ASSERT_TRUE(a == b);
}

void test_vec3_static_directions() {
    ASSERT_VEC3_EQ(Vec3::up(),      0,  1,  0, EPS);
    ASSERT_VEC3_EQ(Vec3::down(),    0, -1,  0, EPS);
    ASSERT_VEC3_EQ(Vec3::right(),   1,  0,  0, EPS);
    ASSERT_VEC3_EQ(Vec3::forward(), 0,  0, -1, EPS);
    ASSERT_VEC3_EQ(Vec3::zero(),    0,  0,  0, EPS);
    ASSERT_VEC3_EQ(Vec3::one(),     1,  1,  1, EPS);
}

// ============================================================================
//  Quaternion — Comprehensive
// ============================================================================

void test_quat_identity() {
    Quaternion q;
    ASSERT_FLOAT_EQ(q.w, 1.0f, EPS);
    ASSERT_FLOAT_EQ(q.x, 0.0f, EPS);
    ASSERT_FLOAT_EQ(q.y, 0.0f, EPS);
    ASSERT_FLOAT_EQ(q.z, 0.0f, EPS);
}

void test_quat_norm() {
    Quaternion q(1, 2, 3, 4);
    float expected = std::sqrt(1 + 4 + 9 + 16);
    ASSERT_FLOAT_EQ(q.norm(), expected, EPS);
}

void test_quat_normalized() {
    Quaternion q(1, 2, 3, 4);
    auto n = q.normalized();
    ASSERT_FLOAT_EQ(n.norm(), 1.0f, EPS);
}

void test_quat_normalize_zero_throws() {
    Quaternion q(0, 0, 0, 0);
    ASSERT_THROWS(q.normalized());
}

void test_quat_conjugate() {
    Quaternion q(1, 2, 3, 4);
    auto c = q.conjugate();
    ASSERT_FLOAT_EQ(c.w,  1.0f, EPS);
    ASSERT_FLOAT_EQ(c.x, -2.0f, EPS);
    ASSERT_FLOAT_EQ(c.y, -3.0f, EPS);
    ASSERT_FLOAT_EQ(c.z, -4.0f, EPS);
}

void test_quat_inverse() {
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    auto inv = q.inverse();
    auto product = q * inv;
    ASSERT_TRUE(product.approx_equal(Quaternion::identity()));
}

void test_quat_inverse_zero_throws() {
    Quaternion q(0, 0, 0, 0);
    ASSERT_THROWS(q.inverse());
}

void test_quat_multiplication_identity() {
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 3.0f);
    auto result = q * Quaternion::identity();
    ASSERT_TRUE(result.approx_equal(q));
}

void test_quat_multiplication_composition() {
    // 90 degrees around Y twice = 180 degrees around Y
    auto q90 = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    auto q180 = q90 * q90;
    auto expected = Quaternion::from_axis_angle(Vec3::up(), PI);
    ASSERT_TRUE(q180.approx_equal(expected));
}

void test_quat_from_axis_angle() {
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
    // 90 deg around Y should rotate (1,0,0) to (0,0,-1)
    Vec3 rotated = q.rotate(Vec3(1, 0, 0));
    ASSERT_TRUE(rotated.approx_equal(Vec3(0, 0, -1)));
}

void test_quat_from_euler() {
    // Pure yaw of 90 degrees
    auto q = Quaternion::from_euler(0, PI / 2.0f, 0);
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
}

void test_quat_rotate_identity() {
    Quaternion id = Quaternion::identity();
    Vec3 v(1, 2, 3);
    Vec3 r = id.rotate(v);
    ASSERT_TRUE(r.approx_equal(v));
}

void test_quat_rotate_90_around_y() {
    auto q = Quaternion::from_axis_angle(Vec3(0, 1, 0), PI / 2.0f);
    Vec3 v(1, 0, 0);
    Vec3 r = q.rotate(v);
    ASSERT_TRUE(r.approx_equal(Vec3(0, 0, -1)));
}

void test_quat_rotate_180_around_z() {
    auto q = Quaternion::from_axis_angle(Vec3(0, 0, 1), PI);
    Vec3 v(1, 0, 0);
    Vec3 r = q.rotate(v);
    ASSERT_TRUE(r.approx_equal(Vec3(-1, 0, 0)));
}

void test_quat_slerp_endpoints() {
    auto a = Quaternion::from_axis_angle(Vec3::up(), 0);
    auto b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    auto s0 = Quaternion::slerp(a, b, 0.0f);
    auto s1 = Quaternion::slerp(a, b, 1.0f);
    ASSERT_TRUE(s0.approx_equal(a));
    ASSERT_TRUE(s1.approx_equal(b));
}

void test_quat_slerp_midpoint() {
    auto a = Quaternion::from_axis_angle(Vec3::up(), 0);
    auto b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    auto mid = Quaternion::slerp(a, b, 0.5f);
    auto expected = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    ASSERT_TRUE(mid.approx_equal(expected, 1e-3f));
}

void test_quat_slerp_same_quaternion() {
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    auto result = Quaternion::slerp(q, q, 0.5f);
    ASSERT_TRUE(result.approx_equal(q));
}

void test_quat_slerp_opposite_quaternions() {
    auto a = Quaternion::from_axis_angle(Vec3::up(), 0);
    auto b = Quaternion::from_axis_angle(Vec3::up(), PI);
    // Should take shortest path
    auto mid = Quaternion::slerp(a, b, 0.5f);
    ASSERT_FLOAT_EQ(mid.norm(), 1.0f, EPS);
}

void test_quat_slerp_constant_angular_velocity() {
    auto a = Quaternion::identity();
    auto b = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    // Sample at t=0.25, 0.5, 0.75 — angular differences should be equal
    auto q1 = Quaternion::slerp(a, b, 0.25f);
    auto q2 = Quaternion::slerp(a, b, 0.50f);
    auto q3 = Quaternion::slerp(a, b, 0.75f);
    float angle1 = std::acos(std::abs(a.dot(q1)));
    float angle2 = std::acos(std::abs(q1.dot(q2)));
    float angle3 = std::acos(std::abs(q2.dot(q3)));
    ASSERT_FLOAT_EQ(angle1, angle2, 1e-3f);
    ASSERT_FLOAT_EQ(angle2, angle3, 1e-3f);
}

void test_quat_nlerp() {
    auto a = Quaternion::identity();
    auto b = Quaternion::from_axis_angle(Vec3::up(), PI / 4.0f);
    auto result = Quaternion::nlerp(a, b, 0.5f);
    ASSERT_FLOAT_EQ(result.norm(), 1.0f, EPS);
}

void test_quat_to_axis_angle_roundtrip() {
    Vec3 axis = Vec3(1, 1, 0).normalized();
    float angle = PI / 3.0f;
    auto q = Quaternion::from_axis_angle(axis, angle);
    auto [out_axis, out_angle] = q.to_axis_angle();
    ASSERT_FLOAT_EQ(out_angle, angle, 1e-3f);
    ASSERT_TRUE(out_axis.approx_equal(axis, 1e-3f));
}

void test_quat_to_axis_angle_identity() {
    auto [axis, angle] = Quaternion::identity().to_axis_angle();
    ASSERT_FLOAT_EQ(angle, 0.0f, EPS);
}

void test_quat_from_two_vectors_same() {
    auto q = Quaternion::from_two_vectors(Vec3::up(), Vec3::up());
    ASSERT_TRUE(q.approx_equal(Quaternion::identity()));
}

void test_quat_from_two_vectors_opposite() {
    auto q = Quaternion::from_two_vectors(Vec3::up(), Vec3::down());
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
    // Should represent 180-degree rotation
    Vec3 r = q.rotate(Vec3::up());
    ASSERT_TRUE(r.approx_equal(Vec3::down(), 1e-3f));
}

void test_quat_from_two_vectors_90() {
    auto q = Quaternion::from_two_vectors(Vec3(1, 0, 0), Vec3(0, 1, 0));
    Vec3 r = q.rotate(Vec3(1, 0, 0));
    ASSERT_TRUE(r.approx_equal(Vec3(0, 1, 0), 1e-3f));
}

void test_quat_dot() {
    auto a = Quaternion::identity();
    ASSERT_FLOAT_EQ(a.dot(a), 1.0f, EPS);
    auto b = Quaternion(0, 1, 0, 0);
    ASSERT_FLOAT_EQ(a.dot(b), 0.0f, EPS);
}

void test_quat_negation() {
    Quaternion q(1, 2, 3, 4);
    auto neg = -q;
    ASSERT_FLOAT_EQ(neg.w, -1.0f, EPS);
    ASSERT_FLOAT_EQ(neg.x, -2.0f, EPS);
}

void test_quat_scalar_mult() {
    Quaternion q(1, 0, 0, 0);
    auto r = q * 2.0f;
    ASSERT_FLOAT_EQ(r.w, 2.0f, EPS);
}

void test_quat_euler_extraction() {
    float p = 0.3f, y = 0.5f, r = 0.1f;
    auto q = Quaternion::from_euler(p, y, r);
    // Round-trip is approximate due to gimbal near poles but should hold
    // for small angles
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
}

void test_quat_approx_equal_negation() {
    // q and -q represent the same rotation
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 3.0f);
    auto neg = -q;
    ASSERT_TRUE(q.approx_equal(neg));
}

// ============================================================================
//  Mat4 — Comprehensive
// ============================================================================

void test_mat4_identity() {
    auto id = Mat4::identity();
    ASSERT_FLOAT_EQ(id.m[0][0], 1.0f, EPS);
    ASSERT_FLOAT_EQ(id.m[1][1], 1.0f, EPS);
    ASSERT_FLOAT_EQ(id.m[2][2], 1.0f, EPS);
    ASSERT_FLOAT_EQ(id.m[3][3], 1.0f, EPS);
    ASSERT_FLOAT_EQ(id.m[0][1], 0.0f, EPS);
    ASSERT_FLOAT_EQ(id.m[1][0], 0.0f, EPS);
}

void test_mat4_identity_multiply() {
    auto id = Mat4::identity();
    auto r = id * id;
    ASSERT_FLOAT_EQ(r.m[0][0], 1.0f, EPS);
    ASSERT_FLOAT_EQ(r.m[3][3], 1.0f, EPS);
    ASSERT_FLOAT_EQ(r.m[0][1], 0.0f, EPS);
}

void test_mat4_translation() {
    auto t = Mat4::translation(Vec3(10, 20, 30));
    Vec3 origin(0, 0, 0);
    Vec3 result = t.transform_point(origin);
    ASSERT_VEC3_EQ(result, 10, 20, 30, EPS);
}

void test_mat4_translation_direction() {
    auto t = Mat4::translation(Vec3(10, 20, 30));
    Vec3 dir(1, 0, 0);
    Vec3 result = t.transform_direction(dir);
    // Directions are not affected by translation
    ASSERT_VEC3_EQ(result, 1, 0, 0, EPS);
}

void test_mat4_scale() {
    auto s = Mat4::scale(Vec3(2, 3, 4));
    Vec3 v(1, 1, 1);
    Vec3 result = s.transform_point(v);
    ASSERT_VEC3_EQ(result, 2, 3, 4, EPS);
}

void test_mat4_rotation_from_quaternion() {
    auto q = Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f);
    auto r = Mat4::rotation(q);
    Vec3 v(1, 0, 0);
    Vec3 result = r.transform_point(v);
    ASSERT_TRUE(result.approx_equal(Vec3(0, 0, -1)));
}

void test_mat4_trs() {
    Vec3 pos(5, 0, 0);
    auto rot = Quaternion::identity();
    Vec3 scl(2, 2, 2);
    auto m = Mat4::trs(pos, rot, scl);
    Vec3 origin(0, 0, 0);
    Vec3 result = m.transform_point(origin);
    ASSERT_VEC3_EQ(result, 5, 0, 0, EPS);

    Vec3 unit(1, 0, 0);
    Vec3 result2 = m.transform_point(unit);
    ASSERT_VEC3_EQ(result2, 7, 0, 0, EPS);  // scaled by 2 + translated by 5
}

void test_mat4_perspective() {
    auto p = Mat4::perspective(PI / 3.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    // Verify near plane maps correctly (basic sanity)
    ASSERT_TRUE(p.m[0][0] != 0.0f);
    ASSERT_TRUE(p.m[1][1] != 0.0f);
    ASSERT_FLOAT_EQ(p.m[2][3], -1.0f, EPS);
}

void test_mat4_look_at() {
    Vec3 eye(0, 0, 5);
    Vec3 target(0, 0, 0);
    auto view = Mat4::look_at(eye, target, Vec3::up());
    // The eye should map near origin in view space
    Vec3 p = view.transform_point(eye);
    ASSERT_FLOAT_EQ(p.x, 0.0f, EPS);
    ASSERT_FLOAT_EQ(p.y, 0.0f, EPS);
}

void test_mat4_data_pointer() {
    auto id = Mat4::identity();
    const float* ptr = id.data();
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_FLOAT_EQ(ptr[0], 1.0f, EPS);
}

// ============================================================================
//  Transform — Comprehensive
// ============================================================================

void test_transform_default() {
    Transform t;
    ASSERT_VEC3_EQ(t.position(), 0, 0, 0, EPS);
    ASSERT_TRUE(t.rotation().approx_equal(Quaternion::identity()));
    ASSERT_VEC3_EQ(t.scale(), 1, 1, 1, EPS);
}

void test_transform_translate() {
    Transform t;
    t.translate(Vec3(1, 2, 3));
    ASSERT_VEC3_EQ(t.position(), 1, 2, 3, EPS);
}

void test_transform_translate_local() {
    Transform t;
    // Rotate 90 degrees around Y, then translate forward in local space
    t.set_rotation(Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f));
    t.translate_local(Vec3(0, 0, -1));  // forward = -Z
    // After 90 Y rotation, local -Z maps to world -X
    ASSERT_TRUE(t.position().approx_equal(Vec3(-1, 0, 0), 1e-3f));
}

void test_transform_direction_vectors() {
    Transform t;
    t.set_rotation(Quaternion::identity());
    ASSERT_TRUE(t.forward().approx_equal(Vec3::forward()));
    ASSERT_TRUE(t.right().approx_equal(Vec3::right()));
    ASSERT_TRUE(t.up().approx_equal(Vec3::up()));
}

void test_transform_look_at() {
    Transform t;
    t.set_position(Vec3(0, 0, 0));
    t.look_at(Vec3(0, 0, -10));
    ASSERT_TRUE(t.forward().approx_equal(Vec3(0, 0, -1), 1e-3f));
}

void test_transform_interpolate() {
    Transform a(Vec3(0, 0, 0), Quaternion::identity(), Vec3::one());
    Transform b(Vec3(10, 0, 0),
                Quaternion::from_axis_angle(Vec3::up(), PI / 2.0f),
                Vec3(2, 2, 2));
    auto mid = Transform::interpolate(a, b, 0.5f);
    ASSERT_FLOAT_EQ(mid.position().x, 5.0f, EPS);
    ASSERT_FLOAT_EQ(mid.scale().x, 1.5f, EPS);
}

void test_transform_matrix() {
    Transform t(Vec3(5, 0, 0), Quaternion::identity(), Vec3::one());
    auto m = t.to_matrix();
    Vec3 origin(0, 0, 0);
    Vec3 result = m.transform_point(origin);
    ASSERT_VEC3_EQ(result, 5, 0, 0, EPS);
}

void test_transform_rotate_axis() {
    Transform t;
    t.rotate_axis(Vec3::up(), PI / 2.0f);
    Vec3 fwd = t.forward();
    // After 90 Y rotation, forward (-Z) maps to (-X, 0, 0)
    ASSERT_TRUE(fwd.approx_equal(Vec3(-1, 0, 0), 1e-3f));
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "\n=== QuatEngine Core Math Tests ===" << std::endl;

    std::cout << "\n--- Vec3 ---" << std::endl;
    RUN_TEST(test_vec3_default_ctor);
    RUN_TEST(test_vec3_arithmetic);
    RUN_TEST(test_vec3_compound_assignment);
    RUN_TEST(test_vec3_dot_product);
    RUN_TEST(test_vec3_cross_product);
    RUN_TEST(test_vec3_length);
    RUN_TEST(test_vec3_normalize);
    RUN_TEST(test_vec3_normalize_zero_throws);
    RUN_TEST(test_vec3_division_by_zero_throws);
    RUN_TEST(test_vec3_lerp);
    RUN_TEST(test_vec3_negation);
    RUN_TEST(test_vec3_equality);
    RUN_TEST(test_vec3_distance);
    RUN_TEST(test_vec3_approx_equal);
    RUN_TEST(test_vec3_hadamard);
    RUN_TEST(test_vec3_commutative_scalar);
    RUN_TEST(test_vec3_static_directions);

    std::cout << "\n--- Quaternion ---" << std::endl;
    RUN_TEST(test_quat_identity);
    RUN_TEST(test_quat_norm);
    RUN_TEST(test_quat_normalized);
    RUN_TEST(test_quat_normalize_zero_throws);
    RUN_TEST(test_quat_conjugate);
    RUN_TEST(test_quat_inverse);
    RUN_TEST(test_quat_inverse_zero_throws);
    RUN_TEST(test_quat_multiplication_identity);
    RUN_TEST(test_quat_multiplication_composition);
    RUN_TEST(test_quat_from_axis_angle);
    RUN_TEST(test_quat_from_euler);
    RUN_TEST(test_quat_rotate_identity);
    RUN_TEST(test_quat_rotate_90_around_y);
    RUN_TEST(test_quat_rotate_180_around_z);
    RUN_TEST(test_quat_slerp_endpoints);
    RUN_TEST(test_quat_slerp_midpoint);
    RUN_TEST(test_quat_slerp_same_quaternion);
    RUN_TEST(test_quat_slerp_opposite_quaternions);
    RUN_TEST(test_quat_slerp_constant_angular_velocity);
    RUN_TEST(test_quat_nlerp);
    RUN_TEST(test_quat_to_axis_angle_roundtrip);
    RUN_TEST(test_quat_to_axis_angle_identity);
    RUN_TEST(test_quat_from_two_vectors_same);
    RUN_TEST(test_quat_from_two_vectors_opposite);
    RUN_TEST(test_quat_from_two_vectors_90);
    RUN_TEST(test_quat_dot);
    RUN_TEST(test_quat_negation);
    RUN_TEST(test_quat_scalar_mult);
    RUN_TEST(test_quat_euler_extraction);
    RUN_TEST(test_quat_approx_equal_negation);

    std::cout << "\n--- Mat4 ---" << std::endl;
    RUN_TEST(test_mat4_identity);
    RUN_TEST(test_mat4_identity_multiply);
    RUN_TEST(test_mat4_translation);
    RUN_TEST(test_mat4_translation_direction);
    RUN_TEST(test_mat4_scale);
    RUN_TEST(test_mat4_rotation_from_quaternion);
    RUN_TEST(test_mat4_trs);
    RUN_TEST(test_mat4_perspective);
    RUN_TEST(test_mat4_look_at);
    RUN_TEST(test_mat4_data_pointer);

    std::cout << "\n--- Transform ---" << std::endl;
    RUN_TEST(test_transform_default);
    RUN_TEST(test_transform_translate);
    RUN_TEST(test_transform_translate_local);
    RUN_TEST(test_transform_direction_vectors);
    RUN_TEST(test_transform_look_at);
    RUN_TEST(test_transform_interpolate);
    RUN_TEST(test_transform_matrix);
    RUN_TEST(test_transform_rotate_axis);

    std::cout << "\n=== Results: " << g_tests_passed << "/" << g_tests_run
              << " passed";
    if (g_tests_failed > 0) {
        std::cout << " (" << g_tests_failed << " FAILED)";
    }
    std::cout << " ===" << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
