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
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
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

// ── Level JSON Loader (internal helpers) ─────────────────────────────────────

namespace detail {

// Resolve the data directory relative to this header file's location at
// compile time, or from the QE_LEVELS_DIR environment variable at runtime.
inline std::string levels_data_dir() {
    const char* env = std::getenv("QE_LEVELS_DIR");
    if (env && env[0] != '\0') return std::string(env);
    // Default: <repo-root>/data/levels, derived from __FILE__
    // __FILE__ resolves to .../src/game/tps/LevelSystem.h — walk up 3 levels.
    std::string path = __FILE__;
    for (int i = 0; i < 3; ++i) {
        auto pos = path.find_last_of("/\\");
        if (pos == std::string::npos) break;
        path = path.substr(0, pos);
    }
    return path + "/data/levels";
}

// Strip leading/trailing whitespace.
inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Extract the raw JSON value string associated with a top-level key.
// Works for simple scalar values and inline array literals.
// Returns empty string if key not found.
inline std::string json_value(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return {};
    if (json[pos] == '[') {
        // Find matching closing bracket (handles nested arrays/objects)
        int depth = 0;
        std::size_t end = pos;
        for (; end < json.size(); ++end) {
            if (json[end] == '[' || json[end] == '{') ++depth;
            else if (json[end] == ']' || json[end] == '}') { --depth; if (depth == 0) break; }
        }
        return json.substr(pos, end - pos + 1);
    }
    if (json[pos] == '{') {
        // Find matching closing brace
        int depth = 0;
        std::size_t end = pos;
        for (; end < json.size(); ++end) {
            if (json[end] == '{') ++depth;
            else if (json[end] == '}') { --depth; if (depth == 0) break; }
        }
        return json.substr(pos, end - pos + 1);
    }
    auto end = json.find_first_of(",\n}", pos);
    if (end == std::string::npos) end = json.size();
    return trim(json.substr(pos, end - pos));
}

// Parse a JSON string value (strips surrounding quotes, handles simple escapes).
inline std::string json_string(const std::string& raw) {
    auto s = trim(raw);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    // Unescape em-dash and basic sequences
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

// Parse a float from a raw JSON token.
inline float json_float(const std::string& raw) {
    return std::stof(trim(raw));
}

// Parse an int from a raw JSON token.
inline int json_int(const std::string& raw) {
    return std::stoi(trim(raw));
}

// Parse a bool from a raw JSON token.
inline bool json_bool(const std::string& raw) {
    auto s = trim(raw);
    return s == "true";
}

// Parse a [x, y, z] JSON array into a Vec3.
inline math::Vec3 json_vec3(const std::string& raw) {
    auto s = trim(raw);
    // Remove brackets
    if (!s.empty() && s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s = s.substr(0, s.size() - 1);
    std::istringstream ss(s);
    float x = 0, y = 0, z = 0;
    char comma = 0;
    ss >> x >> comma >> y >> comma >> z;
    return {x, y, z};
}

// Map string -> EnvironmentType
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

// Map string -> MutantType
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

// Map string -> TPSWeaponType
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

// Parse the spawn_table JSON array (array of objects).
inline std::vector<SpawnEntry> parse_spawn_table(const std::string& json) {
    // Find the spawn_table array value
    std::string arr = json_value(json, "spawn_table");
    std::vector<SpawnEntry> table;
    if (arr.empty() || arr.front() != '[') return table;
    // Iterate over objects within the array
    std::size_t pos = 1;
    while (pos < arr.size()) {
        auto obj_start = arr.find('{', pos);
        if (obj_start == std::string::npos) break;
        auto obj_end = arr.find('}', obj_start);
        if (obj_end == std::string::npos) break;
        std::string obj = arr.substr(obj_start, obj_end - obj_start + 1);
        SpawnEntry entry{};
        entry.type  = parse_mutant_type(json_string(json_value(obj, "type")));
        entry.count = json_int(json_value(obj, "count"));
        entry.delay = json_float(json_value(obj, "delay"));
        table.push_back(entry);
        pos = obj_end + 1;
    }
    return table;
}

// Parse the primary_objective object.
inline LevelObjective parse_objective(const std::string& json) {
    std::string obj = json_value(json, "primary_objective");
    std::string type_str = json_string(json_value(obj, "type"));
    if (type_str == "KillAll")     return LevelObjective::kill_all();
    if (type_str == "BossKill")    return LevelObjective::boss_kill();
    if (type_str == "Survive") {
        float dur = json_float(json_value(obj, "duration"));
        return LevelObjective::survive(dur);
    }
    if (type_str == "ReachExit") {
        math::Vec3 target = json_vec3(json_value(obj, "target"));
        return LevelObjective::reach_exit(target);
    }
    if (type_str == "DefendPoint") {
        math::Vec3 target = json_vec3(json_value(obj, "target"));
        float dur = json_float(json_value(obj, "duration"));
        return LevelObjective::defend_point(target, dur);
    }
    throw std::runtime_error("Unknown ObjectiveType: " + type_str);
}

// Parse the environment sub-object.
inline EnvironmentConfig parse_environment(const std::string& json) {
    std::string env = json_value(json, "environment");
    EnvironmentConfig ec{};
    ec.type             = parse_env_type(json_string(json_value(env, "type")));
    ec.ambient_color    = json_vec3(json_value(env, "ambient_color"));
    ec.fog_color        = json_vec3(json_value(env, "fog_color"));
    ec.fog_density      = json_float(json_value(env, "fog_density"));
    ec.fog_start        = json_float(json_value(env, "fog_start"));
    ec.fog_end          = json_float(json_value(env, "fog_end"));
    ec.light_direction  = json_vec3(json_value(env, "light_direction"));
    ec.light_intensity  = json_float(json_value(env, "light_intensity"));
    ec.sky_color        = json_vec3(json_value(env, "sky_color"));
    ec.gravity_modifier = json_float(json_value(env, "gravity_modifier"));
    return ec;
}

// Load the full JSON text for a level file.
inline std::string load_level_json(int level) {
    char filename[64];
    std::snprintf(filename, sizeof(filename), "/level%02d.json", level);
    std::string path = levels_data_dir() + filename;
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("make_level: cannot open " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace detail

// ── Level Factory ────────────────────────────────────────────────────────────

/** Load level data for a given level number from a JSON data file.
 *
 *  Data files live at <repo-root>/data/levels/level##.json, or in the
 *  directory pointed to by the QE_LEVELS_DIR environment variable.
 *
 *  @pre  level >= 1 && level <= TOTAL_LEVELS
 *  @post returned data passes check_invariants()
 */
inline LevelData make_level(int level) {
    assert(level >= 1 && level <= TOTAL_LEVELS && "make_level: level out of range");

    const std::string json = detail::load_level_json(level);

    LevelData ld{};
    ld.level_number   = detail::json_int(detail::json_value(json, "level_number"));
    ld.name           = detail::json_string(detail::json_value(json, "name"));
    ld.description    = detail::json_string(detail::json_value(json, "description"));
    ld.briefing       = detail::json_string(detail::json_value(json, "briefing"));
    ld.environment    = detail::parse_environment(json);
    ld.player_start   = detail::json_vec3(detail::json_value(json, "player_start"));
    ld.arena_radius   = detail::json_float(detail::json_value(json, "arena_radius"));
    ld.spawn_table    = detail::parse_spawn_table(json);
    ld.total_enemy_count = detail::json_int(detail::json_value(json, "total_enemy_count"));
    ld.primary_objective = detail::parse_objective(json);
    ld.par_time          = detail::json_float(detail::json_value(json, "par_time"));
    ld.completion_score  = detail::json_int(detail::json_value(json, "completion_score"));
    ld.par_time_bonus    = detail::json_int(detail::json_value(json, "par_time_bonus"));
    ld.unlocks_weapon    = detail::json_bool(detail::json_value(json, "unlocks_weapon"));
    ld.weapon_unlock     = TPSWeaponType::AssaultRifle;  // default
    if (ld.unlocks_weapon) {
        ld.weapon_unlock = detail::parse_weapon_type(
            detail::json_string(detail::json_value(json, "weapon_unlock")));
    }

    // Per-level difficulty overrides (optional fields); fall back to formula.
    {
        auto hm = detail::json_value(json, "enemy_health_mult");
        auto dm = detail::json_value(json, "enemy_damage_mult");
        auto sm = detail::json_value(json, "enemy_speed_mult");
        ld.enemy_health_mult = hm.empty()
            ? 1.0f + (level - 1) * 0.12f
            : detail::json_float(hm);
        ld.enemy_damage_mult = dm.empty()
            ? 1.0f + (level - 1) * 0.08f
            : detail::json_float(dm);
        ld.enemy_speed_mult  = sm.empty()
            ? 1.0f + (level - 1) * 0.05f
            : detail::json_float(sm);
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
