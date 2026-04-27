#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file WaveSystem.h
 * @brief Wave-based game state machine with progressive difficulty scaling.
 *
 * Single Responsibility: manages wave progression and game state transitions.
 * Design by Contract: preconditions, postconditions, and invariants enforced
 * via asserts throughout.
 *
 * State graph:
 *   Menu -> WaveIntro -> WaveActive -> WaveClear -> WaveIntro (loop)
 *                                   \-> GameOver
 *   Menu <- GameOver
 */

#include <algorithm>
#include <stdexcept>

namespace qe {
namespace game {

// ── Game State ──────────────────────────────────────────────────────────────

enum class GameState {
    Menu,
    WaveIntro,
    WaveActive,
    WaveClear,
    GameOver
};

// ── Wave Configuration ──────────────────────────────────────────────────────

struct WaveConfig {
    int   wave_number        = 1;
    int   target_count       = 6;
    float speed_multiplier   = 1.0f;
    float health_multiplier  = 1.0f;
    float spawn_radius_min   = 8.0f;
    float spawn_radius_max   = 20.0f;
    int   bonus_points       = 100;
    int   time_bonus_seconds = 58;
};

// ── Wave System ─────────────────────────────────────────────────────────────

class WaveSystem {
public:
    // ── Constants ────────────────────────────────────────────────────────

    static constexpr float INTRO_DURATION = 2.0f;
    static constexpr float CLEAR_DURATION = 3.0f;

    // ── Factory ──────────────────────────────────────────────────────────

    /** Generate a WaveConfig for wave number n.
     *  @pre n >= 1 */
    static WaveConfig generate_wave(int n) {
        if (n < 1)
            throw std::invalid_argument("generate_wave: wave number must be >= 1");

        WaveConfig cfg;
        cfg.wave_number       = n;
        cfg.target_count      = 6 + (n - 1) * 3;
        cfg.speed_multiplier  = 1.0f + (n - 1) * 0.15f;
        cfg.health_multiplier = 1.0f + (n - 1) * 0.2f;
        cfg.spawn_radius_min  = 8.0f;
        cfg.spawn_radius_max  = 20.0f;
        cfg.bonus_points      = 100 * n;
        cfg.time_bonus_seconds = std::max(20, 60 - n * 2);

        // Postcondition checks (invariants of generated config)
        if (cfg.target_count <= 0)
            throw std::logic_error("generate_wave: computed target_count must be > 0");
        if (cfg.speed_multiplier < 1.0f)
            throw std::logic_error("generate_wave: computed speed_multiplier must be >= 1");
        if (cfg.health_multiplier < 1.0f)
            throw std::logic_error("generate_wave: computed health_multiplier must be >= 1");
        if (cfg.time_bonus_seconds < 20)
            throw std::logic_error("generate_wave: computed time_bonus_seconds must be >= 20");
        return cfg;
    }

    // ── Queries ──────────────────────────────────────────────────────────

    GameState state()          const { return state_; }
    int       current_wave()   const { return wave_number_; }
    float     time_in_state()  const { return state_timer_; }
    float     total_game_time() const { return total_time_; }
    int       best_wave()      const { return best_wave_; }

    const WaveConfig& wave_config() const { return config_; }

    bool is_playing() const {
        return state_ == GameState::WaveIntro
            || state_ == GameState::WaveActive
            || state_ == GameState::WaveClear;
    }

    // ── Commands ─────────────────────────────────────────────────────────

    /** Reset all state and begin wave 1.
     *  @pre  state is Menu or GameOver
     *  @post state is WaveIntro, wave_number == 1 */
    void start_game() {
        if (state_ != GameState::Menu && state_ != GameState::GameOver)
            throw std::logic_error("start_game: must be in Menu or GameOver");

        wave_number_ = 1;
        total_time_  = 0.0f;
        best_wave_   = 1;
        config_      = generate_wave(1);
        transition_to(GameState::WaveIntro);

        // Postconditions
        if (state_ != GameState::WaveIntro)
            throw std::logic_error("start_game: postcondition failed — state not WaveIntro");
        if (wave_number_ < 1)
            throw std::logic_error("start_game: postcondition failed — wave_number < 1");
    }

