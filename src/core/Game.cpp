// src/core/Game.cpp
// Simplified implementation - initialization delegated to GameBootstrap
// Extracted from original ~900 line God-object

#include "Game.h"
#include "GameBootstrap.h"
#include "../utils/Logger.h"

// ECS and entities
#include "../ecs/World.h"

// Graphics and rendering
#include "../graphics/SpriteManager.h"
#include "../systems/Camera.h"

// World and map
#include "../world/Map.h"
#include "../world/DungeonManager.h"
#include "../world/TileVisibility.h"

// Systems
#include "../systems/TurnManager.h"
#include "../systems/CombatSystem.h"
#include "../systems/MagicSystem.h"
#include "../systems/ExperienceSystem.h"
#include "../systems/StairSystem.h"
#include "../systems/ConsumableSystem.h"
#include "../systems/InputController.h"

// UI
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../ui/HealthBar.h"

// Components
#include "../components/Components.h"

// ============================================================================
// Constructor & Destructor
// ============================================================================

Game::Game()
    : running(false),
    player(0),
    current_map(nullptr),
    combat_system(nullptr),
    magic_system(nullptr),
    xp_system(nullptr),
    stair_system(nullptr),
    consumable_system(nullptr),
    map_render_system(nullptr),
    render_system(nullptr),
    tile_vis(nullptr) {
    // Don't log here - logger not initialized yet!
}

Game::~Game() {
    LOG_INFO("=== Game shutting down ===");
    // RAII handles cleanup automatically via unique_ptrs
    // No manual cleanup needed!
}

// ============================================================================
// Initialization - Delegated to GameBootstrap
// ============================================================================

bool Game::initialize() {
    // Bootstrap handles all initialization phases
    auto result = GameBootstrap::initialize();

    if (!result.success) {
        LOG_ERROR("Game initialization failed: " + result.error_message);
        return false;
    }

    // Move initialized components to Game
    config = std::move(result.config);
    resources = std::move(result.resources);
    sprite_manager = std::move(result.sprite_manager);
    enemy_data = std::move(result.enemy_data);
    dungeon_manager = std::move(result.dungeon_manager);
    enemy_spawner = std::move(result.enemy_spawner);
    world = std::move(result.world);
    player = result.player;
    current_map = result.current_map;
    turn_manager = std::move(result.turn_manager);
    combat_system = result.combat_system;
    magic_system = result.magic_system;
    xp_system = result.xp_system;
    stair_system = result.stair_system;
    consumable_system = result.consumable_system;
    map_render_system = result.map_render_system;
    render_system = result.render_system;
    camera = std::move(result.camera);
    ui_layout = std::move(result.ui_layout);
    message_log = std::move(result.message_log);
    minimap = std::move(result.minimap);
    hotbar = std::move(result.hotbar);
    inventory_panel = std::move(result.inventory_panel);
    health_bar = std::move(result.health_bar);
    tile_vis = result.tile_vis;


    // Create InputController (Phase 2)
    input_controller = std::make_unique<InputController>(
        world.get(),
        current_map,
        turn_manager.get(),
        combat_system,
        magic_system,
        stair_system,
        consumable_system,
        minimap.get(),
        inventory_panel.get(),
        message_log.get(),
        camera.get(),
        tile_vis,
        &config,
        player
    );



    LOG_INFO("=== Game initialization complete (via GameBootstrap) ===");
    return true;
}

// ============================================================================
// Main Game Loop
// ============================================================================

void Game::run() {
    running = true;
    SDL_Event event;

    const float frame_time = 1000.0f / config.gameplay.target_fps;

    LOG_INFO("=== Starting game loop ===");

    while (running) {
        Uint64 frame_start = SDL_GetTicks();

        // Handle input
        while (SDL_PollEvent(&event)) {
            //handle_event(event);
            auto result = input_controller->handle_event(event);
            
            if (result.quit_requested) {
                running = false;
                break;
            }

            if (result.turn_ended) {
                //  ADD THIS
                //LOG_INFO("[INPUT] turn_ended=true, calling end_player_turn()");
                //  END ADD
                turn_manager->end_player_turn();
            }

        }

        // Update game state
        update();

        // Render
        render();

        // Frame rate limiting
        Uint64 frame_end = SDL_GetTicks();
        float elapsed = static_cast<float>(frame_end - frame_start);

        if (elapsed < frame_time) {
            SDL_Delay(static_cast<Uint32>(frame_time - elapsed));
        }
    }

    LOG_INFO("=== Game loop ended ===");
}

