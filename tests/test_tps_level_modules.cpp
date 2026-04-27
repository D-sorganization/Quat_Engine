/**
 * @file test_tps_level_modules.cpp
 * @brief Characterization tests for split TPS level type and loader headers.
 */

#include "test_framework.h"

#include "../src/game/tps/LevelLoader.h"
#include "../src/game/tps/LevelTypes.h"

#include <iostream>

using namespace qe::game::tps;

void test_level_types_are_standalone_contracts() {
    LevelData level{};
    level.level_number = 1;
    level.name = "Contract";
    level.description = "Data contract only";
    level.briefing = "Brief";
    level.environment = {
        EnvironmentType::Military,
        qe::math::Vec3{0.1f, 0.1f, 0.1f},
        qe::math::Vec3{0.2f, 0.2f, 0.2f},
        0.01f,
        1.0f,
        10.0f,
        qe::math::Vec3{0.0f, -1.0f, 0.0f},
        1.0f,
        qe::math::Vec3{0.3f, 0.3f, 0.3f},
        1.0f,
    };
    level.player_start = qe::math::Vec3::zero();
    level.arena_radius = 10.0f;
    level.spawn_table = {{MutantType::Grunt, 2, 0.0f}};
    level.total_enemy_count = 2;
    level.primary_objective = LevelObjective::kill_all();
    level.par_time = 60.0f;
    level.enemy_health_mult = 1.0f;
    level.enemy_damage_mult = 1.0f;
    level.enemy_speed_mult = 1.0f;
    level.completion_score = 100;
    level.par_time_bonus = 25;
    level.unlocks_weapon = false;
    level.weapon_unlock = TPSWeaponType::AssaultRifle;

    level.check_invariants();
    ASSERT_TRUE(level.count_enemies_of_type(MutantType::Grunt) == 2);
}

void test_level_loader_loads_json_without_state_machine() {
    auto level = make_level(1);
    ASSERT_TRUE(level.level_number == 1);
    ASSERT_TRUE(level.primary_objective.type == ObjectiveType::KillAll);
    ASSERT_TRUE(level.total_enemy_count > 0);
}

int main() {
    std::cout << "\n=== QuatEngine TPS Level Module Tests ===" << std::endl;

    RUN_TEST(test_level_types_are_standalone_contracts);
    RUN_TEST(test_level_loader_loads_json_without_state_machine);

    return TEST_REPORT();
}
