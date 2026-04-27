// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file LevelLoader.h
 * @brief JSON-backed TPS level loader and parser helpers.
 */

#include "LevelTypes.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace qe {
namespace game {
namespace tps {
namespace detail {

inline std::string levels_data_dir() {
    const char* env = std::getenv("QE_LEVELS_DIR");
    if (env && env[0] != '\0') {
        return std::string(env);
    }

    std::string path = __FILE__;
    for (int i = 0; i < 3; ++i) {
        auto pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            break;
        }
        path = path.substr(0, pos);
    }
    return path + "/data/levels";
}

inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::string json_value(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) {
        return {};
    }
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return {};
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }
    if (pos >= json.size()) {
        return {};
    }
    if (json[pos] == '[' || json[pos] == '{') {
        int depth = 0;
        std::size_t end = pos;
        for (; end < json.size(); ++end) {
            if (json[end] == '[' || json[end] == '{') {
                ++depth;
            } else if (json[end] == ']' || json[end] == '}') {
                --depth;
                if (depth == 0) {
                    break;
                }
            }
        }
        return json.substr(pos, end - pos + 1);
    }

    auto end = json.find_first_of(",\n}", pos);
    if (end == std::string::npos) {
        end = json.size();
    }
    return trim(json.substr(pos, end - pos));
}

inline std::string json_string(const std::string& raw) {
    auto s = trim(raw);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }

    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                default:  out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

inline float json_float(const std::string& raw) {
    return std::stof(trim(raw));
}

inline int json_int(const std::string& raw) {
    return std::stoi(trim(raw));
}

inline bool json_bool(const std::string& raw) {
    return trim(raw) == "true";
}

inline math::Vec3 json_vec3(const std::string& raw) {
    auto s = trim(raw);
    if (!s.empty() && s.front() == '[') {
        s = s.substr(1);
    }
    if (!s.empty() && s.back() == ']') {
        s = s.substr(0, s.size() - 1);
    }
    std::istringstream ss(s);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    char comma = 0;
    ss >> x >> comma >> y >> comma >> z;
    return {x, y, z};
}

inline EnvironmentType parse_env_type(const std::string& s) {
    if (s == "Military")    return EnvironmentType::Military;
    if (s == "Underground") return EnvironmentType::Underground;
    if (s == "Urban")       return EnvironmentType::Urban;
    if (s == "Forest")      return EnvironmentType::Forest;
    if (s == "Bunker")      return EnvironmentType::Bunker;
    if (s == "Flooded")     return EnvironmentType::Flooded;
    if (s == "Industrial")  return EnvironmentType::Industrial;
    if (s == "Cathedral")   return EnvironmentType::Cathedral;
    if (s == "Laboratory")  return EnvironmentType::Laboratory;
    if (s == "Bridge")      return EnvironmentType::Bridge;
    if (s == "Hive")        return EnvironmentType::Hive;
    if (s == "Crater")      return EnvironmentType::Crater;
    throw std::runtime_error("Unknown EnvironmentType: " + s);
}

inline MutantType parse_mutant_type(const std::string& s) {
    if (s == "Grunt")    return MutantType::Grunt;
    if (s == "Crawler")  return MutantType::Crawler;
    if (s == "Brute")    return MutantType::Brute;
    if (s == "Stalker")  return MutantType::Stalker;
    if (s == "Spitter")  return MutantType::Spitter;
    if (s == "Screamer") return MutantType::Screamer;
    if (s == "Hound")    return MutantType::Hound;
    if (s == "Amalgam")  return MutantType::Amalgam;
    if (s == "Behemoth") return MutantType::Behemoth;
    if (s == "Apex")     return MutantType::Apex;
    throw std::runtime_error("Unknown MutantType: " + s);
}

inline TPSWeaponType parse_weapon_type(const std::string& s) {
    if (s == "AssaultRifle")    return TPSWeaponType::AssaultRifle;
    if (s == "CombatShotgun")   return TPSWeaponType::CombatShotgun;
    if (s == "PlasmaCaster")    return TPSWeaponType::PlasmaCaster;
    if (s == "MarksmanRifle")   return TPSWeaponType::MarksmanRifle;
    if (s == "SubmachineGun")   return TPSWeaponType::SubmachineGun;
    if (s == "GrenadeLauncher") return TPSWeaponType::GrenadeLauncher;
    if (s == "TeslaCoil")       return TPSWeaponType::TeslaCoil;
    throw std::runtime_error("Unknown TPSWeaponType: " + s);
}