// ============================================================================
// Event Handling (TODO: Extract to InputController - Phase 2)
// ============================================================================

//void Game::handle_event(const SDL_Event& event) {
//    if (event.type == SDL_EVENT_QUIT) {
//        running = false;
//        return;
//    }
//
//    if (event.type != SDL_EVENT_KEY_DOWN) {
//        return;  // Only handle key presses
//    }
//
//    // Global hotkeys (work even when inventory is open)
//    if (event.key.key == SDLK_ESCAPE) {
//        if (inventory_panel->is_visible()) {
//            inventory_panel->hide();
//            return;
//        }
//        running = false;
//        return;
//    }
//
//    if (event.key.key == SDLK_I) {
//        inventory_panel->toggle();
//        return;
//    }
//
//    // Skip game input if inventory is open
//    if (inventory_panel->is_visible()) {
//        return;
//    }
//
//    // Only allow player input during player turn
//    if (!turn_manager->is_player_turn()) {
//        return;
//    }
//
//    // Handle different input types
//    handle_spell_and_item_hotkeys(event);
//    handle_stair_navigation(event);
//    handle_minimap_toggle(event);
//    handle_player_movement(event);
//}
//
//void Game::handle_spell_and_item_hotkeys(const SDL_Event& event) {
//    auto& components = world->get_component_manager();
//
//    // Keys 1-9
//    if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9) {
//        int key_num = event.key.key - SDLK_1 + 1;  // 1-9
//
//        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
//            // SHIFT + 1-5: Cast spells
//            if (key_num >= 1 && key_num <= 5) {
//                magic_system->cast_spell(components, player, key_num - 1);
//            }
//        }
//        else {
//            // 6-9: Use items (inventory slots 0-3)
//            if (key_num >= 6 && key_num <= 9) {
//                int item_slot = key_num - 6;
//                if (use_consumable(item_slot)) {
//                    end_player_turn();
//                }
//            }
//        }
//    }
//    // Key 0: Use item from slot 4
//    else if (event.key.key == SDLK_0) {
//        if (use_consumable(4)) {
//            end_player_turn();
//        }
//    }
//}
//
//void Game::handle_stair_navigation(const SDL_Event& event) {
//    Position* pos = world->get_component<Position>(player);
//    if (!pos) return;
//
//    // '>' - Descend stairs
//    if (event.key.key == SDLK_PERIOD && SDL_GetModState() & SDL_KMOD_SHIFT) {
//        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_DOWN) {
//            stair_system->trigger_stairs_down();
//        }
//    }
//    // '<' - Ascend stairs
//    else if (event.key.key == SDLK_COMMA && SDL_GetModState() & SDL_KMOD_SHIFT) {
//        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_UP) {
//            stair_system->trigger_stairs_up();
//        }
//    }
//}
//
//void Game::handle_minimap_toggle(const SDL_Event& event) {
//    if (event.key.key == SDLK_F) {
//        minimap->set_show_fog(!minimap->get_show_fog());
//        message_log->add_info(minimap->get_show_fog() ?
//            "Fog of war ENABLED" : "Fog of war DISABLED");
//    }
//}
//
//void Game::handle_player_movement(const SDL_Event& event) {
//    Position* pos = world->get_component<Position>(player);
//    if (!pos) return;
//
//    int dx = 0, dy = 0;
//
//    // Arrow keys
//    if (event.key.key == SDLK_UP) dy = -1;
//    else if (event.key.key == SDLK_DOWN) dy = 1;
//    else if (event.key.key == SDLK_LEFT) dx = -1;
//    else if (event.key.key == SDLK_RIGHT) dx = 1;
//    // WASD
//    else if (event.key.key == SDLK_W) dy = -1;
//    else if (event.key.key == SDLK_S) dy = 1;
//    else if (event.key.key == SDLK_A) dx = -1;
//    else if (event.key.key == SDLK_D) dx = 1;
//    // Numpad
//    else if (event.key.key == SDLK_KP_8) dy = -1;
//    else if (event.key.key == SDLK_KP_2) dy = 1;
//    else if (event.key.key == SDLK_KP_4) dx = -1;
//    else if (event.key.key == SDLK_KP_6) dx = 1;
//    else if (event.key.key == SDLK_KP_7) { dx = -1; dy = -1; }
//    else if (event.key.key == SDLK_KP_9) { dx = 1; dy = -1; }
//    else if (event.key.key == SDLK_KP_1) { dx = -1; dy = 1; }
//    else if (event.key.key == SDLK_KP_3) { dx = 1; dy = 1; }
//
//    if (dx != 0 || dy != 0) {
//        int new_x = pos->x + dx;
//        int new_y = pos->y + dy;
//
//        if (current_map->is_walkable(new_x, new_y)) {
//            // Check for enemy at target position
//            auto& components = world->get_component_manager();
//            Entity target = Entity(0);
//
//            // Get all entities with Position component
//            auto* positions_array = components.get_array<Position>();
//            if (positions_array) {
//                auto& entities = positions_array->get_entities();
//
//                for (auto entity : entities) {
//                    if (entity == player) continue;
//
//                    Position* enemy_pos = components.get_component<Position>(entity);
//                    Health* health = components.get_component<Health>(entity);
//
//                    if (enemy_pos && health &&
//                        enemy_pos->x == new_x && enemy_pos->y == new_y &&
//                        health->current > 0) {
//                        target = entity;
//                        break;
//                    }
//                }
//            }
//
//            if (target != Entity(0)) {
//                // Attack enemy
//                combat_system->try_attack(components, player, target);
//            }
//            else {
//                // Move player
//                pos->x = new_x;
//                pos->y = new_y;
//                camera->center_on(new_x, new_y);
//                tile_vis->update_fov(new_x, new_y, config.gameplay.player_vision_range);
//                minimap->center_on(new_x, new_y);
//                minimap->update_from_fov(tile_vis);
//            }
//
//            end_player_turn();
//        }
//    }
//}

