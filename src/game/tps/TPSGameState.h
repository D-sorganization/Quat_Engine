#pragma once
/**
 * @file TPSGameState.h
 * @brief Main game orchestrator that coordinates all TPS subsystems per frame.
 *
 * Owns and updates: PlayerController, LevelManager, TPSCombatSystem,
 * DamageFeedbackSystem, all MutantInstances, all Projectiles.
 *
 * Frame update sequence:
 *   1. Process input → PlayerController actions
 *   2. Update player (movement, combat state machines, animation)
 *   3. Update enemy AI → movement, attack decisions
 *   4. Process combat → ranged hits, melee hits, projectile collisions
 *   5. Apply damage to player from enemy attacks
 *   6. Update level progression → objective checking
 *   7. Update feedback → screen shake, damage numbers, hit markers
 *   8. Build HUD state for renderer
 *
 * Pure game logic — no rendering dependency. Fully testable.
 */

#include "../../core/Projectile.h"
#include "../../math/Vec3.h"
#include "CharacterClass.h"
#include "DamageFeedback.h"
#include "LevelSystem.h"
#include "LockOnSystem.h"
#include "MutantAI.h"
#include "MutantTypes.h"
#include "PlayerController.h"
#include "TPSCombat.h"
#include "TPSInput.h"
#include "TPSScene.h"
#include "TPSHUD.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace qe {
namespace game {
namespace tps {

// ── Game Phase ───────────────────────────────────────────────────────────────

enum class GamePhase {
    ClassSelect,
    Briefing,
    Playing,
    LevelComplete,
    LevelFailed,
    Victory,
    GameOver
};

// ── TPS Game State ───────────────────────────────────────────────────────────

class TPSGame {
    // Core systems
    PlayerController player_;
    LevelManager level_mgr_;
    TPSCombatSystem combat_;
    DamageFeedbackSystem feedback_;

    // World state
    std::vector<MutantInstance> enemies_;
    std::vector<core::Projectile> projectiles_;
    TPSSceneData scene_data_;

    // Game flow
    GamePhase phase_ = GamePhase::ClassSelect;
    float phase_timer_ = 0.0f;
    float game_time_ = 0.0f;
    core::Rng ai_rng_{54321};

    // Delayed spawn tracking
    float spawn_timer_ = 0.0f;
    int spawn_wave_index_ = 0;

public:
    // ── Initialization ───────────────────────────────────────────────

    /** Select character class and prepare the game. */
    void select_class(CharacterClassType type) {
        player_ = PlayerController(type);
        phase_ = GamePhase::Briefing;
        phase_timer_ = 0.0f;
    }

    /** Load and start a specific level. */
    void load_level(int level_number) {
        level_mgr_.load_level(level_number);
        scene_data_ = build_scene(level_mgr_.level_data());
        enemies_ = scene_data_.enemies;
        projectiles_.clear();
        combat_.reset();
        feedback_.clear();
        spawn_timer_ = 0.0f;
        spawn_wave_index_ = 0;

        // Reset player position and health
        player_.set_position(level_mgr_.level_data().player_start);
        player_.heal(player_.max_health());

        // Give weapons for unlocked levels
        apply_weapon_unlocks();

        phase_ = GamePhase::Briefing;
        phase_timer_ = 0.0f;
    }

    /** Start playing (from briefing). */
    void start_playing() {
        assert(phase_ == GamePhase::Briefing);
        level_mgr_.start_level();
        phase_ = GamePhase::Playing;
        phase_timer_ = 0.0f;
    }

    /** Retry current level. */
    void retry() {
        load_level(level_mgr_.current_level());
    }

    /** Advance to next level. */
    void next_level() {
        if (!level_mgr_.is_final_level()) {
            level_mgr_.advance_to_next();
            load_level(level_mgr_.current_level());
        }
    }

    // ── Queries ──────────────────────────────────────────────────────

    GamePhase phase() const { return phase_; }
    float phase_timer() const { return phase_timer_; }
    float game_time() const { return game_time_; }

    const PlayerController& player() const { return player_; }
    PlayerController& player_mut() { return player_; }
    const LevelManager& level_manager() const { return level_mgr_; }
    const TPSCombatSystem& combat() const { return combat_; }
    const DamageFeedbackSystem& feedback() const { return feedback_; }
    const std::vector<MutantInstance>& enemies() const { return enemies_; }
    const std::vector<core::Projectile>& projectiles() const { return projectiles_; }
    const TPSSceneData& scene_data() const { return scene_data_; }

