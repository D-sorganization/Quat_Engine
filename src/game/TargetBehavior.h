#pragma once
/**
 * @file TargetBehavior.h
 * @brief AI target movement behaviors driven by quaternion SLERP interpolation.
 *
 * This is the flagship feature of QuatEngine, demonstrating quaternion math
 * in action through a variety of enemy movement patterns. Every behavior
 * computes both position and facing direction using quaternion operations,
 * showcasing SLERP, axis-angle rotation, and quaternion composition.
 *
 * Supported behaviors:
 *   - Static:   No movement (baseline)
 *   - Orbit:    Circular path via quaternion rotation of a radius vector
 *   - Figure8:  Horizontal orbit + vertical oscillation (dual quaternion composition)
 *   - Zigzag:   Oscillating position with SLERP-blended facing direction
 *   - Spiral:   Orbit with time-varying radius
 *   - Patrol:   Waypoint traversal with SLERP-smoothed turning between legs
 *   - Dodge:    Random quaternion-based lateral dodge when alerted
 */

#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <cmath>
#include <cstdlib>

namespace qe {
namespace game {

// Use same PI as Scene.h (inline avoids ODR issues across headers)
inline constexpr float TB_PI = 3.14159265358979f;

enum class BehaviorType {
    Static,
    Orbit,
    Figure8,
    Zigzag,
    Spiral,
    Patrol,
    Dodge,
};

struct TargetBehavior {
    struct PatrolProgress {
        math::Quaternion rotation_from;
        math::Quaternion rotation_to;
        float fraction = 0.0f;
    };

    BehaviorType type = BehaviorType::Static;
    float speed = 1.0f;
    float phase = 0.0f;
    math::Vec3 center;
    float radius = 5.0f;
    float amplitude = 3.0f;
    math::Quaternion base_orientation;
    math::Quaternion facing;
    float alert_timer = 0.0f;
    bool is_alerted = false;

    // Internal state for dodge
    math::Vec3 dodge_offset;

private:
    static TargetBehavior make_behavior(
        BehaviorType type,
        math::Vec3 center,
        float speed,
        float phase,
        float radius = 5.0f,
        float amplitude = 3.0f
    ) {
        TargetBehavior b;
        b.type = type;
        b.center = center;
        b.speed = speed;
        b.phase = phase;
        b.radius = radius;
        b.amplitude = amplitude;
        b.base_orientation = math::Quaternion::identity();
        b.facing = math::Quaternion::identity();
        b.is_alerted = false;
        b.alert_timer = 0.0f;
        b.dodge_offset = math::Vec3::zero();
        return b;
    }

    static PatrolProgress compute_patrol_progress(
        float time,
        float speed,
        float phase
    ) {
        constexpr int waypoint_count = 5;
        float total = std::fmod(time * speed + phase, static_cast<float>(waypoint_count));
        if (total < 0.0f) {
            total += static_cast<float>(waypoint_count);
        }

        int current = static_cast<int>(total);
        if (current >= waypoint_count) {
            current = waypoint_count - 1;
        }

        int next = (current + 1) % waypoint_count;
        float fraction = total - static_cast<float>(current);
        float step = 2.0f * TB_PI / static_cast<float>(waypoint_count);

        PatrolProgress progress;
        progress.rotation_from = math::Quaternion::from_axis_angle(
            math::Vec3::up(),
            step * static_cast<float>(current)
        );
        progress.rotation_to = math::Quaternion::from_axis_angle(
            math::Vec3::up(),
            step * static_cast<float>(next)
        );
        progress.fraction = fraction;
        return progress;
    }

public:

    // --- Factory Methods ---

    static TargetBehavior create_orbit(math::Vec3 center, float radius,
                                       float speed, float phase) {
        return make_behavior(BehaviorType::Orbit, center, speed, phase, radius);
    }

    static TargetBehavior create_figure8(math::Vec3 center, float radius,
                                         float speed, float phase) {
        return make_behavior(BehaviorType::Figure8, center, speed, phase, radius);
    }

    static TargetBehavior create_zigzag(math::Vec3 center, float amplitude,
                                        float speed, float phase) {
        return make_behavior(
            BehaviorType::Zigzag,
            center,
            speed,
            phase,
            5.0f,
            amplitude
        );
    }

    static TargetBehavior create_spiral(math::Vec3 center, float radius,
                                        float speed, float phase) {
        return make_behavior(BehaviorType::Spiral, center, speed, phase, radius);
    }

    static TargetBehavior create_patrol(math::Vec3 center, float radius,
                                        float speed, float phase) {
        return make_behavior(BehaviorType::Patrol, center, speed, phase, radius);
    }

    static TargetBehavior create_dodge(math::Vec3 center, float speed) {
        TargetBehavior b = make_behavior(
            BehaviorType::Dodge,
            center,
            speed,
            0.0f,
            3.0f
        );
        return b;
    }

    // --- Core Methods ---

