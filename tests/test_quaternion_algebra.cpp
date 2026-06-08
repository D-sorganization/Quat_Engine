// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_quaternion_algebra.cpp
 * @brief Algebraic-identity and edge-case tests for the Quaternion type.
 *
 * These tests complement test_math.cpp / test_math_core.cpp /
 * test_math_extended.cpp / test_quaternion_properties.cpp by exercising the
 * fundamental *algebraic* identities of Hamilton's quaternions and several
 * code paths that the existing suites never reach:
 *
 *   - The defining basis-element products (i*j=k, j*k=i, k*i=j, i^2=j^2=k^2=-1)
 *   - Non-commutativity and associativity of the Hamilton product
 *   - Conjugate / inverse algebraic laws, including the *non-unit* inverse
 *     scaling branch (inverse() == conjugate()/norm^2) which the unit-quaternion
 *     tests elsewhere can never distinguish from a plain conjugate
 *   - Double-cover: q and -q rotate vectors identically
 *   - Rotation homomorphism: (q1*q2).rotate(v) == q1.rotate(q2.rotate(v))
 *   - rotate() preserves vector length
 *   - from_euler <-> pitch/yaw/roll round-trip at known angles
 *   - Gimbal-lock behaviour at pitch = +/- pi/2
 *   - SLERP time-reversal symmetry
 *   - to_axis_angle() negative-w canonicalisation branch
 *   - Mat4::rotation(q) agrees with q.rotate() for an arbitrary axis
 */

#include "test_framework.h"

#include "../src/math/Mat4.h"
#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <cmath>

using namespace qe::math;

constexpr float PI  = 3.14159265358979f;
constexpr float EPS = 1e-4f;

// Basis quaternions for Hamilton's algebra: 1, i, j, k.
static const Quaternion Q_ONE(1.0f, 0.0f, 0.0f, 0.0f);
static const Quaternion Q_I(0.0f, 1.0f, 0.0f, 0.0f);
static const Quaternion Q_J(0.0f, 0.0f, 1.0f, 0.0f);
static const Quaternion Q_K(0.0f, 0.0f, 0.0f, 1.0f);

static bool quat_components_equal(const Quaternion& a, const Quaternion& b, float eps) {
    return std::abs(a.w - b.w) < eps && std::abs(a.x - b.x) < eps &&
           std::abs(a.y - b.y) < eps && std::abs(a.z - b.z) < eps;
}

// ============================================================================
//  Hamilton basis-element identities:  i^2 = j^2 = k^2 = ijk = -1
// ============================================================================

void test_basis_squares_are_negative_one() {
    // i*i = j*j = k*k = -1  (i.e. the quaternion (-1, 0, 0, 0))
    ASSERT_TRUE(quat_components_equal(Q_I * Q_I, -Q_ONE, EPS));
    ASSERT_TRUE(quat_components_equal(Q_J * Q_J, -Q_ONE, EPS));
    ASSERT_TRUE(quat_components_equal(Q_K * Q_K, -Q_ONE, EPS));
}

void test_basis_cyclic_products() {
    // i*j = k,  j*k = i,  k*i = j
    ASSERT_TRUE(quat_components_equal(Q_I * Q_J, Q_K, EPS));
    ASSERT_TRUE(quat_components_equal(Q_J * Q_K, Q_I, EPS));
    ASSERT_TRUE(quat_components_equal(Q_K * Q_I, Q_J, EPS));
}

void test_basis_anticyclic_products() {
    // j*i = -k,  k*j = -i,  i*k = -j   (the products do NOT commute)
    ASSERT_TRUE(quat_components_equal(Q_J * Q_I, -Q_K, EPS));
    ASSERT_TRUE(quat_components_equal(Q_K * Q_J, -Q_I, EPS));
    ASSERT_TRUE(quat_components_equal(Q_I * Q_K, -Q_J, EPS));
}

void test_ijk_product_is_negative_one() {
    // The defining relation i*j*k = -1
    ASSERT_TRUE(quat_components_equal((Q_I * Q_J) * Q_K, -Q_ONE, EPS));
}

// ============================================================================
//  Hamilton product structural properties
// ============================================================================

void test_multiplication_not_commutative() {
    Quaternion a = Quaternion::from_axis_angle(Vec3::up(),    PI / 3.0f);
    Quaternion b = Quaternion::from_axis_angle(Vec3::right(), PI / 4.0f);
    // a*b and b*a are genuinely different rotations here.
    ASSERT_TRUE(!quat_components_equal(a * b, b * a, 1e-2f));
}

void test_multiplication_is_associative() {
    Quaternion a = Quaternion::from_axis_angle(Vec3::up(),       0.7f);
    Quaternion b = Quaternion::from_axis_angle(Vec3::right(),    1.1f);
    Quaternion c = Quaternion::from_axis_angle(Vec3::forward(), -0.5f);
    ASSERT_TRUE(quat_components_equal((a * b) * c, a * (b * c), EPS));
}

