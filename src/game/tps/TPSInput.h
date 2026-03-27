#pragma once
/**
 * @file TPSInput.h
 * @brief Maps abstract input values to PlayerController actions for TPS gameplay.
 *
 * Decoupled from SDL — receives normalized input values (-1 to 1 for axes,
 * bool for buttons) and translates them into PlayerController commands.
 * This allows testing without any windowing system dependency.
 *
 * Input mapping:
 *   Left Stick / WASD    → Movement
 *   Right Stick / Mouse   → Camera look (passed through to camera)
 *   LT / Right Click      → Lock-on toggle
 *   RT / Left Click       → Shoot (ranged) or attack based on lock state
 *   X / Mouse Side        → Light melee attack
 *   Y / Middle Click      → Heavy melee attack
 *   A / Space             → Jump
 *   B / Ctrl              → Dodge
 *   LB / Q                → Previous weapon
 *   RB / E                → Next weapon
 *   R / Reload btn        → Reload
 *   L3 / Shift            → Sprint
 *   D-pad / Mouse Wheel   → Weapon select direct
 */

#include "../../math/Vec3.h"
#include "PlayerController.h"
#include "LockOnSystem.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// ── Abstract Input State ─────────────────────────────────────────────────────

struct TPSInputState {
    // Axes (-1 to 1)
    float move_forward = 0.0f;
    float move_right = 0.0f;
    float look_x = 0.0f;
    float look_y = 0.0f;

    // Buttons (pressed this frame)
    bool shoot_pressed = false;
    bool shoot_held = false;
    bool light_melee_pressed = false;
    bool heavy_melee_pressed = false;
    bool jump_pressed = false;
    bool jump_released = false;
    bool dodge_pressed = false;
    bool reload_pressed = false;
    bool lock_on_pressed = false;
    bool next_weapon_pressed = false;
    bool prev_weapon_pressed = false;
    bool parry_pressed = false;
    bool sprint_held = false;
    bool ground_slam_pressed = false;

    // Weapon direct select (1-7, 0 = none)
    int weapon_select = 0;

    void clear() {
        shoot_pressed = false;
        light_melee_pressed = false;
        heavy_melee_pressed = false;
        jump_pressed = false;
        jump_released = false;
        dodge_pressed = false;
        reload_pressed = false;
        lock_on_pressed = false;
        next_weapon_pressed = false;
        prev_weapon_pressed = false;
        parry_pressed = false;
        ground_slam_pressed = false;
        weapon_select = 0;
    }
};

// ── Input Result ─────────────────────────────────────────────────────────────

struct TPSInputResult {
    bool fired_weapon = false;
    bool started_melee = false;
    bool jumped = false;
    bool dodged = false;
    bool toggled_lock = false;
    bool switched_weapon = false;
    bool reloading = false;
};

// ── Input Processor ──────────────────────────────────────────────────────────

/** Process one frame of input and apply to player controller.
 *  @pre player is not null-like
 *  @post player state updated based on input
 */
inline TPSInputResult process_tps_input(
        const TPSInputState& input,
        PlayerController& player,
        const std::vector<LockOnTarget>& lock_targets) {

    TPSInputResult result;

    // Movement
    math::Vec3 move_input(input.move_right, 0.0f, -input.move_forward);
    if (move_input.length_squared() > 1.0f) {
        move_input = move_input.normalized();
    }
    player.set_move_input(move_input);
    player.set_sprint(input.sprint_held);

    // Jump
    if (input.jump_pressed) {
        result.jumped = player.request_jump();
    }
    if (input.jump_released) {
        player.release_jump();
    }

    // Dodge
    if (input.dodge_pressed) {
        math::Vec3 dodge_dir = move_input.length_squared() > 0.01f
            ? move_input : math::Vec3(0, 0, -1);
        result.dodged = player.dodge(dodge_dir);
    }

    // Lock-on toggle
    if (input.lock_on_pressed) {
        result.toggled_lock = true;
        player.toggle_lock_on(lock_targets);
    }

    // Weapon switching
    if (input.next_weapon_pressed) {
        player.loadout_mut().next_weapon();
        result.switched_weapon = true;
    }
    if (input.prev_weapon_pressed) {
        player.loadout_mut().prev_weapon();
        result.switched_weapon = true;
    }
    if (input.weapon_select > 0 && input.weapon_select <= player.loadout().weapon_count()) {
        player.loadout_mut().switch_weapon(input.weapon_select - 1);
        result.switched_weapon = true;
    }

    // Reload
    if (input.reload_pressed) {
        player.reload();
        result.reloading = true;
    }

    // Combat — prioritize melee if pressed, otherwise ranged
    if (input.parry_pressed) {
        player.parry();
    } else if (input.light_melee_pressed) {
        if (player.jump().is_airborne()) {
            result.started_melee = player.melee_jump_attack();
        } else if (player.is_dodging()) {
            result.started_melee = player.melee_dodge_attack();
        } else {
            result.started_melee = player.melee_light();
        }
    } else if (input.heavy_melee_pressed) {
        result.started_melee = player.melee_heavy();
    } else if (input.shoot_held || input.shoot_pressed) {
        result.fired_weapon = player.shoot();
    }

    // Ground slam (while airborne)
    if (input.ground_slam_pressed && player.jump().is_airborne()) {
        player.melee_jump_attack();
    }

    return result;
}

// ── Deadzone Helper ──────────────────────────────────────────────────────────

inline float apply_deadzone(float value, float deadzone = 0.15f) {
    if (std::abs(value) < deadzone) return 0.0f;
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
}

} // namespace tps
} // namespace game
} // namespace qe
