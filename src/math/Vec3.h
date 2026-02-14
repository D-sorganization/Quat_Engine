#pragma once
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

    /** Dot product. */
    constexpr float dot(const Vec3& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /** Cross product. */
    constexpr Vec3 cross(const Vec3& rhs) const noexcept {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    /** Squared magnitude (avoids sqrt). */
    constexpr float length_squared() const noexcept {
        return x * x + y * y + z * z;
    }

    /** Magnitude. */
    float length() const noexcept {
        return std::sqrt(length_squared());
    }

    /** Returns a normalized copy. Throws if zero-length. */
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-8f) {
            throw std::domain_error("Vec3: cannot normalize zero-length vector");
        }
        return *this / len;
    }

    /** Distance between two points. */
    float distance_to(const Vec3& other) const noexcept {
        return (*this - other).length();
    }

    /** Linear interpolation: this + t * (other - this). */
    constexpr Vec3 lerp(const Vec3& other, float t) const noexcept {
        return {
            x + t * (other.x - x),
            y + t * (other.y - y),
            z + t * (other.z - z)
        };
    }

    /** Approximate equality within epsilon. */
    bool approx_equal(const Vec3& other, float epsilon = 1e-5f) const noexcept {
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
    }

    // --- Common Directions ---
    static constexpr Vec3 zero()    noexcept { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 one()     noexcept { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Vec3 up()      noexcept { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vec3 down()    noexcept { return {0.0f, -1.0f, 0.0f}; }
    static constexpr Vec3 forward() noexcept { return {0.0f, 0.0f, -1.0f}; }
    static constexpr Vec3 right()   noexcept { return {1.0f, 0.0f, 0.0f}; }
};

/** Scalar * Vec3 (commutative). */
constexpr Vec3 operator*(float scalar, const Vec3& v) noexcept {
    return v * scalar;
}

} // namespace math
} // namespace qe
