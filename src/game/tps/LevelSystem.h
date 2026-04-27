#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file LevelSystem.h
 * @brief 12-level progression system for the post-apocalyptic TPS game.
 *
 * Set in an alternate post-WW2 universe where experimental weapons and
 * radiation spawned a mutant plague across war-torn Europe. Each level
 * features unique environments, enemy compositions, and objectives.
 *
 * Levels:
 *   1. Ruined Outpost     - Tutorial, bombed military base
 *   2. Ashen Subway       - Underground metro, tight corridors
 *   3. Crater Market      - Open bombed marketplace, mixed combat
 *   4. Irradiated Forest  - Mutated woods, ambush encounters
 *   5. Bunker Descent     - Vertical bunker, claustrophobic
 *   6. Flooded District   - Half-submerged city, platforming
 *   7. Rail Yards         - Industrial zone, cover-based
 *   8. Cathedral Ruins    - Vertical combat, flying enemies
 *   9. Laboratory Complex - Research facility, traps + experimentals
 *  10. Bridge of Bones    - Massive bridge, horde defense
 *  11. The Spire          - Mutant hive tower, all enemy types
 *  12. Ground Zero        - Final crater, Apex boss fight
 *
 * Design by Contract: level indices are 1-based, validated on access.
 * DRY: enemy spawns use a shared spawn_table structure.
 * Orthogonal: level data is pure data, no rendering or input coupling.
 */

#include "LevelLoader.h"
#include "LevelTypes.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// ── Level State Machine ──────────────────────────────────────────────────────

enum class LevelState {
    Briefing,
    Active,
    ObjectiveComplete,
    Failed,
    Victory
};

class LevelManager {
    int current_level_ = 1;
    LevelState state_ = LevelState::Briefing;
    LevelData level_data_;
    float level_timer_ = 0.0f;
    float state_timer_ = 0.0f;
    int enemies_alive_ = 0;
    int enemies_killed_ = 0;
    bool objective_met_ = false;
    int total_score_ = 0;
    std::vector<bool> levels_completed_;

public:
    LevelManager() : levels_completed_(TOTAL_LEVELS, false) {
        level_data_ = make_level(1);
    }

    // ── Queries ──────────────────────────────────────────────────────

    int current_level() const { return current_level_; }
    LevelState state() const { return state_; }
    const LevelData& level_data() const { return level_data_; }
    float level_timer() const { return level_timer_; }
    int enemies_alive() const { return enemies_alive_; }
    int enemies_killed() const { return enemies_killed_; }
    int total_score() const { return total_score_; }
    bool is_final_level() const { return current_level_ == TOTAL_LEVELS; }
    bool all_complete() const {
        for (bool c : levels_completed_) if (!c) return false;
        return true;
    }

    bool is_level_complete(int level) const {
        assert(level >= 1 && level <= TOTAL_LEVELS);
        return levels_completed_[level - 1];
    }

    float objective_progress() const {
        switch (level_data_.primary_objective.type) {
            case ObjectiveType::KillAll:
            case ObjectiveType::BossKill:
                return level_data_.total_enemy_count > 0
                    ? static_cast<float>(enemies_killed_) / level_data_.total_enemy_count
                    : 0.0f;
            case ObjectiveType::Survive:
            case ObjectiveType::DefendPoint:
                return level_data_.primary_objective.duration > 0
                    ? std::min(1.0f, level_timer_ / level_data_.primary_objective.duration)
                    : 0.0f;
            case ObjectiveType::ReachExit:
                return objective_met_ ? 1.0f : 0.0f;
        }
        return 0.0f;
    }

    // ── Commands ─────────────────────────────────────────────────────

    /** Load a specific level.
     *  @pre level >= 1 && level <= TOTAL_LEVELS
     */
    void load_level(int level) {
        assert(level >= 1 && level <= TOTAL_LEVELS);
        current_level_ = level;
        level_data_ = make_level(level);
        state_ = LevelState::Briefing;
        level_timer_ = 0.0f;
        state_timer_ = 0.0f;
        enemies_alive_ = level_data_.total_enemy_count;
        enemies_killed_ = 0;
        objective_met_ = false;
    }

    /** Start the level (transition from Briefing to Active). */
    void start_level() {
        assert(state_ == LevelState::Briefing);
        state_ = LevelState::Active;
        state_timer_ = 0.0f;
    }

    /** Notify that an enemy was killed. */
    void on_enemy_killed(int score) {
        enemies_killed_++;
        enemies_alive_--;
        if (enemies_alive_ < 0) enemies_alive_ = 0;
        total_score_ += score;
    }

    /** Notify that player reached the exit point. */
    void on_exit_reached() {
        if (level_data_.primary_objective.type == ObjectiveType::ReachExit) {
            objective_met_ = true;
        }
    }

    /** Notify that the player died. */
    void on_player_death() {
        if (state_ == LevelState::Active) {
            state_ = LevelState::Failed;
            state_timer_ = 0.0f;
        }
    }

    /** Advance to the next level.
     *  @pre current_level < TOTAL_LEVELS
     */
    void advance_to_next() {
        assert(current_level_ < TOTAL_LEVELS);
        load_level(current_level_ + 1);
    }

    /** Retry the current level. */
    void retry_level() {
        load_level(current_level_);
    }

    // ── Update ───────────────────────────────────────────────────────

    void update(float dt) {
        assert(dt >= 0.0f);
        state_timer_ += dt;

        if (state_ != LevelState::Active) return;

        level_timer_ += dt;

        // Check objective completion
        switch (level_data_.primary_objective.type) {
            case ObjectiveType::KillAll:
                if (enemies_alive_ <= 0) complete_level();
                break;
            case ObjectiveType::BossKill:
                if (enemies_alive_ <= 0) complete_level();
                break;
            case ObjectiveType::Survive:
                if (level_timer_ >= level_data_.primary_objective.duration)
                    complete_level();
                break;
            case ObjectiveType::DefendPoint:
                if (level_timer_ >= level_data_.primary_objective.duration)
                    complete_level();
                break;
            case ObjectiveType::ReachExit:
                if (objective_met_) complete_level();
                break;
        }
    }

private:
    void complete_level() {
        levels_completed_[current_level_ - 1] = true;
        total_score_ += level_data_.completion_score;

        // Par time bonus
        if (level_timer_ <= level_data_.par_time) {
            total_score_ += level_data_.par_time_bonus;
        }

        state_ = is_final_level() ? LevelState::Victory : LevelState::ObjectiveComplete;
        state_timer_ = 0.0f;
    }
};

// ── Level Name Utility ───────────────────────────────────────────────────────

inline const char* environment_name(EnvironmentType type) {
    switch (type) {
        case EnvironmentType::Military:    return "Military Base";
        case EnvironmentType::Underground: return "Underground";
        case EnvironmentType::Urban:       return "Urban Ruins";
        case EnvironmentType::Forest:      return "Irradiated Forest";
        case EnvironmentType::Bunker:      return "Bunker";
        case EnvironmentType::Flooded:     return "Flooded Zone";
        case EnvironmentType::Industrial:  return "Industrial";
        case EnvironmentType::Cathedral:   return "Cathedral";
        case EnvironmentType::Laboratory:  return "Laboratory";
        case EnvironmentType::Bridge:      return "Bridge";
        case EnvironmentType::Hive:        return "Mutant Hive";
        case EnvironmentType::Crater:      return "Ground Zero";
    }
    return "Unknown";
}

} // namespace tps
} // namespace game
} // namespace qe
