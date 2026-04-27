// Copyright (c) 2026 D-Sorganization. All rights reserved.
#include "test_framework.h"

#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

using namespace qe::math;

constexpr float PI = 3.14159265358979f;
constexpr float EPS = 1e-4f;

bool axis_matches(const Vec3& actual, const Vec3& expected, float epsilon) {
    return actual.approx_equal(expected, epsilon) ||
           actual.approx_equal(-expected, epsilon);
}

void test_axis_angle_roundtrip_properties() {
    const Vec3 axes[] = {
        Vec3::right(),
        Vec3::up(),
        Vec3::forward(),
        Vec3(1.0f, 1.0f, 0.0f).normalized(),
        Vec3(-2.0f, 1.0f, 3.0f).normalized(),
    };
    const float angles[] = {
        0.0f,
        PI / 6.0f,
        PI / 4.0f,
        PI / 2.0f,
        PI,
    };

    for (const Vec3& axis : axes) {
        for (float angle : angles) {
            Quaternion q = Quaternion::from_axis_angle(axis, angle);
            ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);

            auto [roundtrip_axis, roundtrip_angle] = q.to_axis_angle();
            if (angle < 1e-6f) {
                ASSERT_FLOAT_EQ(roundtrip_angle, 0.0f, EPS);
            } else {
                ASSERT_FLOAT_EQ(roundtrip_angle, angle, 0.002f);
                ASSERT_TRUE(axis_matches(roundtrip_axis, axis, 0.002f));
            }
        }
    }
}

void test_rotation_inverse_property() {
    const Vec3 axes[] = {
        Vec3::right(),
        Vec3::up(),
        Vec3::forward(),
        Vec3(1.0f, -1.0f, 2.0f).normalized(),
    };
    const Vec3 vectors[] = {
        Vec3(1.0f, 0.0f, 0.0f),
        Vec3(0.0f, 1.0f, 0.0f),
        Vec3(1.0f, 2.0f, 3.0f),
    };

    for (const Vec3& axis : axes) {
        for (int i = 1; i <= 6; ++i) {
            Quaternion q = Quaternion::from_axis_angle(axis, (PI / 12.0f) * i);
            Quaternion qi = q.inverse();
            ASSERT_TRUE((q * qi).approx_equal(Quaternion::identity(), 0.002f));

            for (const Vec3& vector : vectors) {
                Vec3 rotated = q.rotate(vector);
                Vec3 restored = qi.rotate(rotated);
                ASSERT_TRUE(restored.approx_equal(vector, 0.002f));
            }
        }
    }
}

void test_from_two_vectors_alignment_property() {
    struct Case {
        Vec3 from;
        Vec3 to;
    };

    const Case cases[] = {
        {Vec3::right(), Vec3::up()},
        {Vec3::forward(), Vec3::right()},
        {Vec3(1.0f, 2.0f, 0.0f), Vec3(-2.0f, 1.0f, 1.0f)},
        {Vec3(0.0f, 1.0f, 1.0f), Vec3(1.0f, 0.0f, -1.0f)},
    };

    for (const Case& test_case : cases) {
        Quaternion q = Quaternion::from_two_vectors(test_case.from, test_case.to);
        Vec3 rotated = q.rotate(test_case.from.normalized());
        ASSERT_TRUE(rotated.approx_equal(test_case.to.normalized(), 0.002f));
        ASSERT_FLOAT_EQ(q.norm(), 1.0f, EPS);
    }
}

void test_interpolation_stays_on_unit_sphere() {
    Quaternion a = Quaternion::identity();
    Quaternion b = Quaternion::from_axis_angle(Vec3(1.0f, 1.0f, 0.0f).normalized(), PI * 0.75f);

    for (int step = 0; step <= 10; ++step) {
        float t = static_cast<float>(step) / 10.0f;
        Quaternion slerped = Quaternion::slerp(a, b, t);
        Quaternion nlerped = Quaternion::nlerp(a, b, t);
        ASSERT_FLOAT_EQ(slerped.norm(), 1.0f, 0.002f);
        ASSERT_FLOAT_EQ(nlerped.norm(), 1.0f, 0.002f);
    }
}

int main() {
    std::cout << "=== QuatEngine Quaternion Property Tests ===" << std::endl;

    RUN_TEST(test_axis_angle_roundtrip_properties);
    RUN_TEST(test_rotation_inverse_property);
    RUN_TEST(test_from_two_vectors_alignment_property);
    RUN_TEST(test_interpolation_stays_on_unit_sphere);

    return TEST_REPORT();
}
