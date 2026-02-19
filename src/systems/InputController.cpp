// src/systems/InputController.cpp
// Implementation of InputController

#include "InputController.h"
#include "../ecs/World.h"
#include "../world/Map.h"
#include "../systems/TurnManager.h"
#include "../systems/CombatSystem.h"
#include "../systems/MagicSystem.h"
#include "../systems/StairSystem.h"
#include "../systems/ConsumableSystem.h"
#include "../systems/Camera.h"
#include "../world/TileVisibility.h"
#include "../ui/Minimap.h"
#include "../ui/InventoryPanel.h"
#include "../ui/MessageLog.h"
#include "../config/GameConfig.h"
#include "../components/Components.h"

InputController::InputController(
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
)
    : world(world),
    current_map(current_map),
    turn_manager(turn_manager),
    combat_system(combat_system),
    magic_system(magic_system),
    stair_system(stair_system),
    consumable_system(consumable_system),
    minimap(minimap),
    inventory_panel(inventory_panel),
    message_log(message_log),
    camera(camera),
    tile_vis(tile_vis),
    config(config),
    player(player)
{
}

InputController::InputResult InputController::handle_event(const SDL_Event& event) {
    InputResult result;

    if (event.type == SDL_EVENT_QUIT) {
        result.quit_requested = true;
        return result;
    }

    if (event.type != SDL_EVENT_KEY_DOWN) {
        return result;  // Only handle key presses
    }

    // Global hotkeys (work even when inventory is open)
    if (event.key.key == SDLK_ESCAPE) {
        if (inventory_panel->is_visible()) {
            inventory_panel->hide();
            return result;
        }
        result.quit_requested = true;
        return result;
    }

    if (event.key.key == SDLK_I) {
        inventory_panel->toggle();
        return result;
    }

    // Skip game input if inventory is open
    if (inventory_panel->is_visible()) {
        return result;
    }

    // Only allow player input during player turn
    if (!turn_manager->is_player_turn()) {
        return result;
    }

    // Handle different input types
    handle_spell_and_item_hotkeys(event, result);
    handle_stair_navigation(event);
    handle_minimap_toggle(event);
    handle_player_movement(event, result);

    return result;
}

void InputController::handle_spell_and_item_hotkeys(const SDL_Event& event, InputResult& result) {
    auto& components = world->get_component_manager();

    // Keys 1-9
    if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9) {
        int key_num = event.key.key - SDLK_1 + 1;  // 1-9

        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            // SHIFT + 1-5: Cast spells
            if (key_num >= 1 && key_num <= 5) {
                magic_system->cast_spell(components, player, key_num - 1);
            }
        }
        else {
            // 6-9: Use items (inventory slots 0-3)
            if (key_num >= 6 && key_num <= 9) {
                int item_slot = key_num - 6;
                if (use_consumable(item_slot)) {
                    result.turn_ended = true;
                }
            }
        }
    }
    // Key 0: Use item from slot 4
    else if (event.key.key == SDLK_0) {
        if (use_consumable(4)) {
            result.turn_ended = true;
        }
    }
}

void InputController::handle_stair_navigation(const SDL_Event& event) {
    Position* pos = world->get_component<Position>(player);
    if (!pos) return;

    // '>' - Descend stairs
    if (event.key.key == SDLK_PERIOD && SDL_GetModState() & SDL_KMOD_SHIFT) {
        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_DOWN) {
            stair_system->trigger_stairs_down();
        }
    }
    // '<' - Ascend stairs
    else if (event.key.key == SDLK_COMMA && SDL_GetModState() & SDL_KMOD_SHIFT) {
        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_UP) {
            stair_system->trigger_stairs_up();
        }
    }
}

void InputController::handle_minimap_toggle(const SDL_Event& event) {
    if (event.key.key == SDLK_F) {
        minimap->set_show_fog(!minimap->get_show_fog());
        message_log->add_info(minimap->get_show_fog() ?
            "Fog of war ENABLED" : "Fog of war DISABLED");
    }
}

void InputController::handle_player_movement(const SDL_Event& event, InputResult& result) {
    Position* pos = world->get_component<Position>(player);
    if (!pos) return;

    int dx = 0, dy = 0;

    // Arrow keys
    if (event.key.key == SDLK_UP) dy = -1;
    else if (event.key.key == SDLK_DOWN) dy = 1;
    else if (event.key.key == SDLK_LEFT) dx = -1;
    else if (event.key.key == SDLK_RIGHT) dx = 1;
    // WASD
    else if (event.key.key == SDLK_W) dy = -1;
    else if (event.key.key == SDLK_S) dy = 1;
    else if (event.key.key == SDLK_A) dx = -1;
    else if (event.key.key == SDLK_D) dx = 1;
    // Numpad
    else if (event.key.key == SDLK_KP_8) dy = -1;
    else if (event.key.key == SDLK_KP_2) dy = 1;
    else if (event.key.key == SDLK_KP_4) dx = -1;
    else if (event.key.key == SDLK_KP_6) dx = 1;
    else if (event.key.key == SDLK_KP_7) { dx = -1; dy = -1; }
    else if (event.key.key == SDLK_KP_9) { dx = 1; dy = -1; }
    else if (event.key.key == SDLK_KP_1) { dx = -1; dy = 1; }
    else if (event.key.key == SDLK_KP_3) { dx = 1; dy = 1; }

    if (dx != 0 || dy != 0) {
        int new_x = pos->x + dx;
        int new_y = pos->y + dy;

        if (current_map->is_walkable(new_x, new_y)) {
            // Check for enemy at target position
            auto& components = world->get_component_manager();
            Entity target = Entity(0);

            // Get all entities with Position component
            auto* positions_array = components.get_array<Position>();
            if (positions_array) {
                auto& entities = positions_array->get_entities();

                for (auto entity : entities) {
                    if (entity == player) continue;

                    Position* enemy_pos = components.get_component<Position>(entity);
                    Health* health = components.get_component<Health>(entity);

                    if (enemy_pos && health &&
                        enemy_pos->x == new_x && enemy_pos->y == new_y &&
                        health->current > 0) {
                        target = entity;
                        break;
                    }
                }
            }

            if (target != Entity(0)) {
                // Attack enemy
                combat_system->try_attack(components, player, target);
            }
            else {
                // Move player
                pos->x = new_x;
                pos->y = new_y;
                camera->center_on(new_x, new_y);
                tile_vis->update_fov(new_x, new_y, config->gameplay.player_vision_range);
                minimap->center_on(new_x, new_y);
                minimap->update_from_fov(tile_vis);
            }

            result.turn_ended = true;
        }
    }
}

bool InputController::use_consumable(int slot) {
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