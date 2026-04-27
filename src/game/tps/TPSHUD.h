#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file TPSHUD.h
 * @brief HUD layout data for TPS game — pure state, no rendering dependency.
 *
 * Computes HUD element positions, sizes, colors, and text content from
 * game state. The renderer reads these to draw actual UI elements.
 *
 * Elements:
 *   - Health bar with damage flash
 *   - Stamina bar with depletion warning
 *   - Ammo counter (magazine / reserve)
 *   - Weapon name display
 *   - Lock-on reticle with wobble offset
 *   - Enemy health bars (for targeted/nearby enemies)
 *   - Objective progress indicator
 *   - Score and combo display
 *   - Level info (name, briefing)
 *   - Hit markers and damage direction
 *   - Low health vignette
 */

#include "../../math/Vec3.h"
#include "PlayerController.h"
#include "LevelSystem.h"
#include "TPSCombat.h"
#include "DamageFeedback.h"
#include "MutantTypes.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// ── HUD Bar Element ──────────────────────────────────────────────────────────

struct HUDBarData {
    float x, y;           // NDC position (-1 to 1)
    float width, height;
    float fill;           // 0 to 1
    math::Vec3 fill_color;
    math::Vec3 bg_color;
    bool visible = true;
    bool flash = false;
    float flash_intensity = 0.0f;
};

// ── HUD Text Element ─────────────────────────────────────────────────────────

struct HUDTextData {
    float x, y;
    std::string text;
    math::Vec3 color;
    float scale = 1.0f;
    bool visible = true;
};

// ── Lock-On Reticle ──────────────────────────────────────────────────────────

struct LockOnReticleData {
    bool visible = false;
    float x = 0.0f, y = 0.0f;   // Screen position (-1 to 1)
    float wobble_x = 0.0f;
    float wobble_y = 0.0f;
    float size = 0.06f;
    math::Vec3 color{1.0f, 0.3f, 0.2f};
    float pulse = 0.0f;
};

// ── Enemy Health Bar ─────────────────────────────────────────────────────────

struct EnemyHealthBarData {
    float x, y;
    float width = 0.08f;
    float height = 0.008f;
    float fill;
    std::string name;
    bool is_boss = false;
    bool visible = true;
};

// ── Crosshair ────────────────────────────────────────────────────────────────

struct CrosshairData {
    float x = 0.0f, y = 0.0f;
    float size = 0.02f;
    float gap = 0.01f;
    math::Vec3 color{0.9f, 1.0f, 0.9f};
    bool spread_indicator = false;
    float spread_amount = 0.0f;
};

// ── Full HUD State ───────────────────────────────────────────────────────────

struct TPSHUDState {
    HUDBarData health_bar;
    HUDBarData stamina_bar;
    HUDBarData boss_health_bar;
    CrosshairData crosshair;
    LockOnReticleData lock_on_reticle;
    std::vector<EnemyHealthBarData> enemy_bars;

    HUDTextData ammo_text;
    HUDTextData weapon_name;
    HUDTextData score_text;
    HUDTextData combo_text;
    HUDTextData objective_text;
    HUDTextData level_name;

    float low_health_vignette = 0.0f;
    float hit_marker_intensity = 0.0f;
    bool hit_marker_kill = false;
    std::vector<float> damage_indicator_angles;
    std::vector<float> damage_indicator_intensities;

    int weapon_slot_current = 0;
    int weapon_slot_total = 0;
};

// ── HUD Builder ──────────────────────────────────────────────────────────────

/** Build HUD state from game state each frame.
 *  Pure function: reads game state, produces HUD data.
 */
