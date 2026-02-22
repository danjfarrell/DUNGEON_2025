// src/systems/LevelTransitionSystem.h
// Phase 4: Extract level transition coordination from Game::update()
// Responsibilities: Handle all state changes when player uses stairs

#pragma once

#include "../ecs/Entity.h"

// Forward declarations
class World;
class Map;
class DungeonManager;
class EnemySpawner;
class StairSystem;
class MapRenderSystem;
class RenderSystem;
class Camera;
class TileVisibility;
class Minimap;
class MessageLog;
class HudRenderer;
class InputController;
struct GameConfig;

// ============================================================================
// LevelTransitionSystem
// ============================================================================
// Single Responsibility: Coordinate all systems when the player changes level.
//
// When stairs are triggered, this system:
//   1. Saves current level's exploration state
//   2. Generates (or loads cached) the new level
//   3. Repositions the player at the correct stairs
//   4. Rebuilds TileVisibility for the new map size
//   5. Resets FOV from new player position
//   6. Updates all systems that hold a Map* pointer
//   7. Logs the transition
// ============================================================================

class LevelTransitionSystem {
public:
    LevelTransitionSystem(
        World* world,
        DungeonManager* dungeon_manager,
        EnemySpawner* enemy_spawner,
        StairSystem* stair_system,
        MapRenderSystem* map_render_system,
        RenderSystem* render_system,
        Camera* camera,
        Minimap* minimap,
        MessageLog* message_log,
        InputController* input_controller,
        const GameConfig* config,
        Entity player
    );

    // Call once per frame in Game::update().
    // Returns true if a transition occurred (Game should update current_map
    // and tile_vis from get_current_map() / get_tile_vis()).
    bool check_and_execute(Map*& current_map, TileVisibility*& tile_vis);

    // After a transition, Game can retrieve the updated pointers here.
    // (Convenience — same values written into current_map/tile_vis above.)
    Map* get_current_map() const { return current_map; }
    TileVisibility* get_tile_vis()   const { return tile_vis; }

    // Call after HudRenderer is created so transitions can update it too
    void set_hud_renderer(HudRenderer* hud) { hud_renderer = hud; }

private:
    // Dependencies (non-owning)
    World* world;
    DungeonManager* dungeon_manager;
    EnemySpawner* enemy_spawner;
    StairSystem* stair_system;
    MapRenderSystem* map_render_system;
    RenderSystem* render_system;
    Camera* camera;
    Minimap* minimap;
    MessageLog* message_log;
    InputController* input_controller;
    HudRenderer* hud_renderer = nullptr;
    const GameConfig* config;
    Entity           player;

    // Cached pointers kept in sync with Game
    Map* current_map = nullptr;
    TileVisibility* tile_vis = nullptr;

    // Internal helpers
    void execute_transition(bool descending, Map*& out_map, TileVisibility*& out_vis);
    void update_all_map_pointers(Map* new_map);
    void rebuild_tile_visibility(Map* new_map, TileVisibility*& out_vis);
    void reposition_player_fov(Map* new_map, TileVisibility* vis);
};
