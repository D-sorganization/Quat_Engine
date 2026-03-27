#pragma once
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

#include "../../math/Vec3.h"
#include "MutantTypes.h"
#include "TPSWeapons.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// ── Constants ────────────────────────────────────────────────────────────────

inline constexpr int TOTAL_LEVELS = 12;

// ── Spawn Entry ──────────────────────────────────────────────────────────────

struct SpawnEntry {
    MutantType type;
    int count;
    float delay;  // Seconds after level start before spawning
};

// ── Level Objective ──────────────────────────────────────────────────────────

enum class ObjectiveType {
    KillAll,         // Eliminate all enemies
    Survive,         // Survive for a duration
    ReachExit,       // Get to the extraction point
    DefendPoint,     // Protect a location
    BossKill         // Defeat the boss
};

struct LevelObjective {
    ObjectiveType type;
    float duration;     // For Survive objectives (seconds)
    math::Vec3 target;  // For ReachExit/DefendPoint

    static LevelObjective kill_all() {
        return {ObjectiveType::KillAll, 0.0f, math::Vec3::zero()};
    }
    static LevelObjective survive(float seconds) {
        return {ObjectiveType::Survive, seconds, math::Vec3::zero()};
    }
    static LevelObjective reach_exit(math::Vec3 pos) {
        return {ObjectiveType::ReachExit, 0.0f, pos};
    }
    static LevelObjective defend_point(math::Vec3 pos, float seconds) {
        return {ObjectiveType::DefendPoint, seconds, pos};
    }
    static LevelObjective boss_kill() {
        return {ObjectiveType::BossKill, 0.0f, math::Vec3::zero()};
    }
};

// ── Environment Type ─────────────────────────────────────────────────────────

enum class EnvironmentType {
    Military,
    Underground,
    Urban,
    Forest,
    Bunker,
    Flooded,
    Industrial,
    Cathedral,
    Laboratory,
    Bridge,
    Hive,
    Crater
};

// ── Environment Config ───────────────────────────────────────────────────────

struct EnvironmentConfig {
    EnvironmentType type;
    math::Vec3 ambient_color;
    math::Vec3 fog_color;
    float fog_density;
    float fog_start;
    float fog_end;
    math::Vec3 light_direction;
    float light_intensity;
    math::Vec3 sky_color;
    float gravity_modifier;  // 1.0 = normal

    void check_invariants() const {
        assert(fog_density >= 0.0f);
        assert(light_intensity > 0.0f);
        assert(gravity_modifier > 0.0f);
    }
};

// ── Level Data ───────────────────────────────────────────────────────────────

struct LevelData {
    int level_number;
    std::string name;
    std::string description;
    std::string briefing;

    // Environment
    EnvironmentConfig environment;
    math::Vec3 player_start;
    float arena_radius;

    // Enemies
    std::vector<SpawnEntry> spawn_table;
    int total_enemy_count;

    // Objectives
    LevelObjective primary_objective;
    float par_time;  // Target completion time (seconds)

    // Difficulty
    float enemy_health_mult;
    float enemy_damage_mult;
    float enemy_speed_mult;

    // Rewards
    int completion_score;
    int par_time_bonus;

    // Unlocks
    bool unlocks_weapon;
    TPSWeaponType weapon_unlock;

    void check_invariants() const {
        assert(level_number >= 1 && level_number <= TOTAL_LEVELS);
        assert(arena_radius > 0.0f);
        assert(total_enemy_count >= 0);
        assert(enemy_health_mult > 0.0f);
        assert(enemy_damage_mult > 0.0f);
        assert(enemy_speed_mult > 0.0f);
        assert(completion_score > 0);
        environment.check_invariants();
    }

    int count_enemies_of_type(MutantType type) const {
        int count = 0;
        for (const auto& entry : spawn_table) {
            if (entry.type == type) count += entry.count;
        }
        return count;
    }
};

