// src/world/DungeonManager.h
#pragma once

#include "Map.h"
#include "MapGenerators.h"
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../spawning/EnemySpawner.h"
#include "../graphics/SpriteManager.h"
#include <memory>
#include <random>

class DungeonManager {
private:
    int current_depth;
    std::unique_ptr<Map> current_map;
    std::mt19937 rng;
    int base_map_width;
    int base_map_height;

public:
    DungeonManager(int base_width = 80, int base_height = 50) 
        : current_depth(1), 
          base_map_width(base_width), 
          base_map_height(base_height),
          rng(std::random_device{}()) {
    }

    Map* generate_level(int depth) {
        current_depth = depth;
        
        int width = base_map_width + (depth - 1) * 10;
        int height = base_map_height + (depth - 1) * 5;
        
        width = std::min(width, 120);
        height = std::min(height, 80);
        
        current_map = std::make_unique<Map>(width, height, rng());
        
        std::unique_ptr<MapGenerator> generator = choose_generator(depth);
        current_map->generate(*generator);
        
        place_stairs();
        
        return current_map.get();
    }

    Map* get_current_map() {
        return current_map.get();
    }

    int get_current_depth() const {
        return current_depth;
    }

    Position get_player_spawn_position() {
        if (current_map->get_rooms().empty()) {
            return Position{5, 5};
        }
        
        if (current_depth > 1) {
            for (int y = 0; y < current_map->get_height(); y++) {
                for (int x = 0; x < current_map->get_width(); x++) {
                    if (current_map->get_tile(x, y) == TileType::STAIRS_UP) {
                        return Position{x, y};
                    }
                }
            }
        }
        
        const Room& first_room = current_map->get_rooms()[0];
        return Position{first_room.center_x(), first_room.center_y()};
    }

    void spawn_enemies(EnemySpawner& spawner, World& world) {
        const auto& rooms = current_map->get_rooms();
        
        if (rooms.empty()) return;
        
        int num_enemies = 3 + (current_depth - 1) * 2;
        num_enemies = std::min(num_enemies, static_cast<int>(rooms.size()) - 1);
        
        std::vector<std::string> enemy_pool = get_enemy_pool_for_depth(current_depth);
        
        std::uniform_int_distribution<int> room_dist(1, rooms.size() - 1);
        
        for (int i = 0; i < num_enemies; i++) {
            const Room& room = rooms[room_dist(rng)];
            
            std::uniform_int_distribution<int> enemy_dist(0, enemy_pool.size() - 1);
            std::string enemy_type = enemy_pool[enemy_dist(rng)];
            
            Entity enemy = spawner.spawn(enemy_type, room.center_x(), room.center_y());
            
            scale_enemy_for_depth(world, enemy, current_depth);
        }
    }

private:
    std::unique_ptr<MapGenerator> choose_generator(int depth) {
        if (depth % 3 == 1) {
            return std::make_unique<RoomCorridorGenerator>(10 + depth, 5, 12);
        } else if (depth % 3 == 2) {
            return std::make_unique<LarnMazeGenerator>(true, 2 + depth / 2);
        } else {
            return std::make_unique<CellularAutomataGenerator>(0.45f, 4);
        }
    }

    void place_stairs() {
        const auto& rooms = current_map->get_rooms();
        if (rooms.empty()) return;
        
        if (current_depth > 1) {
            const Room& first_room = rooms[0];
            current_map->set_tile(first_room.center_x(), first_room.center_y(), 
                                 TileType::STAIRS_UP);
        }
        
        const Room& last_room = rooms[rooms.size() - 1];
        current_map->set_tile(last_room.center_x(), last_room.center_y(), 
                             TileType::STAIRS_DOWN);
    }

    std::vector<std::string> get_enemy_pool_for_depth(int depth) {
        std::vector<std::string> pool;
        
        if (depth <= 2) {
            pool = {"rat", "goblin"};
        } else if (depth <= 5) {
            pool = {"goblin", "goblin", "orc"};
        } else {
            pool = {"orc", "goblin", "orc"};
        }
        
        return pool;
    }

    void scale_enemy_for_depth(World& world, Entity enemy, int depth) {
        if (depth == 1) return;
        
        CombatStats* stats = world.get_component<CombatStats>(enemy);
        if (!stats) return;
        
        float scale = 1.0f + (depth - 1) * 0.15f;
        
        stats->max_hp = static_cast<int>(stats->max_hp * scale);
        stats->current_hp = stats->max_hp;
        stats->attack = static_cast<int>(stats->attack * scale);
        stats->defense = static_cast<int>(stats->defense * scale);
    }
};