// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Rng.h
 * @brief Shared xorshift32 pseudo-random number generator.
 *
 * Provides a single, deterministic PRNG implementation to replace the 5+
 * duplicate xorshift32 implementations scattered across the codebase.
 *
 * Usage:
 *   qe::core::Rng rng(12345);          // seed
 *   float f = rng.random_float(0, 1);   // uniform float in [min, max]
 *   uint32_t r = rng.next();            // raw 32-bit value
 */

#ifndef QE_CORE_RNG_H
#define QE_CORE_RNG_H

#include <cstdint>

namespace qe {
namespace core {

class Rng {
public:
    explicit Rng(uint32_t seed = 12345) : state_(seed ? seed : 1) {}

    /** Advance the state and return a raw 32-bit pseudo-random value. */
    uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }

    /** Uniform float in [min, max]. */
    float random_float(float min, float max) {
        uint32_t r = next();
        float t = static_cast<float>(r) / static_cast<float>(0xFFFFFFFFu);
        return min + t * (max - min);
    }

    /** Current state (for seeding or inspection). */
    uint32_t state() const { return state_; }

    /** Replace the state directly (e.g. for reproducibility). */
    void seed(uint32_t s) { state_ = s ? s : 1; }

private:
    uint32_t state_;
};

} // namespace core
} // namespace qe

#endif // QE_CORE_RNG_H
