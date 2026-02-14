#pragma once
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

    /** Create from center point and half-extents. */
    static AABB from_center(const math::Vec3& center, const math::Vec3& half) {
        return {center - half, center + half};
    }

    /** Create from center and uniform radius. */
    static AABB from_center(const math::Vec3& center, float half) {
        math::Vec3 h(half, half, half);
        return {center - h, center + h};
    }

    math::Vec3 center() const { return (min + max) * 0.5f; }
    math::Vec3 size() const { return max - min; }
    math::Vec3 half_extents() const { return (max - min) * 0.5f; }

    /** Test if a point is inside this AABB. */
    bool contains(const math::Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }

    /** Test overlap with another AABB. */
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    /**
     * Ray-AABB intersection test (slab method).
     * @param origin  Ray origin.
     * @param dir     Ray direction (normalized).
     * @param t_out   Output: distance along ray to first hit.
     * @return true if ray intersects this AABB.
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

    /** Get a transformed AABB (re-fits after rotation/scale — conservative). */
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