    /** Compute the world-space position for this behavior at a given time. */
    math::Vec3 compute_position(float time) const {
        switch (type) {
        case BehaviorType::Static:
            return center;

        case BehaviorType::Orbit: {
            float t = time * speed + phase;
            math::Quaternion q = math::Quaternion::from_axis_angle(
                math::Vec3::up(), t);
            return center + q.rotate(math::Vec3(radius, 0.0f, 0.0f));
        }

        case BehaviorType::Figure8: {
            float t = time * speed + phase;
            math::Quaternion q_horiz = math::Quaternion::from_axis_angle(
                math::Vec3::up(), t);
            math::Vec3 base_pos = q_horiz.rotate(math::Vec3(radius, 0.0f, 0.0f));
            base_pos.y += std::sin(2.0f * t) * radius * 0.5f;
            return center + base_pos;
        }

        case BehaviorType::Zigzag: {
            float t = time * speed + phase;
            math::Vec3 offset(
                std::sin(t) * amplitude,
                0.0f,
                std::cos(t * 0.5f) * amplitude * 0.3f);
            return center + offset;
        }

        case BehaviorType::Spiral: {
            float t = time * speed + phase;
            float r = radius * (0.5f + 0.5f * std::sin(t * 0.3f));
            math::Quaternion q = math::Quaternion::from_axis_angle(
                math::Vec3::up(), t);
            return center + q.rotate(math::Vec3(r, std::sin(t * 2.0f) * 0.5f, 0.0f));
        }

        case BehaviorType::Patrol: {
            PatrolProgress progress = compute_patrol_progress(time, speed, phase);
            math::Vec3 pos_a = center +
                progress.rotation_from.rotate(math::Vec3(radius, 0.0f, 0.0f));
            math::Vec3 pos_b = center +
                progress.rotation_to.rotate(math::Vec3(radius, 0.0f, 0.0f));

            return pos_a.lerp(pos_b, progress.fraction);
        }

        case BehaviorType::Dodge: {
            if (is_alerted && alert_timer > 0.0f) {
                return center + dodge_offset;
            }
            return center;
        }

        default:
            return center;
        }
    }

    /** Compute the facing quaternion for this behavior at a given time.
     *  Uses SLERP to produce smooth, continuous rotations.
     */
    math::Quaternion compute_rotation(float time) const {
        switch (type) {
        case BehaviorType::Static:
            return base_orientation;

        case BehaviorType::Orbit: {
            // Face tangent to orbit: the derivative of position at angle t
            // is (-sin(t), 0, -cos(t)), which matches rotating forward (-Z)
            // by angle t around Y.
            float t = time * speed + phase;
            return math::Quaternion::from_axis_angle(
                math::Vec3::up(), t);
        }

        case BehaviorType::Figure8: {
            // SLERP between forward tangent quaternions at two nearby times
            // to produce a smooth facing direction along the figure-8 path
            float t = time * speed + phase;
            float dt = 0.01f;

            math::Quaternion q_now = math::Quaternion::from_axis_angle(
                math::Vec3::up(), t);
            math::Quaternion q_next = math::Quaternion::from_axis_angle(
                math::Vec3::up(), t + dt);

            // Blend the horizontal rotation with a pitch component from
            // the vertical oscillation
            float vert_slope = 2.0f * std::cos(2.0f * t);
            math::Quaternion pitch = math::Quaternion::from_axis_angle(
                math::Vec3::right(), std::atan(vert_slope * radius * 0.5f) * 0.1f);

            math::Quaternion base = math::Quaternion::slerp(q_now, q_next, 0.5f);
            return (base * pitch).normalized();
        }

        case BehaviorType::Zigzag: {
            // SLERP between two facing orientations based on oscillation
            float t = time * speed + phase;
            math::Quaternion a = math::Quaternion::from_axis_angle(
                math::Vec3::up(), -0.5f);
            math::Quaternion b = math::Quaternion::from_axis_angle(
                math::Vec3::up(), 0.5f);
            float blend = std::sin(t) * 0.5f + 0.5f;
            return math::Quaternion::slerp(a, b, blend);
        }

        case BehaviorType::Spiral: {
            float t = time * speed + phase;
            return math::Quaternion::from_axis_angle(
                math::Vec3::up(), t + TB_PI * 0.5f);
        }

        case BehaviorType::Patrol: {
            PatrolProgress progress = compute_patrol_progress(time, speed, phase);
            return math::Quaternion::slerp(
                progress.rotation_from,
                progress.rotation_to,
                progress.fraction
            );
        }

        case BehaviorType::Dodge: {
            if (is_alerted && alert_timer > 0.0f) {
                // Face away from dodge direction
                if (dodge_offset.length_squared() > 1e-6f) {
                    math::Vec3 dodge_dir = dodge_offset.normalized();
                    float yaw = std::atan2(dodge_dir.x, dodge_dir.z);
                    return math::Quaternion::from_axis_angle(
                        math::Vec3::up(), yaw);
                }
            }
            return base_orientation;
        }

        default:
            return base_orientation;
        }
    }

    /** Trigger the dodge behavior (e.g., target is being shot at). */
    void alert() {
        is_alerted = true;
        alert_timer = 1.0f;

        // Generate a random perpendicular dodge using quaternion rotation.
        // Pick a random angle and rotate a forward offset around the up axis.
        float random_angle = static_cast<float>(std::rand() % 628) / 100.0f;
        math::Quaternion dodge_rot = math::Quaternion::from_axis_angle(
            math::Vec3::up(), random_angle);
        dodge_offset = dodge_rot.rotate(math::Vec3(radius, 0.0f, 0.0f));
    }

    /** Update time-dependent internal state (alert cooldown, etc.).
     *  @param dt Delta time in seconds.
     */
    void update(float dt) {
        if (is_alerted) {
            alert_timer -= dt;
            if (alert_timer <= 0.0f) {
                alert_timer = 0.0f;
                is_alerted = false;
                dodge_offset = math::Vec3::zero();
            }
        }
    }
};

} // namespace game
} // namespace qe
