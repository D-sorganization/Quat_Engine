#pragma once
/**
 * @file LevelTypes.h
 * @brief Pure data contracts for TPS level configuration and objectives.
 */

#include "../../math/Vec3.h"
#include "MutantTypes.h"
#include "TPSWeapons.h"

#include <cassert>
#include <string>
#include <vector>

namespace qe {
namespace game {
namespace tps {

inline constexpr int TOTAL_LEVELS = 12;

struct SpawnEntry {
    MutantType type;
    int count;
    float delay;  // Seconds after level start before spawning
};

enum class ObjectiveType {
    KillAll,
    Survive,
    ReachExit,
    DefendPoint,
    BossKill
};

struct LevelObjective {
    ObjectiveType type;
    float duration;
    math::Vec3 target;

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
    float gravity_modifier;

    void check_invariants() const {
        assert(fog_density >= 0.0f);
        assert(light_intensity > 0.0f);
        assert(gravity_modifier > 0.0f);
    }
};

struct LevelData {
    int level_number;
    std::string name;
    std::string description;
    std::string briefing;

    EnvironmentConfig environment;
    math::Vec3 player_start;
    float arena_radius;

    std::vector<SpawnEntry> spawn_table;
    int total_enemy_count;

    LevelObjective primary_objective;
    float par_time;

    float enemy_health_mult;
    float enemy_damage_mult;
    float enemy_speed_mult;

    int completion_score;
    int par_time_bonus;

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
            if (entry.type == type) {
                count += entry.count;
            }
        }
        return count;
    }
};

} // namespace tps
} // namespace game
} // namespace qe