void test_norm_is_multiplicative() {
    // |a*b| == |a|*|b| even for non-unit quaternions.
    Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion b(-2.0f, 0.5f, 1.0f, -1.5f);
    ASSERT_FLOAT_EQ((a * b).norm(), a.norm() * b.norm(), 1e-3f);
}

// ============================================================================
//  Conjugate / inverse algebraic laws
// ============================================================================

void test_conjugate_reverses_product_order() {
    // conj(a*b) == conj(b) * conj(a)
    Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion b(5.0f, -1.0f, 0.5f, 2.0f);
    ASSERT_TRUE(quat_components_equal((a * b).conjugate(),
                                      b.conjugate() * a.conjugate(), EPS));
}

void test_conjugate_is_involutive() {
    Quaternion q(1.0f, -2.0f, 3.0f, -4.0f);
    ASSERT_TRUE(quat_components_equal(q.conjugate().conjugate(), q, EPS));
}

void test_inverse_of_non_unit_quaternion() {
    // For a *non-unit* quaternion, inverse() must scale by 1/norm^2.
    // This distinguishes inverse() from conjugate() (they coincide only for
    // unit quaternions) and exercises the norm_squared != 1 branch.
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);   // norm^2 = 30
    Quaternion inv = q.inverse();

    // q * q^-1 == identity
    ASSERT_TRUE(quat_components_equal(q * inv, Quaternion::identity(), 1e-4f));
    // q^-1 * q == identity
    ASSERT_TRUE(quat_components_equal(inv * q, Quaternion::identity(), 1e-4f));

    // Explicit component check: inv == conjugate / norm^2
    float ns = q.norm_squared();            // 30
    ASSERT_FLOAT_EQ(inv.w,  q.w / ns, EPS);
    ASSERT_FLOAT_EQ(inv.x, -q.x / ns, EPS);
    ASSERT_FLOAT_EQ(inv.y, -q.y / ns, EPS);
    ASSERT_FLOAT_EQ(inv.z, -q.z / ns, EPS);

    // The inverse of a non-unit quaternion is genuinely NOT its conjugate.
    ASSERT_TRUE(!quat_components_equal(inv, q.conjugate(), 1e-2f));
}

// ============================================================================
//  Rotation semantics
// ============================================================================

void test_double_cover_q_and_neg_q_rotate_identically() {
    // q and -q are the two-fold cover of the same rotation: they must move
    // every vector to exactly the same place.
    Quaternion q   = Quaternion::from_axis_angle(Vec3(1.0f, 2.0f, -1.0f), 1.3f);
    Quaternion nq  = -q;
    const Vec3 samples[] = {
        Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f),
        Vec3(2.0f, -3.0f, 4.0f),
    };
    for (const Vec3& v : samples) {
        ASSERT_TRUE(q.rotate(v).approx_equal(nq.rotate(v), 1e-4f));
    }
}

void test_rotation_is_a_homomorphism() {
    // Applying the product rotation equals applying the rotations in sequence:
    //   (q1 * q2).rotate(v) == q1.rotate(q2.rotate(v))
    Quaternion q1 = Quaternion::from_axis_angle(Vec3::up(),    PI / 3.0f);
    Quaternion q2 = Quaternion::from_axis_angle(Vec3::right(), PI / 5.0f);
    const Vec3 samples[] = {
        Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f),
    };
    for (const Vec3& v : samples) {
        Vec3 combined  = (q1 * q2).rotate(v);
        Vec3 sequenced = q1.rotate(q2.rotate(v));
        ASSERT_TRUE(combined.approx_equal(sequenced, 1e-4f));
    }
}

void test_rotation_preserves_length() {
    Quaternion q = Quaternion::from_axis_angle(Vec3(2.0f, -1.0f, 3.0f), 2.1f);
    const Vec3 samples[] = {
        Vec3(3.0f, 4.0f, 0.0f),   // length 5
        Vec3(1.0f, 2.0f, 2.0f),   // length 3
        Vec3(-5.0f, 0.0f, 0.0f),  // length 5
    };
    for (const Vec3& v : samples) {
        ASSERT_FLOAT_EQ(q.rotate(v).length(), v.length(), 1e-3f);
    }
}

void test_identity_rotation_is_noop() {
    Vec3 v(7.0f, -2.0f, 5.0f);
    ASSERT_TRUE(Quaternion::identity().rotate(v).approx_equal(v, EPS));
}

// ============================================================================
//  Euler <-> quaternion round-trips and gimbal lock
// ============================================================================

