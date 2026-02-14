#pragma once
/**
 * @file Camera.h
 * @brief Dual-mode camera system: FPS (first-person) and TPS (third-person orbit).
 *
 * Both modes use quaternion rotation exclusively:
 *   - FPS: Mouse → axis-angle → quaternion composition (no Euler decomposition)
 *   - TPS: Orbit yaw/pitch → quaternion → position on sphere around target
 *
 * SLERP smoothing is applied to both camera orientation and TPS orbit position,
 * demonstrating quaternion interpolation in a real game camera system.
 */

#include "../core/Transform.h"
#include "../math/Mat4.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"

#include <algorithm>
#include <cmath>

namespace qe {
namespace renderer {

enum class CameraMode { FirstPerson, ThirdPerson };

class Camera {
public:
    /** Camera configuration. */
    struct Config {
        // Shared
        float fov_y         = 1.0472f;   // 60° in radians
        float aspect        = 16.0f / 9.0f;
        float near_z        = 0.1f;
        float far_z         = 200.0f;
        float sensitivity   = 0.003f;    // Mouse sensitivity (radians/pixel)
        float smoothing     = 0.0f;      // SLERP factor (0=instant, 0.9=very smooth)
        float max_pitch     = 1.4f;      // ~80° max look up/down

        // FPS movement
        float move_speed    = 5.0f;      // Walk speed (units/sec)
        float sprint_mult   = 2.2f;      // Sprint multiplier
        float acceleration  = 20.0f;     // Movement acceleration
        float deceleration  = 12.0f;     // Movement deceleration (friction)
        float head_bob_amp  = 0.04f;     // Head bob amplitude
        float head_bob_freq = 8.0f;      // Head bob frequency

        // TPS orbit
        float orbit_distance = 6.0f;     // Distance from target
        float orbit_min_dist = 2.0f;     // Minimum zoom distance
        float orbit_max_dist = 20.0f;    // Maximum zoom distance
        float orbit_height   = 1.5f;     // Height offset above target
        float zoom_speed     = 1.5f;     // Scroll wheel zoom speed
        float orbit_smoothing = 0.88f;   // Orbit position SLERP factor
    };

    Config config;

    Camera() = default;
    explicit Camera(const Config& cfg) : config(cfg) {}

    // --- Mode Switching ---

    CameraMode mode() const noexcept { return mode_; }

    void set_mode(CameraMode new_mode) noexcept {
        if (mode_ == new_mode) return;
        mode_ = new_mode;

        if (new_mode == CameraMode::ThirdPerson) {
            // Transfer FPS orientation to TPS orbit angles
            orbit_yaw_ = yaw_angle_;
            orbit_pitch_ = std::clamp(accumulated_pitch_, -config.max_pitch, config.max_pitch);
        } else {
            // Transfer TPS orbit angles back to FPS orientation
            yaw_angle_ = orbit_yaw_;
            accumulated_pitch_ = orbit_pitch_;
            rebuild_fps_orientation();
        }
    }

    void toggle_mode() noexcept {
        set_mode(mode_ == CameraMode::FirstPerson
                     ? CameraMode::ThirdPerson
                     : CameraMode::FirstPerson);
    }

    // --- Input Processing ---

    /** Process mouse movement (delta pixels). */
    void process_mouse(float dx, float dy) {
        if (mode_ == CameraMode::FirstPerson) {
            process_mouse_fps(dx, dy);
        } else {
            process_mouse_tps(dx, dy);
        }
    }

    /** Process scroll wheel (for TPS zoom). */
    void process_scroll(float delta) {
        if (mode_ == CameraMode::ThirdPerson) {
            config.orbit_distance -= delta * config.zoom_speed;
            config.orbit_distance = std::clamp(
                config.orbit_distance, config.orbit_min_dist, config.orbit_max_dist);
        }
    }

    /** Process keyboard movement with sprint support. */
    void process_movement(float forward, float right, float up,
                          bool sprinting, float dt) {
        float target_speed = config.move_speed * (sprinting ? config.sprint_mult : 1.0f);

        // Build target velocity
        math::Vec3 input_dir = math::Vec3::zero();

        // Forward/backward in the XZ plane (ignore pitch)
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

        input_dir += flat_forward * forward;
        input_dir += flat_right * right;
        input_dir += math::Vec3::up() * up;

        math::Vec3 target_vel = math::Vec3::zero();
        if (input_dir.length_squared() > 1e-6f) {
            target_vel = input_dir.normalized() * target_speed;
            is_moving_ = true;
        } else {
            is_moving_ = false;
        }

        // Smooth acceleration / deceleration
        float accel = is_moving_ ? config.acceleration : config.deceleration;
        velocity_ = velocity_.lerp(target_vel, std::min(1.0f, accel * dt));

        // Apply velocity
        if (mode_ == CameraMode::FirstPerson) {
            position_ += velocity_ * dt;
        } else {
            // TPS: movement controls the target, not the camera directly
            tps_target_ += velocity_ * dt;
        }

        // Head bob (FPS only, while moving)
        if (mode_ == CameraMode::FirstPerson && is_moving_ && sprinting) {
            head_bob_timer_ += dt * config.head_bob_freq * 1.5f;
        } else if (is_moving_) {
            head_bob_timer_ += dt * config.head_bob_freq;
        } else {
            head_bob_timer_ *= 0.9f;  // Fade out
        }
    }

    /** Update camera state (call once per frame). */
    void update(float dt) {
        if (mode_ == CameraMode::FirstPerson) {
            update_fps(dt);
        } else {
            update_tps(dt);
        }
    }

    // --- Output ---

