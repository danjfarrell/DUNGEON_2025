// src/core/Game.h
// Simplified Game class - initialization delegated to GameBootstrap
#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>

// Only include what we absolutely need in the header
#include "../ecs/Entity.h"
#include "../config/GameConfig.h"
#include "ResourceManager.h"

// Forward declarations (avoid heavy includes)
class World;
class SpriteManager;
class EnemyDataManager;
class DungeonManager;
class EnemySpawner;
class Map;
class TileVisibility;
class MessageLog;
struct UILayout;
class Minimap;
class UnifiedHotbar;
class InventoryPanel;
class HealthBar;
class Camera;
class TurnManager;
class CombatSystem;
class MagicSystem;
class ExperienceSystem;
class StairSystem;
class ConsumableSystem;
class MapRenderSystem;
class RenderSystem;
class InputController;
class HudRenderer;
class LevelTransitionSystem;
class StatusEffectSystem;

// ============================================================================
// Game Class - Main game coordinator (Simplified)
// ============================================================================
// Responsibilities:
// - Run main game loop
// - Coordinate game systems
// - Handle input and rendering
// 
// Initialization delegated to GameBootstrap
// Input handling will be extracted to InputController (Phase 2)
// Rendering will be extracted to HudRenderer (Phase 3)
// ============================================================================

class Game {
public:
    Game();
    ~Game();

    // Initialization (delegates to GameBootstrap)
    bool initialize(unsigned int seed = 0);

    // Main game loop
    void run();

    // True once the player has chosen to play again from the game-over /
    // victory screen. main() checks this after run() returns to decide
    // whether to construct a fresh Game and go again.
    bool should_restart() const { return wants_restart; }

private:
    // ========================================
    // Member Variables
    // ========================================

    // Configuration
    GameConfig config;

    // Resources (RAII - automatic cleanup)
    std::unique_ptr<ResourceManager> resources;

    // Managers
    std::unique_ptr<SpriteManager> sprite_manager;
    std::unique_ptr<EnemyDataManager> enemy_data;
    std::unique_ptr<DungeonManager> dungeon_manager;
    std::unique_ptr<EnemySpawner> enemy_spawner;

    // World and entities
    std::unique_ptr<World> world;
    Entity player = 0;
    Map* current_map = nullptr;

    // Systems (pointers to systems owned by World)
    std::unique_ptr<TurnManager> turn_manager;
    CombatSystem* combat_system = nullptr;
    MagicSystem* magic_system = nullptr;
    ExperienceSystem* xp_system = nullptr;
    StairSystem* stair_system = nullptr;
    ConsumableSystem* consumable_system = nullptr;
    MapRenderSystem* map_render_system = nullptr;
    RenderSystem* render_system = nullptr;

    // Camera
    std::unique_ptr<Camera> camera;

    // UI
    std::unique_ptr<UILayout> ui_layout;
    std::unique_ptr<MessageLog> message_log;
    std::unique_ptr<Minimap> minimap;
    std::unique_ptr<UnifiedHotbar> hotbar;
    std::unique_ptr<InventoryPanel> inventory_panel;
    std::unique_ptr<HealthBar> health_bar;

    // Subsystems
    std::unique_ptr<InputController> input_controller;
    std::unique_ptr<HudRenderer>     hud_renderer;    // Phase 3
    std::unique_ptr<LevelTransitionSystem> level_transition;       // Phase 4
    std::unique_ptr<StatusEffectSystem> status_effect_system;      // Roadmap Phase 2

    // Game state
    bool running = false;
    TileVisibility* tile_vis = nullptr;

    // Whether the run is still in progress, lost, or won. Gates the normal
    // update loop and switches input handling in run() to just restart/quit.
    enum class GameState { PLAYING, GAME_OVER, VICTORY };
    GameState state = GameState::PLAYING;
    bool wants_restart = false;

    // ========================================
    // Game Loop Methods
    // ========================================

    //void handle_event(const SDL_Event& event);
    void update();
    void render();

    // ========================================
    // Input Handling (TODO: Extract to InputController - Phase 2)
    // ========================================

    //void handle_spell_and_item_hotkeys(const SDL_Event& event);
    //void handle_stair_navigation(const SDL_Event& event);
    //void handle_minimap_toggle(const SDL_Event& event);
   //void handle_player_movement(const SDL_Event& event);

    // ========================================
    // Rendering (TODO: Extract to HudRenderer - Phase 3)
    // ========================================

    //void render_ui_backgrounds();
    //void render_ui_elements();

    // ========================================
    // Turn Management
    // ========================================

    //void end_player_turn();
    //bool use_consumable(int slot);
};