// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file AABB.h
 * @brief Axis-Aligned Bounding Box with intersection tests.
 *
 * Supports:
 *   - AABB vs AABB overlap
 *   - Ray vs AABB intersection (for hitscan weapons)
 *   - Point containment
 *   - Construction from center + half-extents
 */

#include "../math/Vec3.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace qe {
namespace core {

struct AABB {
    math::Vec3 min{0, 0, 0};
    math::Vec3 max{0, 0, 0};

    AABB() = default;
    AABB(const math::Vec3& lo, const math::Vec3& hi) : min(lo), max(hi) {}

    /** Create an AABB from center point and half-extents.
     *  @param center Center point of the box
     *  @param half Half-extent vector (radius from center to edge in each axis)
     *  @return Axis-aligned bounding box
     *  @complexity O(1) - two vector subtractions and additions
     */
    static AABB from_center(const math::Vec3& center, const math::Vec3& half) {
        return {center - half, center + half};
    }

    /** Create an AABB from center point and uniform radius.
     *  Convenience constructor for cubic bounds (same extent in all axes).
     *  @param center Center point of the box
     *  @param half Uniform half-extent (radius in all directions)
     *  @return Cubic axis-aligned bounding box
     *  @complexity O(1) - vector construction, subtraction, and addition
     */
    static AABB from_center(const math::Vec3& center, float half) {
        math::Vec3 h(half, half, half);
        return {center - h, center + h};
    }

    /** Get the center point of this AABB.
     *  @return Midpoint between min and max
     *  @complexity O(1) - vector addition and scalar multiply
     */
    math::Vec3 center() const { return (min + max) * 0.5f; }

    /** Get the full size (width, height, depth) of this AABB.
     *  @return Extent in each axis (max - min)
     *  @complexity O(1) - vector subtraction
     */
    math::Vec3 size() const { return max - min; }

    /** Get the half-extents (distance from center to edge).
     *  @return Half-size in each direction
     *  @complexity O(1) - vector subtraction and scalar multiply
     */
    math::Vec3 half_extents() const { return (max - min) * 0.5f; }

    /** Test if a point is contained within this AABB (inclusive).
     *  Returns true if the point is inside or on the boundary.
     *  @param p Point to test
     *  @return True if point is inside or on the box, false otherwise
     *  @complexity O(1) - six comparisons
     */
    bool contains(const math::Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }

    /** Test if this AABB overlaps with another AABB.
     *  Uses the separating axis theorem (SAT) for axis-aligned boxes.
     *  @param other Other box to test
     *  @return True if boxes overlap (touching counts as overlap), false if separated
     *  @complexity O(1) - six comparisons
     */
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    /** Test ray-AABB intersection using the slab method.
     *  Efficiently determines if a ray hits this box and at what distance.
     *  Handles edge cases like parallel rays and negative scales.
     *  Used for hitscan weapons and projectile collision detection.
     *  @param origin  Ray starting point in world space
     *  @param dir     Ray direction (should be normalized for accurate t_out)
     *  @param t_out   Output: distance along ray to first intersection (0 = origin, 1 = first unit)
     *  @return True if ray intersects this AABB at a positive t_out, false otherwise
     *  @complexity O(1) - three slab tests with comparisons and division per axis
     */
    bool ray_intersect(const math::Vec3& origin, const math::Vec3& dir,
                       float& t_out) const {
        float tmin = -std::numeric_limits<float>::infinity();
        float tmax =  std::numeric_limits<float>::infinity();

        auto slab = [&](float o, float d, float lo, float hi) -> bool {
            if (std::abs(d) < 1e-8f) {
                // Ray parallel to slab
                return o >= lo && o <= hi;
            }
            float t1 = (lo - o) / d;
            float t2 = (hi - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            return tmin <= tmax;
        };

        if (!slab(origin.x, dir.x, min.x, max.x)) return false;
        if (!slab(origin.y, dir.y, min.y, max.y)) return false;
        if (!slab(origin.z, dir.z, min.z, max.z)) return false;

        t_out = tmin >= 0.0f ? tmin : tmax;
        return t_out >= 0.0f;
    }

    /** Get a transformed AABB by applying position and scale.
     *  Recomputes axis-aligned bounds after translation and non-uniform scaling.
     *  Uses conservative fitting: handles negative scales by computing min/max of scaled corners.
     *  Does NOT handle rotation (that would require rotating corners and recomputing bounds).
     *  @param pos Translation offset to apply
     *  @param scale Non-uniform scale factors for each axis
     *  @return New AABB in the transformed space (still axis-aligned)
     *  @complexity O(1) - three multiplications, two subtractions, six min/max operations
     */
    AABB transformed(const math::Vec3& pos, const math::Vec3& scale) const {
        math::Vec3 scaled_min = min * scale + pos;
        math::Vec3 scaled_max = max * scale + pos;
        // Ensure min < max after negative scales
        return {
            {std::min(scaled_min.x, scaled_max.x),
             std::min(scaled_min.y, scaled_max.y),
             std::min(scaled_min.z, scaled_max.z)},
            {std::max(scaled_min.x, scaled_max.x),
             std::max(scaled_min.y, scaled_max.y),
             std::max(scaled_min.z, scaled_max.z)}
        };
    }
};

} // namespace core
} // namespace qe
