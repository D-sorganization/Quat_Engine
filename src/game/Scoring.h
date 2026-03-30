#pragma once
/**
 * @file Scoring.h
 * @brief Combo and scoring system for tracking kills, streaks, and multipliers.
 *
 * Provides:
 *   - ComboState: tracks hit streaks with a time window, awarding multipliers
 *     that scale from 1x (no combo) up to 5x ("GODLIKE!") for 20+ streaks.
 *   - ScoreEvent: represents a single scoring instance with base, combo,
 *     powerup, and wave bonus components.
 *   - ScoreTracker: top-level manager that ties combo tracking to score
 *     accumulation, high score tracking, and recent event display for the HUD.
 */

#include <cassert>
#include <vector>

namespace qe {
namespace game {

struct ComboState {
    int streak = 0;              // Consecutive hits without missing
    float combo_timer = 0.0f;    // Time since last hit
    float combo_window = 2.0f;   // Max seconds between hits to maintain combo
    int max_streak = 0;          // Best streak this game
    int total_kills = 0;
    int total_headshots = 0;     // (future: for precision shots)

    float multiplier() const {
        if (streak < 2) return 1.0f;
        if (streak < 5) return 1.5f;    // "Double!"
        if (streak < 10) return 2.0f;   // "Triple!"
        if (streak < 20) return 3.0f;   // "Mega!"
        return 5.0f;                     // "ULTRA!"
    }

    const char* combo_name() const {
        if (streak < 2) return "";
        if (streak < 5) return "COMBO x2!";
        if (streak < 10) return "MEGA COMBO!";
        if (streak < 20) return "ULTRA COMBO!";
        return "GODLIKE!";
    }

    void register_hit() {
        streak++;
        combo_timer = 0.0f;
        if (streak > max_streak) max_streak = streak;
    }

    void register_miss() {
        streak = 0;
        combo_timer = 0.0f;
    }

    void register_kill() {
        total_kills++;
    }

    void update(float dt) {
        combo_timer += dt;
        if (combo_timer > combo_window && streak > 0) {
            streak = 0;  // Combo expired
        }
    }

    void reset() {
        streak = 0;
        combo_timer = 0;
        max_streak = 0;
        total_kills = 0;
        total_headshots = 0;
    }
};

struct ScoreEvent {
    int base_points;
    float combo_multiplier;
    float powerup_multiplier;
    int wave_bonus;

    int total() const {
        return static_cast<int>(base_points * combo_multiplier * powerup_multiplier) + wave_bonus;
    }
};

class ScoreTracker {
    int score_ = 0;
    int high_score_ = 0;
    ComboState combo_;
    std::vector<ScoreEvent> recent_events_;  // Last N score events for HUD display
    static constexpr int MAX_RECENT = 5;
    float event_display_timer_ = 0.0f;

public:
    /** Record a kill and compute score.
     *  @pre base_score >= 0
     *  @pre powerup_multiplier >= 1.0
     *  @pre wave_bonus >= 0
     */
    ScoreEvent record_kill(int base_score, float powerup_multiplier = 1.0f, int wave_bonus = 0) {
        assert(base_score >= 0          && "record_kill: base_score must be non-negative");
        assert(powerup_multiplier >= 1.0f && "record_kill: powerup_multiplier must be >= 1");
        assert(wave_bonus >= 0          && "record_kill: wave_bonus must be non-negative");
        combo_.register_hit();
        combo_.register_kill();

        ScoreEvent event;
        event.base_points = base_score;
        event.combo_multiplier = combo_.multiplier();
        event.powerup_multiplier = powerup_multiplier;
        event.wave_bonus = wave_bonus;

        score_ += event.total();
        if (score_ > high_score_) high_score_ = score_;

        recent_events_.push_back(event);
        if (static_cast<int>(recent_events_.size()) > MAX_RECENT) {
            recent_events_.erase(recent_events_.begin());
        }
        event_display_timer_ = 2.0f;

        return event;
    }

    void record_hit() {
        combo_.register_hit();
    }

    void record_miss() {
        combo_.register_miss();
    }

    /** Add a bonus directly to the score.
     *  @pre points >= 0
     */
    void add_bonus(int points) {
        assert(points >= 0 && "add_bonus: points must be non-negative");
        score_ += points;
        if (score_ > high_score_) high_score_ = score_;
    }

    /** @pre dt >= 0 */
    void update(float dt) {
        assert(dt >= 0.0f && "ScoreTracker::update: dt must be non-negative");
        combo_.update(dt);
        if (event_display_timer_ > 0) {
            event_display_timer_ -= dt;
            if (event_display_timer_ < 0) event_display_timer_ = 0;
        }
    }

    int score() const { return score_; }
    int high_score() const { return high_score_; }
    const ComboState& combo() const { return combo_; }
    bool has_recent_event() const { return event_display_timer_ > 0; }
    const std::vector<ScoreEvent>& recent_events() const { return recent_events_; }

    void reset() {
        score_ = 0;
        combo_.reset();
        recent_events_.clear();
        event_display_timer_ = 0;
    }
};

} // namespace game
} // namespace qe