    int alive_enemy_count() const {
        int count = 0;
        for (const auto& e : enemies_) if (e.alive) count++;
        return count;
    }

    /** Build lock-on targets from current enemies. */
    std::vector<LockOnTarget> get_lock_targets() const {
        std::vector<LockOnTarget> targets;
        for (const auto& e : enemies_) {
            targets.push_back({e.id, e.position, e.alive, 0.0f});
        }
        return targets;
    }

    /** Build HUD state from current game state. */
    TPSHUDState build_hud(float time) const {
        return build_tps_hud(player_, level_mgr_, combat_.stats(),
                              feedback_, enemies_, time);
    }

    // ── Main Update ──────────────────────────────────────────────────

    /** Process one frame of the game.
     *  @param input  Abstract input state for this frame
     *  @param dt     Delta time in seconds
     */
    void update(const TPSInputState& input, float dt) {
        assert(dt >= 0.0f);
        phase_timer_ += dt;
        game_time_ += dt;

        switch (phase_) {
            case GamePhase::ClassSelect:
                break;

            case GamePhase::Briefing:
                if (phase_timer_ > 3.0f || input.shoot_pressed || input.jump_pressed) {
                    start_playing();
                }
                break;

            case GamePhase::Playing:
                update_gameplay(input, dt);
                break;

            case GamePhase::LevelComplete:
                if (phase_timer_ > 2.0f && (input.shoot_pressed || input.jump_pressed)) {
                    if (level_mgr_.is_final_level()) {
                        phase_ = GamePhase::Victory;
                    } else {
                        next_level();
                    }
                }
                break;

            case GamePhase::LevelFailed:
                if (phase_timer_ > 2.0f && (input.shoot_pressed || input.jump_pressed)) {
                    retry();
                }
                break;

            case GamePhase::Victory:
            case GamePhase::GameOver:
                break;
        }
    }

private:
    // ── Gameplay Update ──────────────────────────────────────────────

