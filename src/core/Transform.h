#pragma once
/**
 * @file Transform.h
 * @brief 3D Transform component using quaternion rotation.
 *
 * Represents an entity's position, orientation, and scale in 3D space.
 * Uses quaternions internally for all rotation, providing:
 *   - Gimbal-lock-free rotation
 *   - Smooth interpolation via SLERP
 *   - Efficient composition of rotations
 *
 * This is the fundamental building block for every game entity.
 */

#include "../math/Mat4.h"
#include "../math/Quaternion.h"
#include "../math/Vec3.h"

namespace qe {
namespace core {

class Transform {
public:
    // --- Constructors ---

    /** Default: origin, no rotation, unit scale. */
    Transform() noexcept
        : position_(math::Vec3::zero()),
          rotation_(math::Quaternion::identity()),
          scale_(math::Vec3::one()) {}

    /** Explicit position, rotation, scale. */
    Transform(const math::Vec3& pos, const math::Quaternion& rot,
              const math::Vec3& scl) noexcept
        : position_(pos), rotation_(rot), scale_(scl) {}

    // --- Accessors ---

    const math::Vec3& position() const noexcept { return position_; }
    const math::Quaternion& rotation() const noexcept { return rotation_; }
    const math::Vec3& scale() const noexcept { return scale_; }

    void set_position(const math::Vec3& pos) noexcept {
        position_ = pos;
        dirty_ = true;
    }

    void set_rotation(const math::Quaternion& rot) noexcept {
        rotation_ = rot;
        dirty_ = true;
    }

    void set_scale(const math::Vec3& scl) noexcept {
        scale_ = scl;
        dirty_ = true;
    }

    // --- Movement ---

    /** Translate in world space. */
    void translate(const math::Vec3& delta) noexcept {
        position_ += delta;
        dirty_ = true;
    }

    /** Translate in local space (relative to current orientation). */
    void translate_local(const math::Vec3& delta) noexcept {
        position_ += rotation_.rotate(delta);
        dirty_ = true;
    }

    /** Rotate by a quaternion (post-multiply: new = current * delta). */
    void rotate(const math::Quaternion& delta) noexcept {
        rotation_ = (rotation_ * delta).normalized();
        dirty_ = true;
    }

    /** Rotate around an axis by angle (radians). */
    void rotate_axis(const math::Vec3& axis, float angle) {
        rotate(math::Quaternion::from_axis_angle(axis, angle));
    }

    // --- Direction Vectors ---

    /** Forward direction (-Z in local space, rotated to world). */
    math::Vec3 forward() const noexcept {
        return rotation_.rotate(math::Vec3::forward());
    }

    /** Right direction (+X in local space, rotated to world). */
    math::Vec3 right() const noexcept {
        return rotation_.rotate(math::Vec3::right());
    }

    /** Up direction (+Y in local space, rotated to world). */
    math::Vec3 up() const noexcept {
        return rotation_.rotate(math::Vec3::up());
    }

    // --- Look At ---

    /** Orient to face a target point (world space).
     *  @param target World-space point to look at.
     *  @param world_up Reference up vector (default: Y-up).
     */
    void look_at(const math::Vec3& target,
                 const math::Vec3& /*world_up*/ = math::Vec3::up()) {
        math::Vec3 dir = (target - position_).normalized();
        rotation_ = math::Quaternion::from_two_vectors(math::Vec3::forward(), dir);
        dirty_ = true;
    }

    // --- Interpolation ---

    /** Smoothly interpolate between two transforms.
     *  Position: LERP, Rotation: SLERP, Scale: LERP.
     */
    static Transform interpolate(const Transform& a, const Transform& b,
                                 float t) {
        return Transform(
            a.position_.lerp(b.position_, t),
            math::Quaternion::slerp(a.rotation_, b.rotation_, t),
            a.scale_.lerp(b.scale_, t)
        );
    }

    // --- Matrix ---

    /** Compute the model matrix (Translation * Rotation * Scale). */
    math::Mat4 to_matrix() const noexcept {
        if (dirty_) {
            cached_matrix_ = math::Mat4::trs(position_, rotation_, scale_);
            dirty_ = false;
        }
        return cached_matrix_;
    }

private:
    math::Vec3 position_;
    math::Quaternion rotation_;
    math::Vec3 scale_;

    mutable math::Mat4 cached_matrix_;
    mutable bool dirty_ = true;
};

} // namespace core
} // namespace qe
