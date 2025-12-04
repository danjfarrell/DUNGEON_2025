// src/systems/StairSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../world/Map.h"
#include "../world/DungeonManager.h"
#include "../ui/MessageLog.h"

class StairSystem : public System {
private:
    Map* current_map;
    DungeonManager* dungeon_manager;
    MessageLog* message_log;
    bool stairs_triggered;
    bool descending;

    // ADD THESE:
    bool message_shown;      // Track if we've shown the message
    TileType last_tile;      // Track what tile player was on last frame

public:
    StairSystem(Map* map, DungeonManager* dm, MessageLog* log)
        : current_map(map), dungeon_manager(dm), message_log(log),
        stairs_triggered(false), descending(false),
        message_shown(false), last_tile(TileType::VOID) {  // INITIALIZE NEW MEMBERS
    }

    void update(ComponentManager& components, float dt) override {
        stairs_triggered = false;

        auto* player_tags = components.get_array<PlayerControlled>();
        if (!player_tags || player_tags->size() == 0) return;

        Entity player = player_tags->get_entities()[0];
        Position* player_pos = components.get_component<Position>(player);

        if (!player_pos || !current_map) return;

        TileType tile = current_map->get_tile(player_pos->x, player_pos->y);

        // ========================================
        // FIX: Only show message when first stepping on stairs
        // ========================================
        if (tile == TileType::STAIRS_DOWN) {
            if (!message_shown || last_tile != TileType::STAIRS_DOWN) {
                if (message_log) {
                    message_log->add_info("Press '>' to descend the stairs.");
                }
                message_shown = true;
            }
        }
        else if (tile == TileType::STAIRS_UP) {
            if (!message_shown || last_tile != TileType::STAIRS_UP) {
                if (message_log) {
                    message_log->add_info("Press '<' to ascend the stairs.");
                }
                message_shown = true;
            }
        }
        else {
            // Player moved off stairs, reset message flag
            message_shown = false;
        }

        last_tile = tile;  // Remember current tile
    }

    void trigger_stairs_down() {
        stairs_triggered = true;
        descending = true;
        message_shown = false;  // Reset for next level
    }

    void trigger_stairs_up() {
        stairs_triggered = true;
        descending = false;
        message_shown = false;  // Reset for next level
    }

    bool was_triggered() const { return stairs_triggered; }
    bool is_descending() const { return descending; }

    void set_map(Map* map) {
        current_map = map;
        message_shown = false;  // Reset when changing maps
        last_tile = TileType::VOID;
    }
};