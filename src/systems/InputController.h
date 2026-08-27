// src/systems/InputController.h
// Phase 2: Extract input handling from Game class
// Responsibilities: Map keyboard input to game actions

#pragma once

#include <SDL3/SDL.h>
#include "../ecs/Entity.h"

// Forward declarations
class World;
class Map;
class TurnManager;
class CombatSystem;
class MagicSystem;
class StairSystem;
class ConsumableSystem;
class Minimap;
class InventoryPanel;
class MessageLog;
class Camera;
class TileVisibility;
struct GameConfig;

// ============================================================================
// InputController - Handles all keyboard input
// ============================================================================
// Single Responsibility: Convert SDL events to game actions
// Extracted from Game class to reduce God-object complexity
// ============================================================================

class InputController {
public:
    // Result of input processing
    struct InputResult {
        bool quit_requested = false;
        bool turn_ended = false;  // Did action consume player's turn?
    };

    InputController(
        World* world,
        Map* current_map,
        TurnManager* turn_manager,
        CombatSystem* combat_system,
        MagicSystem* magic_system,
        StairSystem* stair_system,
        ConsumableSystem* consumable_system,
        Minimap* minimap,
        InventoryPanel* inventory_panel,
        MessageLog* message_log,
        Camera* camera,
        TileVisibility* tile_vis,
        const GameConfig* config,
        Entity player
    );

    // Main input processing
    InputResult handle_event(const SDL_Event& event);

    // Update player reference (when changing levels, etc)
    void set_player(Entity new_player) { player = new_player; }
    void set_map(Map* new_map) { current_map = new_map; }

private:
    // Dependencies
    World* world;
    Map* current_map;
    TurnManager* turn_manager;
    CombatSystem* combat_system;
    MagicSystem* magic_system;
    StairSystem* stair_system;
    ConsumableSystem* consumable_system;
    Minimap* minimap;
    InventoryPanel* inventory_panel;
    MessageLog* message_log;
    Camera* camera;
    TileVisibility* tile_vis;
    const GameConfig* config;
    Entity player;

    // Input handlers
    void handle_spell_and_item_hotkeys(const SDL_Event& event, InputResult& result);
    void handle_stair_navigation(const SDL_Event& event);
    void handle_minimap_toggle(const SDL_Event& event);
    void handle_player_movement(const SDL_Event& event, InputResult& result);

    // Arrow keys / Enter while the inventory panel is open (selection + use/equip)
    void handle_inventory_input(const SDL_Event& event, InputResult& result);
    void use_or_equip_selected(InputResult& result);

    // Helper
    bool use_consumable(int slot);
};
