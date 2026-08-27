// src/systems/LevelTransitionSystem.cpp
// Phase 4: Level transition coordination extracted from Game::update()

#include "LevelTransitionSystem.h"
#include "../world/DungeonManager.h"
#include "../world/TileVisibility.h"
#include "../world/Map.h"
#include "../ecs/World.h"
#include "../systems/StairSystem.h"
#include "../systems/MapRenderSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/Camera.h"
#include "../systems/InputController.h"
#include "../spawning/EnemySpawner.h"
#include "../ui/Minimap.h"
#include "../ui/MessageLog.h"
#include "../ui/HudRenderer.h"
#include "../config/GameConfig.h"
#include "../components/Components.h"
#include "../utils/Logger.h"

// ============================================================================
// Constructor
// ============================================================================

LevelTransitionSystem::LevelTransitionSystem(
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
)
    : world(world),
    dungeon_manager(dungeon_manager),
    enemy_spawner(enemy_spawner),
    stair_system(stair_system),
    map_render_system(map_render_system),
    render_system(render_system),
    camera(camera),
    minimap(minimap),
    message_log(message_log),
    input_controller(input_controller),
    config(config),
    player(player)
{
}

// ============================================================================
// check_and_execute � call once per frame from Game::update()
// ============================================================================

bool LevelTransitionSystem::check_and_execute(Map*& out_map, TileVisibility*& out_vis) {
    // Sync our cached pointers with whatever Game currently has
    current_map = out_map;
    tile_vis = out_vis;

    if (!stair_system->was_triggered()) {
        return false;
    }

    bool descending = stair_system->is_descending();
    execute_transition(descending, out_map, out_vis);

    // Keep our cache up-to-date too
    current_map = out_map;
    tile_vis = out_vis;

    return true;
}

// ============================================================================
// execute_transition � the full level change sequence
// ============================================================================

void LevelTransitionSystem::execute_transition(bool descending, Map*& out_map, TileVisibility*& out_vis) {
    int old_depth = dungeon_manager->get_current_depth();
    int new_depth = descending ? old_depth + 1 : old_depth - 1;
    new_depth = std::max(1, new_depth);

    LOG_INFO("Level transition: depth " + std::to_string(old_depth) +
        " ? " + std::to_string(new_depth) +
        (descending ? " (descending)" : " (ascending)"));

    // ---- Step 1: Save current exploration state ----
    if (tile_vis) {
        // Clone current TileVisibility so DungeonManager can cache it
        auto saved_vis = std::make_unique<TileVisibility>(*tile_vis);
        dungeon_manager->save_exploration(old_depth, std::move(saved_vis));
        LOG_INFO("Saved exploration state for depth " + std::to_string(old_depth));
    }

    // ---- Step 2: Get player position for stair placement ----
    Position* player_pos = world->get_component<Position>(player);
    if (!player_pos) {
        LOG_ERROR("LevelTransitionSystem: player has no Position component!");
        return;
    }

    // ---- Step 3: Generate (or load) new level ----
    Map* new_map = dungeon_manager->generate_level(new_depth, player_pos, descending);
    if (!new_map) {
        LOG_ERROR("LevelTransitionSystem: failed to generate level " + std::to_string(new_depth));
        return;
    }

    LOG_INFO("New map size: " + std::to_string(new_map->get_width()) +
        "x" + std::to_string(new_map->get_height()));

    // ---- Step 4: Spawn enemies if this is a fresh level ----
    if (!dungeon_manager->has_enemies_spawned(new_depth)) {
        dungeon_manager->spawn_enemies(*enemy_spawner, *world);
        LOG_INFO("Spawned enemies for depth " + std::to_string(new_depth));
    }

    // ---- Step 5: Rebuild TileVisibility for new map ----
    rebuild_tile_visibility(new_map, out_vis);

    // Try to restore saved exploration for this level
    auto restored = dungeon_manager->get_exploration(new_depth);
    if (restored) {
        *out_vis = *restored;
        LOG_INFO("Restored exploration state for depth " + std::to_string(new_depth));
    }

    // ---- Step 6: Update all systems that hold a Map* ----
    out_map = new_map;
    update_all_map_pointers(new_map);

    // ---- Step 7: Recompute FOV from new player position ----
    reposition_player_fov(new_map, out_vis);

    // ---- Step 8: Announce the transition ----
    if (descending) {
        message_log->add_info("You descend to depth " + std::to_string(new_depth) + ".");
    }
    else {
        message_log->add_info("You ascend to depth " + std::to_string(new_depth) + ".");
    }

    LOG_INFO("Level transition complete ? depth " + std::to_string(new_depth));
}

// ============================================================================
// update_all_map_pointers � notify every system that holds a Map*
// ============================================================================

void LevelTransitionSystem::update_all_map_pointers(Map* new_map) {
    if (map_render_system) map_render_system->set_map(new_map);
    if (render_system)     render_system->set_tile_visibility(tile_vis);
    if (stair_system)      stair_system->set_map(new_map);
    if (minimap)           minimap->set_map(new_map);
    if (input_controller)  input_controller->set_map(new_map);
    if (hud_renderer)      hud_renderer->set_player(player); // refreshes entity ref

    LOG_INFO("All system map pointers updated");
}

// ============================================================================
// rebuild_tile_visibility � create fresh visibility for new map dimensions
// ============================================================================

void LevelTransitionSystem::rebuild_tile_visibility(Map* new_map, TileVisibility*& out_vis) {
    // TileVisibility is owned by World via std::unique_ptr<TileVisibility>.
    // Hand World the new instance through set_tile_visibility() so its unique_ptr
    // destroys the old one (safely, exactly once) as part of the assignment, then
    // take a fresh non-owning view for Game/other systems to hold.
    // (Previously this did `delete out_vis; out_vis = new TileVisibility(...)`
    // directly on the raw alias, which left World's unique_ptr dangling at the
    // freed old object — a double free later in Game::~Game() / World::~World().)
    world->set_tile_visibility(std::make_unique<TileVisibility>(new_map->get_width(), new_map->get_height()));
    out_vis = world->get_tile_visibility();

    // Also update render systems with new visibility pointer
    if (map_render_system) map_render_system->set_tile_visibility(out_vis);
    if (render_system)     render_system->set_tile_visibility(out_vis);

    LOG_INFO("TileVisibility rebuilt: " +
        std::to_string(new_map->get_width()) + "x" +
        std::to_string(new_map->get_height()));
}

// ============================================================================
// reposition_player_fov � update camera and compute initial FOV
// ============================================================================

void LevelTransitionSystem::reposition_player_fov(Map* new_map, TileVisibility* vis) {
    Position* pos = world->get_component<Position>(player);
    if (!pos || !vis) return;

    // Center camera on new player position
    if (camera) {
        camera->center_on(pos->x, pos->y);
    }

    // Compute FOV
    vis->update_fov(pos->x, pos->y, config->gameplay.player_vision_range);

    // Sync minimap
    if (minimap) {
        minimap->center_on(pos->x, pos->y);
        minimap->update_from_fov(vis);
    }

    LOG_INFO("FOV updated at (" + std::to_string(pos->x) + ", " +
        std::to_string(pos->y) + ")");
}