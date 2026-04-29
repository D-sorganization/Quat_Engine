// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file Vec3.h
 * @brief 3D Vector class for game mathematics.
 *
 * Provides a high-performance 3D vector with standard operations:
 * dot product, cross product, normalization, and linear interpolation.
 * Follows modern C++ (C++17) conventions with constexpr support.
 */

#include <cmath>
#include <stdexcept>

namespace qe {
namespace math {

struct Vec3 {
    float x, y, z;

    // --- Constructors ---
    constexpr Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vec3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

    // --- Arithmetic Operators ---
    constexpr Vec3 operator+(const Vec3& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    constexpr Vec3 operator-(const Vec3& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    constexpr Vec3 operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }

    /** Component-wise (Hadamard) multiplication. */
    constexpr Vec3 operator*(const Vec3& rhs) const noexcept {
        return {x * rhs.x, y * rhs.y, z * rhs.z};
    }

    constexpr Vec3 operator/(float scalar) const {
        if (scalar == 0.0f) {
            throw std::domain_error("Vec3: division by zero");
        }
        return {x / scalar, y / scalar, z / scalar};
    }

    constexpr Vec3 operator-() const noexcept {
        return {-x, -y, -z};
    }

    // --- Compound Assignment ---
    constexpr Vec3& operator+=(const Vec3& rhs) noexcept {
        x += rhs.x; y += rhs.y; z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept {
        x -= rhs.x; y -= rhs.y; z -= rhs.z;
        return *this;
    }

    constexpr Vec3& operator*=(float scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // --- Comparison ---
    constexpr bool operator==(const Vec3& rhs) const noexcept {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    constexpr bool operator!=(const Vec3& rhs) const noexcept {
        return !(*this == rhs);
    }

    // --- Vector Operations ---

    /** Compute the dot product (scalar product).
     *  Returns the sum of component-wise products: x*rhs.x + y*rhs.y + z*rhs.z.
     *  Useful for: angle between vectors (dot = |a||b|cos(theta)), projection, etc.
     *  @param rhs The right-hand vector
     *  @return Dot product scalar value
     *  @complexity O(1) - three multiplications and two additions
     */
    constexpr float dot(const Vec3& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /** Compute the cross product (vector product).
     *  Returns a vector perpendicular to both input vectors.
     *  Result magnitude = |a||b|sin(theta). Result direction follows right-hand rule.
     *  @param rhs The right-hand vector
     *  @return Cross product vector (perpendicular to both inputs)
     *  @complexity O(1) - six multiplications and three subtractions
     */
    constexpr Vec3 cross(const Vec3& rhs) const noexcept {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    /** Compute squared magnitude (length squared).
     *  Returns dot product with self: x*x + y*y + z*z.
     *  Prefer this when comparing distances to avoid sqrt overhead.
     *  @return Squared length (always non-negative)
     *  @complexity O(1) - three multiplications and two additions
     */
    constexpr float length_squared() const noexcept {
        return x * x + y * y + z * z;
    }

    /** Compute the magnitude (Euclidean length).
     *  Returns sqrt(x*x + y*y + z*z).
     *  For comparisons, prefer length_squared() to avoid sqrt cost.
     *  @return Length of the vector
     *  @complexity O(1) - sqrt operation (typically fast in modern hardware)
     */
    float length() const noexcept {
        return std::sqrt(length_squared());
    }

    /** Return a normalized copy of this vector.
     *  Divides all components by magnitude, resulting in unit length.
     *  Throws std::domain_error if vector is near zero (magnitude < 1e-8).
     *  @return Unit vector in the same direction
     *  @complexity O(1) - length computation + three divisions
     *  @throws std::domain_error if magnitude is effectively zero
     */
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-8f) {
            throw std::domain_error("Vec3: cannot normalize zero-length vector");
        }
        return *this / len;
    }

    /** Compute Euclidean distance to another point.
     *  Returns magnitude of (this - other).
     *  For distance comparisons, prefer distance_squared() to avoid sqrt.
     *  @param other The target point
     *  @return Distance from this point to other
     *  @complexity O(1) - vector subtraction + length computation
     */
    float distance_to(const Vec3& other) const noexcept {
        return (*this - other).length();
    }

    /** Linearly interpolate between two vectors.
     *  Returns this + t * (other - this). At t=0 returns this, at t=1 returns other.
     *  @param other The target vector
     *  @param t Interpolation factor (0=start, 1=end, typically 0-1 but unclamped)
     *  @return Interpolated vector
     *  @complexity O(1) - three component interpolations
     */
    constexpr Vec3 lerp(const Vec3& other, float t) const noexcept {
        return {
            x + t * (other.x - x),
            y + t * (other.y - y),
            z + t * (other.z - z)
        };
    }

    /** Check approximate equality within a tolerance.
     *  Compares each component with absolute tolerance epsilon.
     *  Useful for floating-point comparisons to handle rounding errors.
     *  @param other The vector to compare with
     *  @param epsilon Maximum allowed absolute difference per component (default: 1e-5)
     *  @return True if all components differ by at most epsilon, false otherwise
     *  @complexity O(1) - three comparisons with abs operations
     */
    bool approx_equal(const Vec3& other, float epsilon = 1e-5f) const noexcept {
        return std::abs(x - other.x) <= epsilon &&
               std::abs(y - other.y) <= epsilon &&
               std::abs(z - other.z) <= epsilon;
    }

    // --- Common Directions ---
    /** Origin point: (0, 0, 0). */
    static constexpr Vec3 zero()    noexcept { return {0.0f, 0.0f, 0.0f}; }

    /** All ones: (1, 1, 1). Used for uniform scaling. */
    static constexpr Vec3 one()     noexcept { return {1.0f, 1.0f, 1.0f}; }

    /** World up direction: (0, 1, 0). Y-axis in typical 3D engines. */
    static constexpr Vec3 up()      noexcept { return {0.0f, 1.0f, 0.0f}; }

    /** World down direction: (0, -1, 0). Opposite of up. */
    static constexpr Vec3 down()    noexcept { return {0.0f, -1.0f, 0.0f}; }

    /** Forward direction (into screen): (0, 0, -1). -Z in typical 3D conventions. */
    static constexpr Vec3 forward() noexcept { return {0.0f, 0.0f, -1.0f}; }

    /** Right direction: (1, 0, 0). +X axis. */
    static constexpr Vec3 right()   noexcept { return {1.0f, 0.0f, 0.0f}; }
};

/** Scalar-vector multiplication (commutative).
 *  Multiplies each component by the scalar: (scalar*v.x, scalar*v.y, scalar*v.z).
 *  @param scalar Scalar multiplicand
 *  @param v Vector multiplicand
 *  @return Scaled vector
 *  @complexity O(1) - three multiplications
 */
constexpr Vec3 operator*(float scalar, const Vec3& v) noexcept {
    return v * scalar;
}

} // namespace math
} // namespace qe
