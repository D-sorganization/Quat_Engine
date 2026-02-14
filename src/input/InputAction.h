#pragma once
/**
 * @file InputAction.h
 * @brief Abstract input actions decoupled from device (keyboard, mouse, gamepad).
 *
 * Design by Contract:
 *   - Precondition: InputAction names must be non-empty strings
 *   - Invariant: axis values always clamped to [-1, 1]
 *   - Postcondition: is_pressed() returns true only for the frame it was first pressed
 *
 * This module defines the mapping layer between raw device inputs and
 * game-meaningful actions, enabling device-agnostic game code.
 */

#include <string>

namespace qe {
namespace input {

/** A logical input action (e.g., "shoot", "jump", "move_forward"). */
struct InputAction {
    std::string name;

    bool held     = false;   // Currently held down
    bool pressed  = false;   // Just pressed this frame
    bool released = false;   // Just released this frame
    float axis    = 0.0f;    // Analog value [-1, 1] (for sticks/triggers)

    explicit InputAction(std::string action_name)
        : name(std::move(action_name)) {}

    /** Reset per-frame transients. Call at start of each frame. */
    void begin_frame() {
        pressed  = false;
        released = false;
    }

    /** Mark as pressed (transition from not-held to held). */
    void press() {
        if (!held) {
            pressed = true;
        }
        held = true;
    }

    /** Mark as released (transition from held to not-held). */
    void release() {
        if (held) {
            released = true;
        }
        held = false;
        axis = 0.0f;
    }

    /** Set analog axis value, clamped to [-1, 1]. */
    void set_axis(float value) {
        axis = (value < -1.0f) ? -1.0f : (value > 1.0f) ? 1.0f : value;
    }
};

} // namespace input
} // namespace qe