inline TPSHUDState build_tps_hud(
        const PlayerController& player,
        const LevelManager& level_mgr,
        const TPSCombatStats& combat_stats,
        const DamageFeedbackSystem& feedback,
        const std::vector<MutantInstance>& enemies,
        float time) {

    TPSHUDState hud;

    // Health bar
    hud.health_bar.x = -0.92f;
    hud.health_bar.y = -0.88f;
    hud.health_bar.width = 0.3f;
    hud.health_bar.height = 0.025f;
    hud.health_bar.fill = player.health_fraction();
    float hf = hud.health_bar.fill;
    if (hf > 0.5f) hud.health_bar.fill_color = math::Vec3(0.2f, 0.9f, 0.3f);
    else if (hf > 0.25f) hud.health_bar.fill_color = math::Vec3(0.9f, 0.8f, 0.2f);
    else hud.health_bar.fill_color = math::Vec3(0.9f, 0.2f, 0.2f);
    hud.health_bar.bg_color = math::Vec3(0.15f, 0.15f, 0.15f);
    hud.health_bar.flash = hf < 0.25f;
    hud.health_bar.flash_intensity = hf < 0.25f
        ? 0.3f + 0.2f * std::sin(time * 6.0f) : 0.0f;

    // Stamina bar
    hud.stamina_bar.x = -0.92f;
    hud.stamina_bar.y = -0.92f;
    hud.stamina_bar.width = 0.2f;
    hud.stamina_bar.height = 0.015f;
    hud.stamina_bar.fill = player.stamina_fraction();
    hud.stamina_bar.fill_color = math::Vec3(0.3f, 0.6f, 0.9f);
    hud.stamina_bar.bg_color = math::Vec3(0.1f, 0.1f, 0.15f);

    // Ammo
    const auto& wpn = player.loadout().current();
    hud.ammo_text.x = 0.65f;
    hud.ammo_text.y = -0.88f;
    hud.ammo_text.text = std::to_string(wpn.ammo()) + " / " + std::to_string(wpn.reserve());
    hud.ammo_text.color = wpn.ammo() > 0
        ? math::Vec3(0.9f, 0.9f, 0.9f) : math::Vec3(0.9f, 0.2f, 0.2f);
    if (wpn.is_reloading()) {
        hud.ammo_text.text = "RELOADING";
        hud.ammo_text.color = math::Vec3(0.9f, 0.7f, 0.2f);
    }

    // Weapon name
    hud.weapon_name.x = 0.65f;
    hud.weapon_name.y = -0.93f;
    hud.weapon_name.text = weapon_name(wpn.config().type);
    hud.weapon_name.color = math::Vec3(0.6f, 0.6f, 0.7f);
    hud.weapon_name.scale = 0.7f;

    // Weapon slots
    hud.weapon_slot_current = player.loadout().current_index();
    hud.weapon_slot_total = player.loadout().weapon_count();

    // Crosshair
    hud.crosshair.spread_amount = wpn.effective_spread() * 10.0f;
    hud.crosshair.spread_indicator = true;

    // Lock-on reticle
    if (player.lock_on().is_locked()) {
        hud.lock_on_reticle.visible = true;
        auto wobble = player.lock_on().get_wobble_offset();
        hud.lock_on_reticle.wobble_x = wobble.x * 0.05f;
        hud.lock_on_reticle.wobble_y = wobble.y * 0.05f;
        hud.lock_on_reticle.pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
    } else {
        hud.lock_on_reticle.visible = false;
    }

    // Score
    hud.score_text.x = 0.6f;
    hud.score_text.y = 0.9f;
    hud.score_text.text = "SCORE: " + std::to_string(combat_stats.score);
    hud.score_text.color = math::Vec3(0.9f, 0.85f, 0.6f);

    // Combo
    hud.combo_text.visible = false;

    // Objective
    const auto& ld = level_mgr.level_data();
    float progress = level_mgr.objective_progress();
    std::string obj_str;
    switch (ld.primary_objective.type) {
        case ObjectiveType::KillAll:
            obj_str = "ELIMINATE ALL TARGETS: " +
                std::to_string(level_mgr.enemies_killed()) + "/" +
                std::to_string(ld.total_enemy_count);
            break;
        case ObjectiveType::Survive:
            obj_str = "SURVIVE: " + std::to_string(static_cast<int>(
                ld.primary_objective.duration - level_mgr.level_timer())) + "s";
            break;
        case ObjectiveType::ReachExit:
            obj_str = "REACH EXTRACTION POINT";
            break;
        case ObjectiveType::DefendPoint:
            obj_str = "DEFEND POSITION: " + std::to_string(static_cast<int>(
                ld.primary_objective.duration - level_mgr.level_timer())) + "s";
            break;
        case ObjectiveType::BossKill:
            obj_str = "DESTROY THE APEX";
            break;
    }
    hud.objective_text.x = -0.45f;
    hud.objective_text.y = 0.9f;
    hud.objective_text.text = obj_str;
    hud.objective_text.color = progress >= 1.0f
        ? math::Vec3(0.3f, 1.0f, 0.3f) : math::Vec3(0.8f, 0.8f, 0.9f);

    // Level name
    hud.level_name.x = -0.92f;
    hud.level_name.y = 0.85f;
    hud.level_name.text = std::to_string(ld.level_number) + ". " + ld.name;
    hud.level_name.color = math::Vec3(0.5f, 0.5f, 0.6f);
    hud.level_name.scale = 0.8f;

    // Boss health bar
    hud.boss_health_bar.visible = false;
    for (const auto& e : enemies) {
        if (!e.alive) continue;
        if (e.config.type == MutantType::Behemoth || e.config.type == MutantType::Apex) {
            hud.boss_health_bar.visible = true;
            hud.boss_health_bar.x = -0.3f;
            hud.boss_health_bar.y = 0.78f;
            hud.boss_health_bar.width = 0.6f;
            hud.boss_health_bar.height = 0.02f;
            hud.boss_health_bar.fill = e.health_fraction();
            hud.boss_health_bar.fill_color = math::Vec3(0.8f, 0.15f, 0.1f);
            hud.boss_health_bar.bg_color = math::Vec3(0.2f, 0.05f, 0.05f);
            break;
        }
    }

    // Enemy health bars for locked target
    hud.enemy_bars.clear();
    if (player.lock_on().is_locked()) {
        int tid = player.lock_on().locked_target_id();
        for (const auto& e : enemies) {
            if (e.id == tid && e.alive) {
                EnemyHealthBarData eb;
                eb.fill = e.health_fraction();
                eb.name = mutant_name(e.config.type);
                eb.is_boss = (e.config.type == MutantType::Behemoth ||
                              e.config.type == MutantType::Apex);
                hud.enemy_bars.push_back(eb);
                break;
            }
        }
    }

    // Feedback overlays
    hud.low_health_vignette = feedback.low_health_pulse();
    if (feedback.has_active_hit_marker()) {
        hud.hit_marker_intensity = feedback.hit_marker().intensity();
        hud.hit_marker_kill = feedback.hit_marker().is_kill;
    } else {
        hud.hit_marker_intensity = 0.0f;
    }

    hud.damage_indicator_angles.clear();
    hud.damage_indicator_intensities.clear();
    for (const auto& di : feedback.damage_indicators()) {
        if (di.active()) {
            hud.damage_indicator_angles.push_back(di.angle);
            hud.damage_indicator_intensities.push_back(di.intensity());
        }
    }

    return hud;
}

} // namespace tps
} // namespace game
} // namespace qe
