#pragma once
/**
 * @file Camera.h
 * @brief Quaternion-based FPS camera with smooth SLERP rotation.
 *
 * Unlike traditional Euler-angle cameras, this camera stores orientation
 * as a quaternion, eliminating gimbal lock and enabling smooth interpolation.
 *
 * Mouse input → axis-angle → quaternion composition → view matrix.
 * Optional SLERP smoothing for cinematic camera movement.
 */

#include "../core/Transform.h"
#include "../math/Mat4.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <algorithm>
#include <cmath>

namespace qe {
namespace renderer {

class Camera {
public:
    /** Camera configuration. */
    struct Config {
        float fov_y       = 1.0472f;   // 60° in radians
        float aspect      = 16.0f / 9.0f;
        float near_z      = 0.1f;
        float far_z       = 100.0f;
        float sensitivity  = 0.003f;   // Mouse sensitivity (radians/pixel)
        float move_speed   = 5.0f;     // Units per second
        float smoothing    = 0.0f;     // SLERP factor (0 = instant, 0.9 = very smooth)
        float max_pitch    = 1.4f;     // ~80° max look up/down
    };

    Config config;

    Camera() = default;

    explicit Camera(const Config& cfg) : config(cfg) {}

    // --- Input Processing ---

    /** Process mouse movement (delta pixels). Updates quaternion orientation. */
    void process_mouse(float dx, float dy) {
        // Yaw: rotate around world Y axis (left/right)
        float yaw_angle = -dx * config.sensitivity;
        math::Quaternion yaw_rotation =
            math::Quaternion::from_axis_angle(math::Vec3::up(), yaw_angle);

        // Pitch: rotate around local X axis (up/down)
        float pitch_angle = -dy * config.sensitivity;
        accumulated_pitch_ += pitch_angle;
        accumulated_pitch_ = std::clamp(accumulated_pitch_,
                                         -config.max_pitch, config.max_pitch);

        // Reconstruct orientation from yaw + clamped pitch
        // Apply yaw to the stored yaw quaternion
        yaw_quat_ = (yaw_rotation * yaw_quat_).normalized();

        // Build pitch quaternion from accumulated pitch
        math::Quaternion pitch_quat =
            math::Quaternion::from_axis_angle(math::Vec3::right(), accumulated_pitch_);

        // Target orientation = yaw * pitch (yaw first, then pitch in local space)
        target_orientation_ = (yaw_quat_ * pitch_quat).normalized();
    }

    /** Process keyboard movement (unit direction, will be scaled by dt). */
    void process_movement(float forward, float right, float up, float dt) {
        math::Vec3 move_dir = math::Vec3::zero();

        // Forward/backward in the XZ plane (ignore pitch for ground movement)
        math::Vec3 flat_forward = yaw_quat_.rotate(math::Vec3::forward());
        flat_forward.y = 0.0f;
        if (flat_forward.length_squared() > 1e-6f) {
            flat_forward = flat_forward.normalized();
        }

        math::Vec3 flat_right = yaw_quat_.rotate(math::Vec3::right());
        flat_right.y = 0.0f;
        if (flat_right.length_squared() > 1e-6f) {
            flat_right = flat_right.normalized();
        }

        move_dir += flat_forward * forward;
        move_dir += flat_right * right;
        move_dir += math::Vec3::up() * up;

        if (move_dir.length_squared() > 1e-6f) {
            move_dir = move_dir.normalized();
        }

        position_ += move_dir * config.move_speed * dt;
    }

    /** Update camera state (call once per frame).
     *  Applies SLERP smoothing to orientation if configured.
     */
    void update(float dt) {
        if (config.smoothing > 0.0f) {
            // SLERP toward target orientation
            float t = 1.0f - std::pow(config.smoothing, dt * 60.0f);
            current_orientation_ =
                math::Quaternion::slerp(current_orientation_, target_orientation_, t);
        } else {
            current_orientation_ = target_orientation_;
        }
    }

    // --- Output ---

    /** Get the view matrix for rendering. */
    math::Mat4 view_matrix() const {
        // Camera looks along -Z in its local space
        math::Vec3 forward = current_orientation_.rotate(math::Vec3::forward());
        math::Vec3 target = position_ + forward;
        return math::Mat4::look_at(position_, target, math::Vec3::up());
    }

    /** Get the projection matrix. */
    math::Mat4 projection_matrix() const {
        return math::Mat4::perspective(config.fov_y, config.aspect,
                                       config.near_z, config.far_z);
    }

    /** Get the combined view-projection matrix. */
    math::Mat4 vp_matrix() const {
        return projection_matrix() * view_matrix();
    }

    // --- Accessors ---

    const math::Vec3& position() const noexcept { return position_; }
    const math::Quaternion& orientation() const noexcept { return current_orientation_; }

    void set_position(const math::Vec3& pos) noexcept { position_ = pos; }

    math::Vec3 forward() const noexcept {
        return current_orientation_.rotate(math::Vec3::forward());
    }

    math::Vec3 right_dir() const noexcept {
        return current_orientation_.rotate(math::Vec3::right());
    }

private:
    math::Vec3 position_{0.0f, 1.5f, 5.0f};  // Start slightly above ground, back

    // Orientation decomposed into yaw + pitch for clamping
    math::Quaternion yaw_quat_ = math::Quaternion::identity();
    float accumulated_pitch_ = 0.0f;

    // Target and current (for SLERP smoothing)
    math::Quaternion target_orientation_ = math::Quaternion::identity();
    math::Quaternion current_orientation_ = math::Quaternion::identity();
};

} // namespace renderer
} // namespace qe