    math::Mat4 view_matrix() const {
        math::Vec3 cam_pos = effective_position();
        math::Vec3 forward_dir = current_orientation_.rotate(math::Vec3::forward());
        math::Vec3 target = cam_pos + forward_dir;
        return math::Mat4::look_at(cam_pos, target, math::Vec3::up());
    }

    math::Mat4 projection_matrix() const {
        return math::Mat4::perspective(config.fov_y, config.aspect,
                                       config.near_z, config.far_z);
    }

    math::Mat4 vp_matrix() const {
        return projection_matrix() * view_matrix();
    }

    // --- Accessors ---

    math::Vec3 position() const noexcept { return effective_position(); }
    const math::Quaternion& orientation() const noexcept { return current_orientation_; }

    void set_position(const math::Vec3& pos) noexcept {
        position_ = pos;
        tps_target_ = pos;
    }

    /** Get the TPS target position (what the camera orbits around). */
    const math::Vec3& tps_target() const noexcept { return tps_target_; }

    math::Vec3 forward() const noexcept {
        return current_orientation_.rotate(math::Vec3::forward());
    }

    math::Vec3 right_dir() const noexcept {
        return current_orientation_.rotate(math::Vec3::right());
    }

    /** Is the player currently moving? (for animation purposes) */
    bool is_moving() const noexcept { return is_moving_; }

    /** Current movement speed magnitude. */
    float current_speed() const noexcept { return velocity_.length(); }

private:
    CameraMode mode_ = CameraMode::FirstPerson;

    // Shared state
    math::Vec3 position_{0.0f, 1.5f, 5.0f};
    math::Vec3 velocity_{0.0f, 0.0f, 0.0f};
    math::Quaternion current_orientation_ = math::Quaternion::identity();
    bool is_moving_ = false;

    // FPS state
    math::Quaternion yaw_quat_ = math::Quaternion::identity();
    math::Quaternion target_orientation_ = math::Quaternion::identity();
    float accumulated_pitch_ = 0.0f;
    float yaw_angle_ = 0.0f;
    float head_bob_timer_ = 0.0f;

    // TPS state
    math::Vec3 tps_target_{0.0f, 1.0f, 0.0f};
    math::Vec3 tps_current_pos_{0.0f, 3.0f, 6.0f};
    float orbit_yaw_ = 0.0f;
    float orbit_pitch_ = 0.3f;  // Slight downward look

    // --- FPS Implementation ---

    void process_mouse_fps(float dx, float dy) {
        float yaw_delta = -dx * config.sensitivity;
        yaw_angle_ += yaw_delta;

        accumulated_pitch_ += -dy * config.sensitivity;
        accumulated_pitch_ = std::clamp(accumulated_pitch_,
                                         -config.max_pitch, config.max_pitch);
        rebuild_fps_orientation();
    }

    void rebuild_fps_orientation() {
        yaw_quat_ = math::Quaternion::from_axis_angle(
            math::Vec3::up(), yaw_angle_);
        math::Quaternion pitch_quat = math::Quaternion::from_axis_angle(
            math::Vec3::right(), accumulated_pitch_);
        target_orientation_ = (yaw_quat_ * pitch_quat).normalized();
    }

    void update_fps(float dt) {
        if (config.smoothing > 0.0f) {
            float t = 1.0f - std::pow(config.smoothing, dt * 60.0f);
            current_orientation_ = math::Quaternion::slerp(
                current_orientation_, target_orientation_, t);
        } else {
            current_orientation_ = target_orientation_;
        }
    }

    math::Vec3 effective_position() const {
        if (mode_ == CameraMode::FirstPerson) {
            math::Vec3 pos = position_;
            // Add head bob
            if (is_moving_) {
                pos.y += std::sin(head_bob_timer_) * config.head_bob_amp;
            }
            return pos;
        } else {
            return tps_current_pos_;
        }
    }

    // --- TPS Implementation ---

    void process_mouse_tps(float dx, float dy) {
        orbit_yaw_ -= dx * config.sensitivity;
        orbit_pitch_ += -dy * config.sensitivity;
        orbit_pitch_ = std::clamp(orbit_pitch_, -config.max_pitch, config.max_pitch);
    }

    void update_tps(float dt) {
        // Calculate desired camera position on orbit sphere
        float cos_p = std::cos(orbit_pitch_);
        float sin_p = std::sin(orbit_pitch_);
        float cos_y = std::cos(orbit_yaw_);
        float sin_y = std::sin(orbit_yaw_);

        math::Vec3 orbit_offset = {
            sin_y * cos_p * config.orbit_distance,
            sin_p * config.orbit_distance + config.orbit_height,
            cos_y * cos_p * config.orbit_distance
        };

        math::Vec3 target_pos = tps_target_ + orbit_offset;

        // Prevent going below ground
        if (target_pos.y < 0.3f) target_pos.y = 0.3f;

        // Smooth camera position (LERP for position)
        float pos_t = 1.0f - std::pow(config.orbit_smoothing, dt * 60.0f);
        tps_current_pos_ = tps_current_pos_.lerp(target_pos, pos_t);

        // Look at target — build orientation quaternion
        math::Vec3 look_dir = (tps_target_ - tps_current_pos_);
        if (look_dir.length_squared() > 1e-6f) {
            look_dir = look_dir.normalized();
            math::Quaternion target_orient =
                math::Quaternion::from_two_vectors(math::Vec3::forward(), look_dir);

            // SLERP the orientation for smooth look transitions
            float orient_t = 1.0f - std::pow(config.orbit_smoothing * 0.5f, dt * 60.0f);
            current_orientation_ = math::Quaternion::slerp(
                current_orientation_, target_orient, orient_t);
        }
    }
};

} // namespace renderer
} // namespace qe