void test_euler_roundtrip_known_angles() {
    // from_euler(a1, a2, a3) builds an intrinsic rotation whose three angles
    // are recovered by the extractors in this order (verified empirically and
    // self-consistently): a1 -> roll(), a2 -> yaw(), a3 -> pitch().
    //
    // NOTE: the from_euler parameter *names* in the header (pitch, yaw, roll)
    // do not line up with the pitch()/yaw()/roll() extractors. That is a
    // documented naming inconsistency in the product, not a math error: the
    // factory and the extractors form a consistent inverse pair in the order
    // asserted below. We pin that round-trip rather than any particular naming.
    float a1 = 0.25f, a2 = -0.9f, a3 = 0.4f;  // small angles, away from the pole
    Quaternion q = Quaternion::from_euler(a1, a2, a3);

    ASSERT_FLOAT_EQ(q.roll(),  a1, 1e-3f);
    ASSERT_FLOAT_EQ(q.yaw(),   a2, 1e-3f);
    ASSERT_FLOAT_EQ(q.pitch(), a3, 1e-3f);
}

void test_euler_single_axis_roundtrips() {
    // Each from_euler argument, applied alone, must round-trip through exactly
    // one extractor and leave the other two at zero.
    Quaternion qa = Quaternion::from_euler(0.5f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(qa.roll(),  0.5f, 1e-3f);
    ASSERT_FLOAT_EQ(qa.yaw(),   0.0f, 1e-3f);
    ASSERT_FLOAT_EQ(qa.pitch(), 0.0f, 1e-3f);

    Quaternion qb = Quaternion::from_euler(0.0f, 0.5f, 0.0f);
    ASSERT_FLOAT_EQ(qb.yaw(),   0.5f, 1e-3f);
    ASSERT_FLOAT_EQ(qb.roll(),  0.0f, 1e-3f);
    ASSERT_FLOAT_EQ(qb.pitch(), 0.0f, 1e-3f);

    Quaternion qc = Quaternion::from_euler(0.0f, 0.0f, 0.5f);
    ASSERT_FLOAT_EQ(qc.pitch(), 0.5f, 1e-3f);
    ASSERT_FLOAT_EQ(qc.roll(),  0.0f, 1e-3f);
    ASSERT_FLOAT_EQ(qc.yaw(),   0.0f, 1e-3f);
}

void test_gimbal_lock_pitch_plus_half_pi() {
    // roll() is the asin-based extractor (range [-pi/2, pi/2]); its argument is
    // clamped to [-1, 1] to stay numerically safe at the +/-pi/2 poles. Driving
    // it to exactly +pi/2 (via from_euler's first argument, which maps to roll)
    // must hit the pole without producing NaN.
    Quaternion q = Quaternion::from_euler(PI / 2.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
    float roll = q.roll();
    ASSERT_TRUE(std::isfinite(roll));
    ASSERT_FLOAT_EQ(roll, PI / 2.0f, 1e-3f);
}

void test_gimbal_lock_pitch_minus_half_pi() {
    // Same as above at the -pi/2 pole of the asin-based roll() extractor.
    Quaternion q = Quaternion::from_euler(-PI / 2.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
    float roll = q.roll();
    ASSERT_TRUE(std::isfinite(roll));
    ASSERT_FLOAT_EQ(roll, -PI / 2.0f, 1e-3f);
}

// ============================================================================
//  SLERP symmetry and to_axis_angle canonicalisation
// ============================================================================

void test_slerp_time_reversal_symmetry() {
    // slerp(a, b, t) and slerp(b, a, 1-t) describe the same orientation.
    Quaternion a = Quaternion::from_axis_angle(Vec3::up(), 0.2f);
    Quaternion b = Quaternion::from_axis_angle(Vec3(1.0f, 1.0f, 0.0f), 1.4f);
    for (int i = 0; i <= 10; ++i) {
        float t = static_cast<float>(i) / 10.0f;
        Quaternion fwd = Quaternion::slerp(a, b, t);
        Quaternion rev = Quaternion::slerp(b, a, 1.0f - t);
        ASSERT_TRUE(fwd.approx_equal(rev, 1e-3f));
    }
}

void test_to_axis_angle_negative_w_branch() {
    // A quaternion with w < 0 represents the same rotation as its negation
    // (w > 0). to_axis_angle() canonicalises to the w >= 0 hemisphere, so the
    // reported angle stays in [0, pi]. Build a 270 deg rotation, whose unit
    // quaternion has w = cos(135 deg) < 0, to drive that branch.
    Vec3 axis = Vec3(0.0f, 1.0f, 0.0f);
    Quaternion q = Quaternion::from_axis_angle(axis, 1.5f * PI);  // 270 deg, w<0
    ASSERT_TRUE(q.w < 0.0f);

    auto [out_axis, angle] = q.to_axis_angle();
    // Canonical angle is the shorter equivalent: 360 - 270 = 90 deg about -axis,
    // i.e. angle in [0, pi]; here 0.5*pi.
    ASSERT_TRUE(angle >= 0.0f && angle <= PI + EPS);
    ASSERT_FLOAT_EQ(angle, 0.5f * PI, 1e-3f);

    // Reconstructing from the canonical axis/angle must reproduce the rotation
    // (up to double-cover sign).
    Quaternion rebuilt = Quaternion::from_axis_angle(out_axis, angle);
    ASSERT_TRUE(rebuilt.approx_equal(q, 1e-3f));
}

// ============================================================================
//  Rotation-matrix consistency
// ============================================================================

void test_mat4_rotation_matches_quaternion_rotate() {
    // Mat4::rotation(q) must transform points exactly like q.rotate() for an
    // arbitrary (non-axis-aligned) rotation.
    Quaternion q = Quaternion::from_axis_angle(Vec3(1.0f, 2.0f, 3.0f), 1.234f);
    Mat4 m = Mat4::rotation(q);
    const Vec3 samples[] = {
        Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f),
        Vec3(2.0f, -3.0f, 1.0f),
    };
    for (const Vec3& v : samples) {
        Vec3 via_matrix = m.transform_point(v);
        Vec3 via_quat   = q.rotate(v);
        ASSERT_TRUE(via_matrix.approx_equal(via_quat, 1e-3f));
    }
}

void test_mat4_rotation_is_orthonormal() {
    // The 3x3 rotation block must have orthonormal columns (a proper rotation).
    Quaternion q = Quaternion::from_axis_angle(Vec3(0.0f, 1.0f, 1.0f), 0.85f);
    Mat4 m = Mat4::rotation(q);
    Vec3 c0(m.m[0][0], m.m[0][1], m.m[0][2]);
    Vec3 c1(m.m[1][0], m.m[1][1], m.m[1][2]);
    Vec3 c2(m.m[2][0], m.m[2][1], m.m[2][2]);

    ASSERT_FLOAT_EQ(c0.length(), 1.0f, 1e-3f);
    ASSERT_FLOAT_EQ(c1.length(), 1.0f, 1e-3f);
    ASSERT_FLOAT_EQ(c2.length(), 1.0f, 1e-3f);
    ASSERT_FLOAT_EQ(c0.dot(c1), 0.0f, 1e-3f);
    ASSERT_FLOAT_EQ(c0.dot(c2), 0.0f, 1e-3f);
    ASSERT_FLOAT_EQ(c1.dot(c2), 0.0f, 1e-3f);
}

// ============================================================================
//  Main
// ============================================================================

int main() {
    std::cout << "=== QuatEngine Quaternion Algebra Tests ===" << std::endl;

    std::cout << "\n--- Hamilton basis identities ---" << std::endl;
    RUN_TEST(test_basis_squares_are_negative_one);
    RUN_TEST(test_basis_cyclic_products);
    RUN_TEST(test_basis_anticyclic_products);
    RUN_TEST(test_ijk_product_is_negative_one);

    std::cout << "\n--- Product structure ---" << std::endl;
    RUN_TEST(test_multiplication_not_commutative);
    RUN_TEST(test_multiplication_is_associative);
    RUN_TEST(test_norm_is_multiplicative);

    std::cout << "\n--- Conjugate / inverse ---" << std::endl;
    RUN_TEST(test_conjugate_reverses_product_order);
    RUN_TEST(test_conjugate_is_involutive);
    RUN_TEST(test_inverse_of_non_unit_quaternion);

    std::cout << "\n--- Rotation semantics ---" << std::endl;
    RUN_TEST(test_double_cover_q_and_neg_q_rotate_identically);
    RUN_TEST(test_rotation_is_a_homomorphism);
    RUN_TEST(test_rotation_preserves_length);
    RUN_TEST(test_identity_rotation_is_noop);

    std::cout << "\n--- Euler / gimbal lock ---" << std::endl;
    RUN_TEST(test_euler_roundtrip_known_angles);
    RUN_TEST(test_euler_single_axis_roundtrips);
    RUN_TEST(test_gimbal_lock_pitch_plus_half_pi);
    RUN_TEST(test_gimbal_lock_pitch_minus_half_pi);

    std::cout << "\n--- SLERP / axis-angle ---" << std::endl;
    RUN_TEST(test_slerp_time_reversal_symmetry);
    RUN_TEST(test_to_axis_angle_negative_w_branch);

    std::cout << "\n--- Rotation matrix ---" << std::endl;
    RUN_TEST(test_mat4_rotation_matches_quaternion_rotate);
    RUN_TEST(test_mat4_rotation_is_orthonormal);

    return TEST_REPORT();
}
