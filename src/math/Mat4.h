#pragma once
/**
 * @file Mat4.h
 * @brief 4x4 Matrix class for 3D transformations.
 *
 * Column-major layout (OpenGL convention) for direct GPU upload.
 * Provides perspective projection, look-at view matrix, and
 * TRS (Translation-Rotation-Scale) model matrix construction.
 */

#include "Quaternion.h"
#include "Vec3.h"

#include <array>
#include <cmath>

namespace qe {
namespace math {

struct Mat4 {
    /**
     * Column-major storage: m[col][row].
     * This matches OpenGL/Vulkan expectations for glUniformMatrix4fv.
     *
     * Layout:  m[0][0] m[1][0] m[2][0] m[3][0]
     *          m[0][1] m[1][1] m[2][1] m[3][1]
     *          m[0][2] m[1][2] m[2][2] m[3][2]
     *          m[0][3] m[1][3] m[2][3] m[3][3]
     */
    std::array<std::array<float, 4>, 4> m{};

    // --- Constructors ---

    /** Zero matrix. */
    Mat4() = default;

    /** Identity matrix. */
    static Mat4 identity() noexcept {
        Mat4 result;
        result.m[0][0] = 1.0f;
        result.m[1][1] = 1.0f;
        result.m[2][2] = 1.0f;
        result.m[3][3] = 1.0f;
        return result;
    }

    // --- Transform Factories ---

    /** Translation matrix. */
    static Mat4 translation(const Vec3& t) noexcept {
        Mat4 result = identity();
        result.m[3][0] = t.x;
        result.m[3][1] = t.y;
        result.m[3][2] = t.z;
        return result;
    }

    /** Scale matrix. */
    static Mat4 scale(const Vec3& s) noexcept {
        Mat4 result;
        result.m[0][0] = s.x;
        result.m[1][1] = s.y;
        result.m[2][2] = s.z;
        result.m[3][3] = 1.0f;
        return result;
    }

    /** Rotation matrix from quaternion. */
    static Mat4 rotation(const Quaternion& q) noexcept {
        Mat4 result;

        float xx = q.x * q.x;
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        result.m[0][0] = 1.0f - 2.0f * (yy + zz);
        result.m[0][1] = 2.0f * (xy + wz);
        result.m[0][2] = 2.0f * (xz - wy);

        result.m[1][0] = 2.0f * (xy - wz);
        result.m[1][1] = 1.0f - 2.0f * (xx + zz);
        result.m[1][2] = 2.0f * (yz + wx);

        result.m[2][0] = 2.0f * (xz + wy);
        result.m[2][1] = 2.0f * (yz - wx);
        result.m[2][2] = 1.0f - 2.0f * (xx + yy);

        result.m[3][3] = 1.0f;
        return result;
    }

    /** Model matrix: Translation * Rotation * Scale (TRS order). */
    static Mat4 trs(const Vec3& pos, const Quaternion& rot,
                    const Vec3& scl) noexcept {
        Mat4 t = translation(pos);
        Mat4 r = rotation(rot);
        Mat4 s = scale(scl);
        return t * r * s;
    }

    // --- Camera Matrices ---

    /** Perspective projection matrix.
     *  @param fov_y   Vertical field of view in radians.
     *  @param aspect  Width / height ratio.
     *  @param near_z  Near clipping plane distance.
     *  @param far_z   Far clipping plane distance.
     */
    static Mat4 perspective(float fov_y, float aspect,
                            float near_z, float far_z) noexcept {
        Mat4 result;
        float tan_half = std::tan(fov_y * 0.5f);

        result.m[0][0] = 1.0f / (aspect * tan_half);
        result.m[1][1] = 1.0f / tan_half;
        result.m[2][2] = -(far_z + near_z) / (far_z - near_z);
        result.m[2][3] = -1.0f;
        result.m[3][2] = -(2.0f * far_z * near_z) / (far_z - near_z);
        return result;
    }

    /** Look-at view matrix.
     *  @param eye    Camera position.
     *  @param target Point to look at.
     *  @param world_up  World up direction (typically {0,1,0}).
     */
    static Mat4 look_at(const Vec3& eye, const Vec3& target,
                        const Vec3& world_up) {
        Vec3 zaxis = (eye - target).normalized();     // Forward (camera looks -Z)
        Vec3 xaxis = world_up.cross(zaxis).normalized();  // Right
        Vec3 yaxis = zaxis.cross(xaxis);                   // Up

        Mat4 result;
        result.m[0][0] = xaxis.x;
        result.m[0][1] = yaxis.x;
        result.m[0][2] = zaxis.x;

        result.m[1][0] = xaxis.y;
        result.m[1][1] = yaxis.y;
        result.m[1][2] = zaxis.y;

        result.m[2][0] = xaxis.z;
        result.m[2][1] = yaxis.z;
        result.m[2][2] = zaxis.z;

        result.m[3][0] = -xaxis.dot(eye);
        result.m[3][1] = -yaxis.dot(eye);
        result.m[3][2] = -zaxis.dot(eye);
        result.m[3][3] = 1.0f;

        return result;
    }

    // --- Matrix Multiplication ---

    /** 4x4 matrix multiplication. */
    Mat4 operator*(const Mat4& rhs) const noexcept {
        Mat4 result;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result.m[col][row] =
                    m[0][row] * rhs.m[col][0] +
                    m[1][row] * rhs.m[col][1] +
                    m[2][row] * rhs.m[col][2] +
                    m[3][row] * rhs.m[col][3];
            }
        }
        return result;
    }

    /** Transform a Vec3 as a point (w=1, applies translation). */
    Vec3 transform_point(const Vec3& v) const noexcept {
        float rx = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0];
        float ry = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1];
        float rz = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2];
        float rw = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3];

        if (std::abs(rw) > 1e-8f) {
            return {rx / rw, ry / rw, rz / rw};
        }
        return {rx, ry, rz};
    }

    /** Transform a Vec3 as a direction (w=0, ignores translation). */
    Vec3 transform_direction(const Vec3& v) const noexcept {
        return {
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
            m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z
        };
    }

    /** Get a pointer to the raw float data (for OpenGL upload). */
    const float* data() const noexcept {
        return &m[0][0];
    }
};

} // namespace math
} // namespace qe