    /** Advance the state machine.
     *  @param dt            frame delta time (seconds), must be >= 0
     *  @param alive_targets number of alive enemies in the current wave */
    void update(float dt, int alive_targets) {
        if (dt < 0.0f)
            throw std::invalid_argument("update: dt must be non-negative");
        if (alive_targets < 0)
            throw std::invalid_argument("update: alive_targets must be non-negative");

        // Accumulate game time only while playing
        if (is_playing()) {
            total_time_ += dt;
        }

        state_timer_ += dt;

        switch (state_) {
            case GameState::WaveIntro:
                if (state_timer_ >= INTRO_DURATION) {
                    transition_to(GameState::WaveActive);
                }
                break;

            case GameState::WaveActive:
                if (alive_targets <= 0) {
                    transition_to(GameState::WaveClear);
                }
                break;

            case GameState::WaveClear:
                if (state_timer_ >= CLEAR_DURATION) {
                    next_wave();
                }
                break;

            case GameState::Menu:
            case GameState::GameOver:
                // No automatic transitions from these states
                break;
        }

        check_invariants();
    }

    /** Transition to the GameOver state.
     *  @pre state is a playing state (WaveIntro, WaveActive, WaveClear) */
    void game_over() {
        if (!is_playing())
            throw std::logic_error("game_over: must be in a playing state");
        transition_to(GameState::GameOver);
    }

    /** Return to Menu from GameOver.
     *  @pre state is GameOver */
    void return_to_menu() {
        if (state_ != GameState::GameOver)
            throw std::logic_error("return_to_menu: must be in GameOver");
        transition_to(GameState::Menu);
    }

private:
    GameState  state_       = GameState::Menu;
    int        wave_number_ = 0;
    float      state_timer_ = 0.0f;
    float      total_time_  = 0.0f;
    int        best_wave_   = 0;
    WaveConfig config_{};

    // ── Internal ─────────────────────────────────────────────────────────

    void transition_to(GameState next) {
        if (!is_valid_transition(state_, next))
            throw std::logic_error("transition_to: invalid state transition");
        state_       = next;
        state_timer_ = 0.0f;
    }

    /** Advance to the next wave.
     *  @pre state is WaveClear */
    void next_wave() {
        if (state_ != GameState::WaveClear)
            throw std::logic_error("next_wave: must be in WaveClear");

        wave_number_++;
        if (wave_number_ > best_wave_) {
            best_wave_ = wave_number_;
        }
        config_ = generate_wave(wave_number_);
        transition_to(GameState::WaveIntro);

        if (wave_number_ < 2)
            throw std::logic_error("next_wave: postcondition failed — wave_number < 2");
    }

    static bool is_valid_transition(GameState from, GameState to) {
        switch (from) {
            case GameState::Menu:
                return to == GameState::WaveIntro;
            case GameState::WaveIntro:
                return to == GameState::WaveActive
                    || to == GameState::GameOver;
            case GameState::WaveActive:
                return to == GameState::WaveClear
                    || to == GameState::GameOver;
            case GameState::WaveClear:
                return to == GameState::WaveIntro
                    || to == GameState::GameOver;
            case GameState::GameOver:
                return to == GameState::Menu
                    || to == GameState::WaveIntro;
        }
        return false;
    }

    void check_invariants() const {
        if (state_timer_ < 0.0f)
            throw std::logic_error("invariant violation: state_timer must be non-negative");
        if (total_time_ < 0.0f)
            throw std::logic_error("invariant violation: total_time must be non-negative");
        if (is_playing() && wave_number_ < 1)
            throw std::logic_error("invariant violation: wave_number must be >= 1 while playing");
    }
};

} // namespace game
} // namespace qe
