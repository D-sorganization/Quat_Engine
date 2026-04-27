#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file Constants.h
 * @brief Shared mathematical constants for the QuatEngine.
 *
 * Single authoritative source for PI and related constants.
 * Replaces three redundant local definitions (Scene.h, TPSScene.h,
 * ParticleSystem.h).  See D-sorganization/QuatEngine#93.
 */

namespace qe {
namespace math {

/** Ratio of a circle's circumference to its diameter. */
inline constexpr float PI = 3.14159265358979323846f;

/** Two times PI (full circle in radians). */
inline constexpr float TWO_PI = 2.0f * PI;

}  // namespace math
}  // namespace qe
