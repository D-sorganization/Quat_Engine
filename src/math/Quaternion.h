#pragma once
/**
 * @file Quaternion.h
 * @brief Quaternion class for 3D rotation with SLERP interpolation.
 *
 * Quaternions avoid gimbal lock and provide smooth rotation interpolation.
 * This implementation follows the Hamilton convention (w, x, y, z) where
 * q = w + xi + yj + zk.
 *
 * Key features:
 *   - Construction from axis-angle and Euler angles
 *   - Quaternion multiplication (composition of rotations)
 *   - SLERP (Spherical Linear Interpolation) for smooth rotation blending
 *   - NLERP (Normalized Linear Interpolation) as a fast approximation
 *   - Rotation of Vec3 points
 *   - Conversion to 4x4 rotation matrix
 *
 * Reference: "Quaternions and Rotation Sequences" by Jack B. Kuipers
 */

#include "Vec3.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace qe {
namespace math {

struct Quaternion {
    float w, x, y, z;

    // --- Constructors ---

    /** Identity quaternion (no rotation). */
    constexpr Quaternion() noexcept : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}

    /** Direct component construction. */
    constexpr Quaternion(float w, float x, float y, float z) noexcept
        : w(w), x(x), y(y), z(z) {}

    // --- Factory Methods ---

    /** Create from axis-angle representation.
     *  @param axis  Rotation axis (will be normalized).
     *  @param angle Rotation angle in radians.
     */
    static Quaternion from_axis_angle(const Vec3& axis, float angle) {
        Vec3 normalized_axis = axis.normalized();
        float half_angle = angle * 0.5f;
        float s = std::sin(half_angle);
        return {
            std::cos(half_angle),
            normalized_axis.x * s,
            normalized_axis.y * s,
            normalized_axis.z * s
        };
    }

    /** Create from Euler angles (yaw-pitch-roll, intrinsic ZYX convention).
     *  @param pitch Rotation around X axis (radians).
     *  @param yaw   Rotation around Y axis (radians).
     *  @param roll  Rotation around Z axis (radians).
     */
    static Quaternion from_euler(float pitch, float yaw, float roll) {
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);