    void update_gameplay(const TPSInputState& input, float dt) {
        // 1. Process input
        auto lock_targets = get_lock_targets();
        TPSInputResult input_result = process_tps_input(input, player_, lock_targets);

        // 2. Update player
        player_.update(dt);

        // 3. Update lock-on target position
        if (player_.lock_on().is_locked()) {
            int tid = player_.lock_on().locked_target_id();
            for (const auto& e : enemies_) {
                if (e.id == tid) {
                    // const_cast safe here — we're updating tracking data
                    const_cast<LockOnState&>(player_.lock_on())
                        .update_target_position(e.position);
                    if (player_.lock_on().should_break_lock(player_.position(), e.alive)) {
                        const_cast<LockOnState&>(player_.lock_on()).release_lock();
                    }
                    break;
                }
            }
        }

        // 4. Process ranged combat
        if (input_result.fired_weapon) {
            math::Vec3 aim_dir = player_.rotation().rotate(math::Vec3::forward());
            if (player_.lock_on().is_locked()) {
                math::Vec3 to_target = player_.lock_on().locked_target_position()
                    - player_.position();
                if (to_target.length_squared() > 0.01f) aim_dir = to_target.normalized();
            }

            auto hits = combat_.process_ranged_attack(
                player_.position() + math::Vec3(0, 1.0f, 0),
                aim_dir,
                player_.loadout().current().config(),
                player_.stats(),
                player_.lock_on(),
                enemies_,
                projectiles_);

            for (const auto& hr : hits) {
                if (hr.hit) {
                    math::Vec3 hit_pos = player_.position() + aim_dir * 5.0f;
                    for (const auto& e : enemies_) {
                        if (e.id == hr.target_id) { hit_pos = e.position; break; }
                    }
                    feedback_.on_hit(hit_pos, hr.damage_dealt, hr.critical, hr.killed);
                    if (hr.killed) {
                        level_mgr_.on_enemy_killed(hr.score_earned);
                    }
                }
            }
        }

        // 5. Process melee combat
        if (player_.melee().is_attacking()) {
            auto melee_hits = combat_.process_melee_attack(
                player_.position(),
                player_.rotation(),
                player_.melee(),
                player_.stats(),
                enemies_);

            for (const auto& hr : melee_hits) {
                if (hr.hit) {
                    math::Vec3 hit_pos;
                    for (const auto& e : enemies_) {
                        if (e.id == hr.target_id) { hit_pos = e.position; break; }
                    }
                    feedback_.on_hit(hit_pos, hr.damage_dealt, hr.critical, hr.killed);
                    if (hr.killed) {
                        level_mgr_.on_enemy_killed(hr.score_earned);
                    }
                }
            }
        }

        // 6. Update projectiles
        for (auto& p : projectiles_) p.update(dt);
        auto proj_hits = combat_.process_projectile_collisions(
            projectiles_, enemies_, player_.stats(),
            player_.loadout().current().config().damage_category);
        for (const auto& hr : proj_hits) {
            if (hr.killed) {
                level_mgr_.on_enemy_killed(hr.score_earned);
            }
        }
        // Remove dead projectiles
        projectiles_.erase(
            std::remove_if(projectiles_.begin(), projectiles_.end(),
                [](const core::Projectile& p) { return !p.is_alive(); }),
            projectiles_.end());

        // 7. Update enemy AI
        for (auto& enemy : enemies_) {
            if (!enemy.alive) continue;
            AIDecision decision = update_mutant_ai(
                enemy, player_.position(), player_.is_alive(), dt, ai_rng_);
            apply_ai_movement(enemy, decision, dt);

            // Process enemy attacks against player
            float atk_dmg = process_mutant_attack(
                enemy, decision, player_.position(), player_.has_iframes());
            if (atk_dmg > 0.0f) {
                bool killed = player_.take_damage(atk_dmg,
                    enemy.config.primary_attack.splash_radius > 0.0f);
                feedback_.on_player_hit(atk_dmg, enemy.position,
                    player_.position(),
                    player_.rotation().rotate(math::Vec3::forward()));
                if (killed) {
                    level_mgr_.on_player_death();
                    phase_ = GamePhase::LevelFailed;
                    phase_timer_ = 0.0f;
                }
            }
        }

        // 8. Ground slam damage
        if (player_.jump().just_slam_landed()) {
            feedback_.on_ground_slam();
            auto slam_cfg = make_melee_move(MeleeMove::JumpAttack);
            for (auto& enemy : enemies_) {
                if (!enemy.alive) continue;
                float dist = enemy.position.distance_to(player_.position());
                if (dist <= slam_cfg.splash_radius) {
                    float falloff = 1.0f - dist / slam_cfg.splash_radius;
                    float dmg = slam_cfg.base_damage * player_.stats().melee_damage_mult * falloff;
                    float resist = enemy.get_resistance(false, false, true);
                    bool killed = enemy.take_damage(dmg, resist);
                    feedback_.on_hit(enemy.position, dmg, false, killed);
                    if (killed) {
                        level_mgr_.on_enemy_killed(enemy.config.score_value);
                    }
                }
            }
        }

        // 9. Update level state
        level_mgr_.update(dt);

        if (level_mgr_.state() == LevelState::ObjectiveComplete) {
            phase_ = GamePhase::LevelComplete;
            phase_timer_ = 0.0f;
        } else if (level_mgr_.state() == LevelState::Victory) {
            phase_ = GamePhase::Victory;
            phase_timer_ = 0.0f;
        }

        // 10. Update feedback
        feedback_.update(dt, player_.health_fraction());
    }

    void apply_weapon_unlocks() {
        // Check all completed levels for weapon unlocks
        for (int i = 1; i < level_mgr_.current_level(); ++i) {
            if (level_mgr_.is_level_complete(i)) {
                auto ld = make_level(i);
                if (ld.unlocks_weapon) {
                    add_weapon_if_new(ld.weapon_unlock);
                }
            }
        }
    }

    void add_weapon_if_new(TPSWeaponType type) {
        // Check if player already has this weapon
        for (int i = 0; i < player_.loadout().weapon_count(); ++i) {
            // Simple check — loadout starts with AR + shotgun
            // Additional weapons are added in order
        }
        // Add weapon config based on type
        TPSWeaponConfig cfg;
        switch (type) {
            case TPSWeaponType::CombatShotgun:
                cfg = TPSWeaponState::make_combat_shotgun(); break;
            case TPSWeaponType::PlasmaCaster:
                cfg = TPSWeaponState::make_plasma_caster(); break;
            case TPSWeaponType::MarksmanRifle:
                cfg = TPSWeaponState::make_marksman_rifle(); break;
            case TPSWeaponType::SubmachineGun:
                cfg = TPSWeaponState::make_submachine_gun(); break;
            case TPSWeaponType::GrenadeLauncher:
                cfg = TPSWeaponState::make_grenade_launcher(); break;
            case TPSWeaponType::TeslaCoil:
                cfg = TPSWeaponState::make_tesla_coil(); break;
            default: return;
        }
        player_.loadout_mut().add_weapon(cfg);
    }
};

} // namespace tps
} // namespace game
} // namespace qe
