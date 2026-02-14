#pragma once
/**
 * @file InputManager.h
 * @brief Unified input manager — keyboard, mouse, and gamepad in one interface.
 *
 * Design by Contract:
 *   - Precondition: SDL must be initialized before construction
 *   - Invariant: movement axes always normalized to [-1, 1]
 *   - Postcondition: after poll(), all device states are current-frame
 *
 * Usage:
 *   InputManager input;
 *   input.init();
 *   // In game loop:
 *   input.begin_frame();
 *   input.handle_event(event);
 *   input.poll();
 *   float fw = input.move_forward();  // Works for WASD AND left stick
 */

#include "Gamepad.h"

#include <SDL.h>

namespace qe {
namespace input {

class InputManager {
public:
    InputManager() = default;

    /** Initialize gamepad subsystem. */
    void init() {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
        gamepad_.open();
    }

    /** Reset per-frame state. Call at start of each frame. */
    void begin_frame() {
        mouse_dx_ = 0.0f;
        mouse_dy_ = 0.0f;
        scroll_   = 0.0f;
        shoot_pressed_  = false;
        shoot_released_ = false;
    }

    /** Process SDL event. Returns true if consumed. */
    bool handle_event(const SDL_Event& event) {
        // Gamepad hotplug
        if (gamepad_.handle_event(event)) return true;

        switch (event.type) {
            case SDL_MOUSEMOTION:
                mouse_dx_ += static_cast<float>(event.motion.xrel);
                mouse_dy_ += static_cast<float>(event.motion.yrel);
                return true;

            case SDL_MOUSEWHEEL:
                scroll_ += static_cast<float>(event.wheel.y);
                return true;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    shoot_held_ = true;
                    shoot_pressed_ = true;
                }
                return true;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    shoot_held_ = false;
                    shoot_released_ = true;
                }
                return true;

            default:
                return false;
        }
    }

    /** Poll keyboard and gamepad state. Call after processing events. */
    void poll() {
        keys_ = SDL_GetKeyboardState(nullptr);
        gamepad_.poll();
    }

    // --- Unified Queries (keyboard + gamepad combined) ---

    /** Forward/backward axis [-1, 1]. W/S or left stick Y. */
    float move_forward() const {
        float kb = key_axis(SDL_SCANCODE_W, SDL_SCANCODE_S);
        float gp = -gamepad_.left_stick().y;  // SDL Y is inverted
        return clamp_sum(kb, gp);
    }

    /** Right/left strafe axis [-1, 1]. D/A or left stick X. */
    float move_right() const {
        float kb = key_axis(SDL_SCANCODE_D, SDL_SCANCODE_A);
        float gp = gamepad_.left_stick().x;
        return clamp_sum(kb, gp);
    }

    /** Up/down axis [-1, 1]. Space/C or bumpers. */
    float move_up() const {
        float kb = key_axis(SDL_SCANCODE_SPACE, SDL_SCANCODE_C);
        float gp = 0.0f;
        if (gamepad_.button_held(Gamepad::Button::LeftBumper))  gp -= 1.0f;
        if (gamepad_.button_held(Gamepad::Button::RightBumper)) gp += 1.0f;
        return clamp_sum(kb, gp);
    }

    /** Mouse look delta X (pixels). Combined with right stick X. */
    float look_x() const {
        return mouse_dx_ + gamepad_.right_stick().x * gamepad_look_speed_;
    }

    /** Mouse look delta Y (pixels). Combined with right stick Y. */
    float look_y() const {
        return mouse_dy_ + gamepad_.right_stick().y * gamepad_look_speed_;
    }

    /** Zoom delta (scroll wheel + D-pad up/down). */
    float zoom() const {
        float gp = 0.0f;
        if (gamepad_.button_held(Gamepad::Button::DPadUp))   gp += 1.0f;
        if (gamepad_.button_held(Gamepad::Button::DPadDown)) gp -= 1.0f;
        return scroll_ + gp;
    }

    /** Sprint: Left Shift or left stick click. */
    bool sprint() const {
        return key_held(SDL_SCANCODE_LSHIFT) ||
               gamepad_.button_held(Gamepad::Button::LeftStick);
    }

    /** Shoot: Left Click or right trigger. */
    bool shoot_held() const {
        return shoot_held_ || gamepad_.triggers().right > 0.5f;
    }

    bool shoot_pressed() const {
        return shoot_pressed_ || gamepad_.button_pressed(Gamepad::Button::A);
    }

    /** Toggle camera: Tab or Y button. */
    bool toggle_camera() const {
        return key_pressed(SDL_SCANCODE_TAB) ||
               gamepad_.button_pressed(Gamepad::Button::Y);
    }

    /** Wireframe: F or X button. */
    bool toggle_wireframe() const {
        return key_pressed(SDL_SCANCODE_F) ||
               gamepad_.button_pressed(Gamepad::Button::X);
    }

    /** Reset: R or Back button. */
    bool reset() const {
        return key_pressed(SDL_SCANCODE_R) ||
               gamepad_.button_pressed(Gamepad::Button::Back);
    }

    /** SLERP off: 1 key, SLERP on: 2 key (or D-pad left/right). */
    bool slerp_off() const {
        return key_pressed(SDL_SCANCODE_1) ||
               gamepad_.button_pressed(Gamepad::Button::DPadLeft);
    }

    bool slerp_on() const {
        return key_pressed(SDL_SCANCODE_2) ||
               gamepad_.button_pressed(Gamepad::Button::DPadRight);
    }

    /** Quit: Escape or Start + Back. */
    bool quit() const {
        return key_pressed(SDL_SCANCODE_ESCAPE) ||
               (gamepad_.button_held(Gamepad::Button::Start) &&
                gamepad_.button_held(Gamepad::Button::Back));
    }

    // --- Direct Access ---

    const Gamepad& gamepad() const noexcept { return gamepad_; }
    bool gamepad_connected() const noexcept { return gamepad_.is_connected(); }

    void set_gamepad_look_speed(float speed) noexcept {
        gamepad_look_speed_ = speed;
    }

private:
    const Uint8* keys_ = nullptr;
    Gamepad gamepad_;

    float mouse_dx_ = 0, mouse_dy_ = 0;
    float scroll_ = 0;
    bool shoot_held_ = false;
    bool shoot_pressed_ = false;
    bool shoot_released_ = false;

    float gamepad_look_speed_ = 5.0f;

    // Previous-frame key state for edge detection
    mutable Uint8 prev_keys_[SDL_NUM_SCANCODES] = {};

    bool key_held(SDL_Scancode sc) const {
        return keys_ && keys_[sc];
    }

    bool key_pressed(SDL_Scancode sc) const {
        bool current = keys_ && keys_[sc];
        bool prev = prev_keys_[sc];
        // Update prev (mutable) - slight hack but avoids separate begin_frame
        const_cast<Uint8*>(prev_keys_)[sc] = current ? 1 : 0;
        return current && !prev;
    }

    float key_axis(SDL_Scancode positive, SDL_Scancode negative) const {
        float val = 0.0f;
        if (key_held(positive)) val += 1.0f;
        if (key_held(negative)) val -= 1.0f;
        return val;
    }

    static float clamp_sum(float a, float b) {
        float s = a + b;
        return (s < -1.0f) ? -1.0f : (s > 1.0f) ? 1.0f : s;
    }
};

} // namespace input
} // namespace qe