inline std::vector<SpawnEntry> parse_spawn_table(const std::string& json) {
    std::string arr = json_value(json, "spawn_table");
    std::vector<SpawnEntry> table;
    if (arr.empty() || arr.front() != '[') {
        return table;
    }

    std::size_t pos = 1;
    while (pos < arr.size()) {
        auto obj_start = arr.find('{', pos);
        if (obj_start == std::string::npos) {
            break;
        }
        auto obj_end = arr.find('}', obj_start);
        if (obj_end == std::string::npos) {
            break;
        }

        std::string obj = arr.substr(obj_start, obj_end - obj_start + 1);
        SpawnEntry entry{};
        entry.type = parse_mutant_type(json_string(json_value(obj, "type")));
        entry.count = json_int(json_value(obj, "count"));
        entry.delay = json_float(json_value(obj, "delay"));
        table.push_back(entry);
        pos = obj_end + 1;
    }
    return table;
}

inline LevelObjective parse_objective(const std::string& json) {
    std::string obj = json_value(json, "primary_objective");
    std::string type_str = json_string(json_value(obj, "type"));
    if (type_str == "KillAll") {
        return LevelObjective::kill_all();
    }
    if (type_str == "BossKill") {
        return LevelObjective::boss_kill();
    }
    if (type_str == "Survive") {
        return LevelObjective::survive(json_float(json_value(obj, "duration")));
    }
    if (type_str == "ReachExit") {
        return LevelObjective::reach_exit(json_vec3(json_value(obj, "target")));
    }
    if (type_str == "DefendPoint") {
        math::Vec3 target = json_vec3(json_value(obj, "target"));
        return LevelObjective::defend_point(target, json_float(json_value(obj, "duration")));
    }
    throw std::runtime_error("Unknown ObjectiveType: " + type_str);
}

inline EnvironmentConfig parse_environment(const std::string& json) {
    std::string env = json_value(json, "environment");
    EnvironmentConfig ec{};
    ec.type = parse_env_type(json_string(json_value(env, "type")));
    ec.ambient_color = json_vec3(json_value(env, "ambient_color"));
    ec.fog_color = json_vec3(json_value(env, "fog_color"));
    ec.fog_density = json_float(json_value(env, "fog_density"));
    ec.fog_start = json_float(json_value(env, "fog_start"));
    ec.fog_end = json_float(json_value(env, "fog_end"));
    ec.light_direction = json_vec3(json_value(env, "light_direction"));
    ec.light_intensity = json_float(json_value(env, "light_intensity"));
    ec.sky_color = json_vec3(json_value(env, "sky_color"));
    ec.gravity_modifier = json_float(json_value(env, "gravity_modifier"));
    return ec;
}

inline std::string load_level_json(int level) {
    char filename[64];
    std::snprintf(filename, sizeof(filename), "/level%02d.json", level);
    std::string path = levels_data_dir() + filename;
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("make_level: cannot open " + path);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace detail

inline LevelData make_level(int level) {
    assert(level >= 1 && level <= TOTAL_LEVELS && "make_level: level out of range");

    const std::string json = detail::load_level_json(level);

    LevelData ld{};
    ld.level_number = detail::json_int(detail::json_value(json, "level_number"));
    ld.name = detail::json_string(detail::json_value(json, "name"));
    ld.description = detail::json_string(detail::json_value(json, "description"));
    ld.briefing = detail::json_string(detail::json_value(json, "briefing"));
    ld.environment = detail::parse_environment(json);
    ld.player_start = detail::json_vec3(detail::json_value(json, "player_start"));
    ld.arena_radius = detail::json_float(detail::json_value(json, "arena_radius"));
    ld.spawn_table = detail::parse_spawn_table(json);
    ld.total_enemy_count = detail::json_int(detail::json_value(json, "total_enemy_count"));
    ld.primary_objective = detail::parse_objective(json);
    ld.par_time = detail::json_float(detail::json_value(json, "par_time"));
    ld.completion_score = detail::json_int(detail::json_value(json, "completion_score"));
    ld.par_time_bonus = detail::json_int(detail::json_value(json, "par_time_bonus"));
    ld.unlocks_weapon = detail::json_bool(detail::json_value(json, "unlocks_weapon"));
    ld.weapon_unlock = TPSWeaponType::AssaultRifle;
    if (ld.unlocks_weapon) {
        ld.weapon_unlock = detail::parse_weapon_type(
            detail::json_string(detail::json_value(json, "weapon_unlock")));
    }

    auto hm = detail::json_value(json, "enemy_health_mult");
    auto dm = detail::json_value(json, "enemy_damage_mult");
    auto sm = detail::json_value(json, "enemy_speed_mult");
    ld.enemy_health_mult = hm.empty()
        ? 1.0f + (level - 1) * 0.12f
        : detail::json_float(hm);
    ld.enemy_damage_mult = dm.empty()
        ? 1.0f + (level - 1) * 0.08f
        : detail::json_float(dm);
    ld.enemy_speed_mult = sm.empty()
        ? 1.0f + (level - 1) * 0.05f
        : detail::json_float(sm);

    ld.check_invariants();
    return ld;
}

} // namespace tps
} // namespace game
} // namespace qe
