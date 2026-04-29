// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

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

    /** Get the position component.
     *  @return Current world-space position
     *  @complexity O(1)
     */
    const math::Vec3& position() const noexcept { return position_; }

    /** Get the rotation component.
     *  @return Current rotation as a quaternion
     *  @complexity O(1)
     */
    const math::Quaternion& rotation() const noexcept { return rotation_; }

    /** Get the scale component.
     *  @return Current scale factors
     *  @complexity O(1)
     */
    const math::Vec3& scale() const noexcept { return scale_; }

    /** Set the position component.
     *  Marks the transform matrix cache as dirty for recomputation.
     *  @param pos New world-space position
     *  @complexity O(1) - only updates position and dirty flag
     */
    void set_position(const math::Vec3& pos) noexcept {
        position_ = pos;
        dirty_ = true;
    }

    /** Set the rotation component.
     *  Marks the transform matrix cache as dirty for recomputation.
     *  @param rot New rotation quaternion
     *  @complexity O(1) - only updates rotation and dirty flag
     */
    void set_rotation(const math::Quaternion& rot) noexcept {
        rotation_ = rot;
        dirty_ = true;
    }

    /** Set the scale component.
     *  Marks the transform matrix cache as dirty for recomputation.
     *  @param scl New scale factors
     *  @complexity O(1) - only updates scale and dirty flag
     */
    void set_scale(const math::Vec3& scl) noexcept {
        scale_ = scl;
        dirty_ = true;
    }

    // --- Movement ---

    /** Translate in world space.
     *  Adds the delta to the current position without rotation.
     *  @param delta Translation offset in world coordinates
     *  @complexity O(1) - vector addition and dirty flag update
     */
    void translate(const math::Vec3& delta) noexcept {
        position_ += delta;
        dirty_ = true;
    }

    /** Translate in local space relative to current orientation.
     *  Rotates the delta by the current rotation before applying translation.
     *  Useful for moving "forward" or "right" relative to the entity's facing direction.
     *  @param delta Translation offset in local coordinates
     *  @complexity O(1) - rotation of vector, addition, and dirty flag update
     */
    void translate_local(const math::Vec3& delta) noexcept {
        position_ += rotation_.rotate(delta);
        dirty_ = true;
    }

    /** Rotate by applying a quaternion delta (post-multiply).
     *  Applies rotation as: new_rotation = current_rotation * delta_rotation.
     *  The result is normalized to maintain unit-length quaternion.
     *  @param delta Incremental rotation quaternion
     *  @complexity O(1) - quaternion multiplication and normalization
     */
    void rotate(const math::Quaternion& delta) noexcept {
        rotation_ = (rotation_ * delta).normalized();
        dirty_ = true;
    }

    /** Rotate around an axis by an angle in radians.
     *  Convenience wrapper that creates an axis-angle quaternion then applies rotation.
     *  @param axis Normalized rotation axis
     *  @param angle Rotation angle in radians
     *  @complexity O(1) - axis-angle conversion and quaternion rotation
     */
    void rotate_axis(const math::Vec3& axis, float angle) {
        rotate(math::Quaternion::from_axis_angle(axis, angle));
    }

    // --- Direction Vectors ---

    /** Get the forward direction vector in world space.
     *  Forward is the -Z direction in local space, rotated to world coordinates.
     *  Typically represents where the entity is facing.
     *  @return Normalized forward vector
     *  @complexity O(1) - quaternion vector rotation
     */
    math::Vec3 forward() const noexcept {
        return rotation_.rotate(math::Vec3::forward());
    }

    /** Get the right direction vector in world space.
     *  Right is the +X direction in local space, rotated to world coordinates.
     *  @return Normalized right vector (perpendicular to forward and up)
     *  @complexity O(1) - quaternion vector rotation
     */
    math::Vec3 right() const noexcept {
        return rotation_.rotate(math::Vec3::right());
    }

    /** Get the up direction vector in world space.
     *  Up is the +Y direction in local space, rotated to world coordinates.
     *  @return Normalized up vector
     *  @complexity O(1) - quaternion vector rotation
     */
    math::Vec3 up() const noexcept {
        return rotation_.rotate(math::Vec3::up());
    }

    // --- Look At ---

    /** Orient this transform to face a target point.
     *  Computes a rotation quaternion that points the forward direction (-Z) toward the target.
     *  The up vector is used as reference but is not perfectly preserved (gimbal lock avoided via quaternions).
     *  @param target World-space point to look at
     *  @param world_up Reference up vector for computing the right vector (default: Y-up)
     *  @complexity O(1) - vector subtraction, normalization, and quaternion creation from two vectors
     */
    void look_at(const math::Vec3& target,
                 const math::Vec3& /*world_up*/ = math::Vec3::up()) {
        math::Vec3 dir = (target - position_).normalized();
        rotation_ = math::Quaternion::from_two_vectors(math::Vec3::forward(), dir);
        dirty_ = true;
    }

    // --- Interpolation ---

    /** Smoothly interpolate between two transforms.
     *  Linearly interpolates position and scale, uses spherical linear interpolation (SLERP)
     *  for rotation to maintain smooth rotation without gimbal lock.
     *  @param a Start transform
     *  @param b End transform
     *  @param t Interpolation factor: 0.0 = a, 1.0 = b
     *  @return Interpolated transform at parameter t
     *  @complexity O(1) - three vector lerps, one quaternion slerp
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

    /** Compute the 4x4 model transformation matrix.
     *  Returns a cached matrix that converts from local to world space.
     *  Internally applies: Translation * Rotation * Scale.
     *  The matrix is cached and recomputed only when dirty flag is set.
     *  @return Column-major 4x4 transformation matrix
     *  @complexity O(1) amortized - returns cached matrix, recomputes only when transform changes
     */
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
