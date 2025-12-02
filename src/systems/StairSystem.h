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

public:
    StairSystem(Map* map, DungeonManager* dm, MessageLog* log) 
        : current_map(map), dungeon_manager(dm), message_log(log),
          stairs_triggered(false), descending(false) {
    }

    void update(ComponentManager& components, float dt) override {
        stairs_triggered = false;
        
        auto* player_tags = components.get_array<PlayerControlled>();
        if (!player_tags || player_tags->size() == 0) return;
        
        Entity player = player_tags->get_entities()[0];
        Position* player_pos = components.get_component<Position>(player);
        
        if (!player_pos || !current_map) return;
        
        TileType tile = current_map->get_tile(player_pos->x, player_pos->y);
        
        if (tile == TileType::STAIRS_DOWN) {
            if (message_log) {
                message_log->add_info("Press '>' to descend the stairs.");
            }
        } else if (tile == TileType::STAIRS_UP) {
            if (message_log) {
                message_log->add_info("Press '<' to ascend the stairs.");
            }
        }
    }

    void trigger_stairs_down() {
        stairs_triggered = true;
        descending = true;
    }

    void trigger_stairs_up() {
        stairs_triggered = true;
        descending = false;
    }

    bool was_triggered() const { return stairs_triggered; }
    bool is_descending() const { return descending; }

    void set_map(Map* map) { current_map = map; }
};