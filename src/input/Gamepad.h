#pragma once
/**
 * @file Gamepad.h
 * @brief Xbox/generic gamepad wrapper using SDL2 GameController API.
 *
 * Design by Contract:
 *   - Precondition: SDL must be initialized with SDL_INIT_GAMECONTROLLER
 *   - Invariant: stick values are always in [-1, 1], triggers in [0, 1]
 *   - Postcondition: poll() updates all button/axis state for one frame
 *
 * Supports: Xbox 360, Xbox One, Xbox Series, PS4/5 (via SDL mapping DB).
 * SDL2 ships with a built-in controller mapping database.
 */

#include <SDL.h>

#include <cmath>
#include <iostream>
#include <string>

namespace qe {
namespace input {

class Gamepad {
public:
    /** Stick axes (normalized to [-1, 1]). */
    struct StickState {
        float x = 0.0f;
        float y = 0.0f;
    };

    /** Trigger values (normalized to [0, 1]). */
    struct TriggerState {
        float left  = 0.0f;
        float right = 0.0f;
    };

    /** Button identifiers matching Xbox layout. */
    enum class Button {
        A, B, X, Y,
        LeftBumper, RightBumper,
        Back, Start, Guide,
        LeftStick, RightStick,
        DPadUp, DPadDown, DPadLeft, DPadRight,
        Count
    };

    Gamepad() = default;
    ~Gamepad() { close(); }

    // Non-copyable, movable
    Gamepad(const Gamepad&) = delete;
    Gamepad& operator=(const Gamepad&) = delete;
    Gamepad(Gamepad&& other) noexcept { swap(other); }
    Gamepad& operator=(Gamepad&& other) noexcept { swap(other); return *this; }

    // --- Lifecycle ---

    /** Try to open the first available game controller. */
    bool open() {
        close();

        int count = SDL_NumJoysticks();
        for (int i = 0; i < count; ++i) {
            if (SDL_IsGameController(i)) {
                controller_ = SDL_GameControllerOpen(i);
                if (controller_) {
                    name_ = SDL_GameControllerName(controller_);
                    std::cout << "[Gamepad] Connected: " << name_ << std::endl;
                    connected_ = true;
                    return true;
                }
            }
        }
        return false;
    }

    /** Close the controller. */
    void close() {
        if (controller_) {
            SDL_GameControllerClose(controller_);
            controller_ = nullptr;
        }
        connected_ = false;
        name_ = "None";
    }

    /** Handle SDL events for hotplug. Returns true if a controller was connected/disconnected. */
    bool handle_event(const SDL_Event& event) {
        if (event.type == SDL_CONTROLLERDEVICEADDED) {
            if (!connected_) {
                open();
                return true;
            }
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (connected_) {
                std::cout << "[Gamepad] Disconnected: " << name_ << std::endl;
                close();
                return true;
            }
        }
        return false;
    }

    // --- State Queries ---

    bool is_connected() const noexcept { return connected_; }
    const std::string& name() const noexcept { return name_; }

    /** Poll current state. Call once per frame. */
    void poll() {
        if (!controller_) return;

        // Sticks (raw range: -32768 to 32767)
        left_stick_.x  = normalize_axis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX));
        left_stick_.y  = normalize_axis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY));
        right_stick_.x = normalize_axis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTX));
        right_stick_.y = normalize_axis(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTY));

        // Apply deadzone
        apply_deadzone(left_stick_);
        apply_deadzone(right_stick_);

        // Triggers (raw range: 0 to 32767)
        triggers_.left  = normalize_trigger(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        triggers_.right = normalize_trigger(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

        // Buttons
        for (int i = 0; i < static_cast<int>(Button::Count); ++i) {
            bool prev = buttons_[i];
            buttons_[i] = SDL_GameControllerGetButton(controller_,
                static_cast<SDL_GameControllerButton>(i)) != 0;
            buttons_pressed_[i]  = !prev && buttons_[i];
            buttons_released_[i] = prev && !buttons_[i];
        }
    }

    // --- Accessors ---

    const StickState& left_stick() const noexcept { return left_stick_; }
    const StickState& right_stick() const noexcept { return right_stick_; }
    const TriggerState& triggers() const noexcept { return triggers_; }

    bool button_held(Button b) const noexcept {
        return buttons_[static_cast<int>(b)];
    }
    bool button_pressed(Button b) const noexcept {
        return buttons_pressed_[static_cast<int>(b)];
    }
    bool button_released(Button b) const noexcept {
        return buttons_released_[static_cast<int>(b)];
    }

    // --- Configuration ---

    float deadzone() const noexcept { return deadzone_; }
    void set_deadzone(float dz) noexcept { deadzone_ = dz; }

private:
    SDL_GameController* controller_ = nullptr;
    bool connected_ = false;
    std::string name_ = "None";

    StickState left_stick_;
    StickState right_stick_;
    TriggerState triggers_;

    static constexpr int BUTTON_COUNT = static_cast<int>(Button::Count);
    bool buttons_[BUTTON_COUNT] = {};
    bool buttons_pressed_[BUTTON_COUNT] = {};
    bool buttons_released_[BUTTON_COUNT] = {};

    float deadzone_ = 0.15f;

    void swap(Gamepad& other) noexcept {
        std::swap(controller_, other.controller_);
        std::swap(connected_, other.connected_);
        std::swap(name_, other.name_);
    }

    static float normalize_axis(Sint16 raw) {
        return static_cast<float>(raw) / 32767.0f;
    }

    static float normalize_trigger(Sint16 raw) {
        return static_cast<float>(raw) / 32767.0f;
    }

    void apply_deadzone(StickState& stick) const {
        float mag = std::sqrt(stick.x * stick.x + stick.y * stick.y);
        if (mag < deadzone_) {
            stick.x = 0.0f;
            stick.y = 0.0f;
        } else {
            // Rescale to [0, 1] after deadzone
            float scale = (mag - deadzone_) / (1.0f - deadzone_) / mag;
            stick.x *= scale;
            stick.y *= scale;
        }
    }
};

} // namespace input
} // namespace qe