        return {
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        };
    }

    /** Create a quaternion that rotates from direction 'from' to direction 'to'. */
    static Quaternion from_two_vectors(const Vec3& from, const Vec3& to) {
        Vec3 a = from.normalized();
        Vec3 b = to.normalized();

        float dot = a.dot(b);

        // Parallel vectors (same direction)
        if (dot > 0.999999f) {
            return identity();
        }

        // Anti-parallel vectors (opposite direction)
        if (dot < -0.999999f) {
            // Find an orthogonal axis
            Vec3 ortho = Vec3(1.0f, 0.0f, 0.0f).cross(a);
            if (ortho.length_squared() < 1e-6f) {
                ortho = Vec3(0.0f, 1.0f, 0.0f).cross(a);
            }
            ortho = ortho.normalized();
            return {0.0f, ortho.x, ortho.y, ortho.z};  // 180° rotation
        }

        Vec3 cross = a.cross(b);
        float w_val = 1.0f + dot;

        return Quaternion(w_val, cross.x, cross.y, cross.z).normalized();
    }

    /** Identity quaternion (no rotation). */
    static constexpr Quaternion identity() noexcept {
        return {1.0f, 0.0f, 0.0f, 0.0f};
    }

    // --- Core Operations ---

    /** Quaternion multiplication (Hamilton product).
     *  Composes rotations: (q1 * q2) applies q2 first, then q1.
     */
    constexpr Quaternion operator*(const Quaternion& rhs) const noexcept {
        return {
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
        };
    }

    /** Scalar multiplication. */
    constexpr Quaternion operator*(float scalar) const noexcept {
        return {w * scalar, x * scalar, y * scalar, z * scalar};
    }

    /** Quaternion addition (used internally for interpolation). */
    constexpr Quaternion operator+(const Quaternion& rhs) const noexcept {
        return {w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z};
    }

    /** Negation. */
    constexpr Quaternion operator-() const noexcept {
        return {-w, -x, -y, -z};
    }

    /** Conjugate: negates the vector part.
     *  For unit quaternions, conjugate == inverse.
     */
    constexpr Quaternion conjugate() const noexcept {
        return {w, -x, -y, -z};
    }

    /** Squared norm. */
    constexpr float norm_squared() const noexcept {
        return w * w + x * x + y * y + z * z;
    }

    /** Norm (magnitude). */
    float norm() const noexcept {
        return std::sqrt(norm_squared());
    }

    /** Returns a normalized (unit) quaternion. */
    Quaternion normalized() const {
        float n = norm();
        if (n < 1e-8f) {
            throw std::domain_error("Quaternion: cannot normalize zero quaternion");
        }
        float inv = 1.0f / n;
        return {w * inv, x * inv, y * inv, z * inv};
    }

    /** Inverse: conjugate / norm^2.
     *  For unit quaternions, inverse == conjugate.
     */
    Quaternion inverse() const {
        float ns = norm_squared();
        if (ns < 1e-8f) {
            throw std::domain_error("Quaternion: cannot invert zero quaternion");
        }
        float inv = 1.0f / ns;
        return {w * inv, -x * inv, -y * inv, -z * inv};
    }

    /** Dot product between quaternions. */
    constexpr float dot(const Quaternion& rhs) const noexcept {
        return w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z;
    }

    // --- Rotation ---

    /** Rotate a 3D point by this quaternion.
     *  v' = q * v * q^(-1)
     *  Optimized form avoids creating intermediate quaternions.
     */
    Vec3 rotate(const Vec3& v) const noexcept {
        // Rodrigues' rotation via quaternion:
        // t = 2 * (q_vec × v)
        // v' = v + w * t + q_vec × t
        Vec3 q_vec(x, y, z);
        Vec3 t = q_vec.cross(v) * 2.0f;
        return v + t * w + q_vec.cross(t);
    }

    // --- Euler Angle Extraction ---

    /** Extract pitch (X rotation) in radians. */
    float pitch() const noexcept {
        float sinp = 2.0f * (w * x + y * z);
        float cosp = 1.0f - 2.0f * (x * x + y * y);
        return std::atan2(sinp, cosp);
    }

    /** Extract yaw (Y rotation) in radians. */
    float yaw() const noexcept {
        float siny = 2.0f * (w * z + x * y);
        float cosy = 1.0f - 2.0f * (y * y + z * z);
        return std::atan2(siny, cosy);
    }

    /** Extract roll (Z rotation) in radians. */
    float roll() const noexcept {
        float sinr = 2.0f * (w * y - z * x);
        sinr = std::clamp(sinr, -1.0f, 1.0f);
        return std::asin(sinr);
    }

    // --- Interpolation ---

    /** Spherical Linear Interpolation (SLERP).
     *
     *  Produces constant-angular-velocity rotation between two orientations.
     *  This is the gold standard for smooth camera/character rotation blending.
     *
     *  @param a Start orientation (unit quaternion).
     *  @param b End orientation (unit quaternion).
     *  @param t Interpolation parameter [0, 1].
     *  @return  Interpolated unit quaternion.
     *
     *  Mathematical basis:
     *    slerp(a, b, t) = a * sin((1-t)θ) / sin(θ) + b * sin(tθ) / sin(θ)
     *    where θ = arccos(a · b)
     *
     *  Falls back to NLERP when quaternions are very close (sin(θ) ≈ 0)
     *  to avoid numerical instability.
     */
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        // Compute cosine of angle between quaternions
        float cos_theta = a.dot(b);

        // If dot product is negative, negate one quaternion to take the shorter arc.
        // Quaternions q and -q represent the same rotation, but SLERP would take
        // the long way around the 4D hypersphere without this correction.
        Quaternion b_adjusted = b;
        if (cos_theta < 0.0f) {
            b_adjusted = -b;
            cos_theta = -cos_theta;
        }

        // If quaternions are very close, use NLERP to avoid division by near-zero
        constexpr float SLERP_THRESHOLD = 0.9995f;
        if (cos_theta > SLERP_THRESHOLD) {
            return nlerp(a, b_adjusted, t);
        }

        // Standard SLERP formula
        float theta = std::acos(std::clamp(cos_theta, -1.0f, 1.0f));
        float sin_theta = std::sin(theta);

        float weight_a = std::sin((1.0f - t) * theta) / sin_theta;
        float weight_b = std::sin(t * theta) / sin_theta;

        return {
            a.w * weight_a + b_adjusted.w * weight_b,
            a.x * weight_a + b_adjusted.x * weight_b,
            a.y * weight_a + b_adjusted.y * weight_b,
            a.z * weight_a + b_adjusted.z * weight_b
        };
    }

    /** Normalized Linear Interpolation (NLERP).
     *
     *  Faster than SLERP but does not maintain constant angular velocity.
     *  Good enough for small angular differences or when performance matters.
     */
    static Quaternion nlerp(const Quaternion& a, const Quaternion& b, float t) {
        Quaternion b_adjusted = b;
        if (a.dot(b) < 0.0f) {
            b_adjusted = -b;
        }

        Quaternion result = {
            a.w + t * (b_adjusted.w - a.w),
            a.x + t * (b_adjusted.x - a.x),
            a.y + t * (b_adjusted.y - a.y),
            a.z + t * (b_adjusted.z - a.z)
        };
        return result.normalized();
    }

    // --- Conversion ---

    /** Convert to axis-angle representation.
     *  @return {axis, angle_in_radians}
     */
    std::pair<Vec3, float> to_axis_angle() const {
        Quaternion q = (w < 0.0f) ? Quaternion(-w, -x, -y, -z) : *this;

        float angle = 2.0f * std::acos(std::clamp(q.w, -1.0f, 1.0f));
        float s = std::sqrt(1.0f - q.w * q.w);

        if (s < 1e-6f) {
            return {Vec3(1.0f, 0.0f, 0.0f), 0.0f};  // No rotation
        }

        return {Vec3(q.x / s, q.y / s, q.z / s), angle};
    }

    /** Approximate equality within epsilon. */
    bool approx_equal(const Quaternion& other, float epsilon = 1e-5f) const noexcept {
        // Quaternions q and -q represent the same rotation
        float d1 = std::abs(w - other.w) + std::abs(x - other.x) +
                    std::abs(y - other.y) + std::abs(z - other.z);
        float d2 = std::abs(w + other.w) + std::abs(x + other.x) +
                    std::abs(y + other.y) + std::abs(z + other.z);
        return std::min(d1, d2) < epsilon;
    }
};

} // namespace math
} // namespace qe