// ============================================================================
// Update
// ============================================================================

void Game::update() {


    
    if (turn_manager->is_enemy_turn()) {
        //  ADD THIS
        //LOG_INFO("[UPDATE] Processing enemy turn...");
        //  END ADD

        turn_manager->process_turn(*world);
        // ... enemy AI runs ...

        //  ADD THIS
        //LOG_INFO("[UPDATE] Enemy turn complete, calling end_enemy_turn()");
        //  END ADD
        turn_manager->end_enemy_turn();
    }
    
    // Process enemy turns
    //turn_manager->process_turn(*world);

    // Check for player death
    Health* player_health = world->get_component<Health>(player);
    if (player_health && player_health->current <= 0) {
        message_log->add_combat("You have died!");
        message_log->add_info("Press ESC to quit.");
        // Could add game over state here
    }
}

// ============================================================================
// Rendering (TODO: Extract to HudRenderer - Phase 3)
// ============================================================================

void Game::render() {
    auto* renderer = resources->renderer.get();



    // Clear screen
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Render UI backgrounds
    render_ui_backgrounds();


    world->update(0.016f);



    // Render UI elements
    render_ui_elements();

    // Present frame
    SDL_RenderPresent(renderer);
}

void Game::render_ui_backgrounds() {
    auto* renderer = resources->renderer.get();

    // Top bar background
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_FRect top_bar_rect = {
        static_cast<float>(ui_layout->top_bar.x),
        static_cast<float>(ui_layout->top_bar.y),
        static_cast<float>(ui_layout->top_bar.w),
        static_cast<float>(ui_layout->top_bar.h)
    };
    SDL_RenderFillRect(renderer, &top_bar_rect);

    // Top bar border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderRect(renderer, &top_bar_rect);

    // Minimap background
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
    SDL_FRect minimap_rect = {
        static_cast<float>(ui_layout->minimap.x),
        static_cast<float>(ui_layout->minimap.y),
        static_cast<float>(ui_layout->minimap.w),
        static_cast<float>(ui_layout->minimap.h)
    };
    SDL_RenderFillRect(renderer, &minimap_rect);

    // Minimap border
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderRect(renderer, &minimap_rect);
}

void Game::render_ui_elements() {
    // Render minimap
    minimap->render();

    // Render message log
    message_log->render();

    // Render hotbar
    hotbar->render(player);

    // Render health bar
    Health* player_health = world->get_component<Health>(player);
    if (player_health) {
        health_bar->render(player_health->current, player_health->maximum);
    }

    // Render inventory panel if visible
    if (inventory_panel->is_visible()) {
        inventory_panel->render(player);
    }
}

// ============================================================================
// Turn Management
// ============================================================================

void Game::end_player_turn() {
    turn_manager->end_player_turn();
}

bool Game::use_consumable(int slot) {
    auto& components = world->get_component_manager();
    Inventory* inv = components.get_component<Inventory>(player);

    if (!inv || slot >= static_cast<int>(inv->items.size())) {
        return false;
    }

    if (inv->items[slot] == Entity(0)) {
        return false;
    }

    consumable_system->use_item(components, player, inv->items[slot]);
    return true;
}