// ── Level Factory ────────────────────────────────────────────────────────────

/** Generate level data for a given level number.
 *  @pre level >= 1 && level <= TOTAL_LEVELS
 *  @post returned data passes check_invariants()
 */
inline LevelData make_level(int level) {
    assert(level >= 1 && level <= TOTAL_LEVELS && "make_level: level out of range");

    LevelData ld{};
    ld.level_number = level;

    // Default scaling
    float diff = 1.0f + (level - 1) * 0.12f;
    ld.enemy_health_mult = diff;
    ld.enemy_damage_mult = 1.0f + (level - 1) * 0.08f;
    ld.enemy_speed_mult = 1.0f + (level - 1) * 0.05f;

    // Default: no weapon unlock
    ld.unlocks_weapon = false;
    ld.weapon_unlock = TPSWeaponType::AssaultRifle;

    switch (level) {
        case 1: {
            ld.name = "Ruined Outpost";
            ld.description = "A bombed-out military checkpoint at the edge of the dead zone.";
            ld.briefing = "Clear the outpost of mutant stragglers. Watch your corners.";
            ld.environment = {
                EnvironmentType::Military,
                {0.15f, 0.12f, 0.1f},    // ambient
                {0.3f, 0.25f, 0.2f},     // fog
                0.02f, 10.0f, 80.0f,     // fog density/start/end
                {0.3f, -0.8f, 0.5f},     // light dir
                0.7f,                      // light intensity
                {0.35f, 0.3f, 0.25f},    // sky
                1.0f                       // gravity
            };
            ld.player_start = {0, 0, 0};
            ld.arena_radius = 40.0f;
            ld.spawn_table = {
                {MutantType::Grunt, 8, 0.0f},
                {MutantType::Hound, 3, 10.0f}
            };
            ld.total_enemy_count = 11;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 120.0f;
            ld.completion_score = 500;
            ld.par_time_bonus = 200;
            break;
        }

        case 2: {
            ld.name = "Ashen Subway";
            ld.description = "The old metro tunnels, choked with ash and crawling horrors.";
            ld.briefing = "Navigate the tunnels. Crawlers hunt in packs — stay alert.";
            ld.environment = {
                EnvironmentType::Underground,
                {0.05f, 0.05f, 0.08f},
                {0.1f, 0.08f, 0.12f},
                0.06f, 5.0f, 40.0f,
                {0.0f, -1.0f, 0.0f},
                0.3f,
                {0.05f, 0.05f, 0.08f},
                1.0f
            };
            ld.player_start = {0, 0, 5};
            ld.arena_radius = 50.0f;
            ld.spawn_table = {
                {MutantType::Crawler, 10, 0.0f},
                {MutantType::Grunt, 5, 5.0f},
                {MutantType::Stalker, 2, 15.0f}
            };
            ld.total_enemy_count = 17;
            ld.primary_objective = LevelObjective::reach_exit({80, 0, 0});
            ld.par_time = 180.0f;
            ld.completion_score = 800;
            ld.par_time_bonus = 350;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::CombatShotgun;
            break;
        }

        case 3: {
            ld.name = "Crater Market";
            ld.description = "An open-air market around a bomb crater. Good sightlines, no cover.";
            ld.briefing = "Mixed threat environment. Use range to your advantage.";
            ld.environment = {
                EnvironmentType::Urban,
                {0.2f, 0.18f, 0.15f},
                {0.4f, 0.35f, 0.3f},
                0.015f, 15.0f, 100.0f,
                {0.5f, -0.7f, 0.3f},
                0.85f,
                {0.5f, 0.45f, 0.35f},
                1.0f
            };
            ld.player_start = {0, 0, -10};
            ld.arena_radius = 60.0f;
            ld.spawn_table = {
                {MutantType::Grunt, 8, 0.0f},
                {MutantType::Spitter, 4, 5.0f},
                {MutantType::Hound, 5, 10.0f},
                {MutantType::Crawler, 3, 15.0f}
            };
            ld.total_enemy_count = 20;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 200.0f;
            ld.completion_score = 1000;
            ld.par_time_bonus = 400;
            break;
        }

        case 4: {
            ld.name = "Irradiated Forest";
            ld.description = "Twisted trees glow with residual radiation. Stalkers lurk in the canopy.";
            ld.briefing = "Ambush territory. Watch for Stalkers — they shimmer before they strike.";
            ld.environment = {
                EnvironmentType::Forest,
                {0.08f, 0.15f, 0.05f},
                {0.15f, 0.25f, 0.1f},
                0.04f, 8.0f, 50.0f,
                {0.2f, -0.6f, 0.4f},
                0.5f,
                {0.12f, 0.2f, 0.08f},
                1.0f
            };
            ld.player_start = {0, 0, 0};
            ld.arena_radius = 70.0f;
            ld.spawn_table = {
                {MutantType::Stalker, 6, 0.0f},
                {MutantType::Crawler, 5, 8.0f},
                {MutantType::Hound, 6, 12.0f},
                {MutantType::Spitter, 3, 20.0f}
            };
            ld.total_enemy_count = 20;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 240.0f;
            ld.completion_score = 1200;
            ld.par_time_bonus = 500;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::MarksmanRifle;
            break;
        }

        case 5: {
            ld.name = "Bunker Descent";
            ld.description = "A vertical descent into a reinforced bunker. Brutes guard every floor.";
            ld.briefing = "Tight quarters. Brutes can't be staggered — dodge or die.";
            ld.environment = {
                EnvironmentType::Bunker,
                {0.08f, 0.08f, 0.1f},
                {0.12f, 0.1f, 0.15f},
                0.05f, 4.0f, 35.0f,
                {0.0f, -1.0f, 0.1f},
                0.35f,
                {0.08f, 0.08f, 0.12f},
                1.0f
            };
            ld.player_start = {0, 10, 0};
            ld.arena_radius = 35.0f;
            ld.spawn_table = {
                {MutantType::Brute, 3, 0.0f},
                {MutantType::Grunt, 10, 3.0f},
                {MutantType::Crawler, 6, 10.0f},
                {MutantType::Screamer, 1, 25.0f}
            };
            ld.total_enemy_count = 20;
            ld.primary_objective = LevelObjective::reach_exit({0, -20, 0});
            ld.par_time = 300.0f;
            ld.completion_score = 1500;
            ld.par_time_bonus = 600;
            break;
        }

        case 6: {
            ld.name = "Flooded District";
            ld.description = "Half-submerged city blocks. Water slows movement but hides threats.";
            ld.briefing = "Platforming required. Hounds leap between rooftops.";
            ld.environment = {
                EnvironmentType::Flooded,
                {0.1f, 0.12f, 0.18f},
                {0.2f, 0.25f, 0.35f},
                0.03f, 10.0f, 60.0f,
                {0.3f, -0.5f, 0.6f},
                0.6f,
                {0.25f, 0.3f, 0.4f},
                1.0f
            };
            ld.player_start = {0, 5, 0};
            ld.arena_radius = 65.0f;
            ld.spawn_table = {
                {MutantType::Hound, 8, 0.0f},
                {MutantType::Spitter, 5, 5.0f},
                {MutantType::Crawler, 4, 10.0f},
                {MutantType::Amalgam, 2, 20.0f}
            };
            ld.total_enemy_count = 19;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 260.0f;
            ld.completion_score = 1600;
            ld.par_time_bonus = 650;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::PlasmaCaster;
            break;
        }

        case 7: {
            ld.name = "Rail Yards";
            ld.description = "Rusted trains and twisted tracks. Perfect cover — for both sides.";
            ld.briefing = "Cover-based engagement. Screamers buff nearby mutants.";
            ld.environment = {
                EnvironmentType::Industrial,
                {0.12f, 0.1f, 0.08f},
                {0.25f, 0.2f, 0.15f},
                0.025f, 12.0f, 70.0f,
                {0.4f, -0.7f, 0.2f},
                0.75f,
                {0.35f, 0.28f, 0.22f},
                1.0f
            };
            ld.player_start = {-20, 0, 0};
            ld.arena_radius = 75.0f;
            ld.spawn_table = {
                {MutantType::Grunt, 8, 0.0f},
                {MutantType::Screamer, 3, 5.0f},
                {MutantType::Brute, 2, 15.0f},
                {MutantType::Spitter, 4, 10.0f},
                {MutantType::Stalker, 3, 20.0f}
            };
            ld.total_enemy_count = 20;
            ld.primary_objective = LevelObjective::defend_point({0, 0, 0}, 120.0f);
            ld.par_time = 300.0f;
            ld.completion_score = 1800;
            ld.par_time_bonus = 700;
            break;
        }

        case 8: {
            ld.name = "Cathedral Ruins";
            ld.description = "Shattered stained glass, collapsed arches. Amalgams nest in the rafters.";
            ld.briefing = "Vertical combat. Amalgams dive-bomb from above — keep moving.";
            ld.environment = {
                EnvironmentType::Cathedral,
                {0.12f, 0.08f, 0.15f},
                {0.2f, 0.15f, 0.25f},
                0.02f, 8.0f, 60.0f,
                {0.1f, -0.9f, 0.2f},
                0.45f,
                {0.18f, 0.12f, 0.22f},
                1.0f
            };
            ld.player_start = {0, 0, 15};
            ld.arena_radius = 55.0f;
            ld.spawn_table = {
                {MutantType::Amalgam, 5, 0.0f},
                {MutantType::Stalker, 4, 8.0f},
                {MutantType::Grunt, 6, 5.0f},
                {MutantType::Screamer, 2, 15.0f},
                {MutantType::Hound, 4, 12.0f}
            };
            ld.total_enemy_count = 21;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 280.0f;
            ld.completion_score = 2000;
            ld.par_time_bonus = 800;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::SubmachineGun;
            break;
        }

        case 9: {
            ld.name = "Laboratory Complex";
            ld.description = "The source of the mutations. Experimental horrors stalk the corridors.";
            ld.briefing = "Behemoth sighted. Experimental mutants are unpredictable.";
            ld.environment = {
                EnvironmentType::Laboratory,
                {0.15f, 0.18f, 0.2f},
                {0.1f, 0.15f, 0.2f},
                0.03f, 6.0f, 45.0f,
                {0.0f, -1.0f, 0.0f},
                0.55f,
                {0.12f, 0.15f, 0.2f},
                1.0f
            };
            ld.player_start = {0, 0, -5};
            ld.arena_radius = 50.0f;
            ld.spawn_table = {
                {MutantType::Behemoth, 1, 0.0f},
                {MutantType::Stalker, 4, 5.0f},
                {MutantType::Spitter, 4, 8.0f},
                {MutantType::Crawler, 6, 3.0f},
                {MutantType::Brute, 2, 20.0f},
                {MutantType::Screamer, 2, 15.0f}
            };
            ld.total_enemy_count = 19;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 360.0f;
            ld.completion_score = 2500;
            ld.par_time_bonus = 1000;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::GrenadeLauncher;
            break;
        }

        case 10: {
            ld.name = "Bridge of Bones";
            ld.description = "A massive destroyed bridge spanning the irradiated river. No retreat.";
            ld.briefing = "Horde defense. Hold the bridge — they come from both sides.";
            ld.environment = {
                EnvironmentType::Bridge,
                {0.18f, 0.12f, 0.08f},
                {0.35f, 0.25f, 0.15f},
                0.02f, 15.0f, 90.0f,
                {0.5f, -0.6f, 0.3f},
                0.8f,
                {0.4f, 0.3f, 0.2f},
                1.0f
            };
            ld.player_start = {0, 5, 0};
            ld.arena_radius = 80.0f;
            ld.spawn_table = {
                {MutantType::Grunt, 12, 0.0f},
                {MutantType::Hound, 8, 5.0f},
                {MutantType::Crawler, 6, 10.0f},
                {MutantType::Brute, 3, 20.0f},
                {MutantType::Spitter, 4, 15.0f},
                {MutantType::Amalgam, 3, 25.0f},
                {MutantType::Screamer, 2, 30.0f}
            };
            ld.total_enemy_count = 38;
            ld.primary_objective = LevelObjective::survive(180.0f);
            ld.par_time = 300.0f;
            ld.completion_score = 3000;
            ld.par_time_bonus = 1200;
            break;
        }

        case 11: {
            ld.name = "The Spire";
            ld.description = "A towering mutant hive. Every enemy type nests here.";
            ld.briefing = "Full escalation. Every mutant type is present. Fight to the top.";
            ld.environment = {
                EnvironmentType::Hive,
                {0.1f, 0.05f, 0.08f},
                {0.18f, 0.08f, 0.12f},
                0.04f, 6.0f, 40.0f,
                {0.1f, -0.8f, 0.3f},
                0.4f,
                {0.12f, 0.06f, 0.1f},
                1.0f
            };
            ld.player_start = {0, 0, 0};
            ld.arena_radius = 45.0f;
            ld.spawn_table = {
                {MutantType::Grunt, 8, 0.0f},
                {MutantType::Crawler, 6, 3.0f},
                {MutantType::Hound, 5, 6.0f},
                {MutantType::Stalker, 4, 10.0f},
                {MutantType::Spitter, 4, 15.0f},
                {MutantType::Screamer, 3, 20.0f},
                {MutantType::Brute, 3, 25.0f},
                {MutantType::Amalgam, 3, 30.0f},
                {MutantType::Behemoth, 1, 40.0f}
            };
            ld.total_enemy_count = 37;
            ld.primary_objective = LevelObjective::kill_all();
            ld.par_time = 420.0f;
            ld.completion_score = 4000;
            ld.par_time_bonus = 1500;
            ld.unlocks_weapon = true;
            ld.weapon_unlock = TPSWeaponType::TeslaCoil;
            break;
        }

        case 12: {
            ld.name = "Ground Zero";
            ld.description = "The original detonation crater. The Apex awaits at its heart.";
            ld.briefing = "Final mission. Destroy The Apex. End the mutant plague.";
            ld.environment = {
                EnvironmentType::Crater,
                {0.2f, 0.1f, 0.05f},
                {0.4f, 0.2f, 0.1f},
                0.015f, 20.0f, 120.0f,
                {0.0f, -0.5f, 0.5f},
                1.0f,
                {0.5f, 0.25f, 0.1f},
                1.0f
            };
            ld.player_start = {0, 0, 30};
            ld.arena_radius = 60.0f;
            ld.spawn_table = {
                {MutantType::Apex, 1, 0.0f},
                {MutantType::Grunt, 6, 10.0f},
                {MutantType::Hound, 4, 15.0f},
                {MutantType::Brute, 2, 25.0f},
                {MutantType::Amalgam, 3, 20.0f},
                {MutantType::Screamer, 2, 30.0f}
            };
            ld.total_enemy_count = 18;
            ld.primary_objective = LevelObjective::boss_kill();
            ld.par_time = 480.0f;
            ld.completion_score = 5000;
            ld.par_time_bonus = 2000;
            ld.enemy_health_mult = 2.0f;
            ld.enemy_damage_mult = 1.8f;
            ld.enemy_speed_mult = 1.4f;
            break;
        }
    }

    ld.check_invariants();
    return ld;
}

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
