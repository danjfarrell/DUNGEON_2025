// src/world/DungeonManager.h
#pragma once

#include "Map.h"
#include "MapGenerators.h"
#include "LevelCache.h"  // ADD THIS
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../spawning/EnemySpawner.h"
#include "../graphics/SpriteManager.h"
#include <memory>
#include <random>

class DungeonManager {
private:
    int current_depth;
    Map* current_map;  // Raw pointer to current level
    LevelCache level_cache;  // ADD THIS
    std::mt19937 rng;
    int base_map_width;
    int base_map_height;

public:
    DungeonManager(int base_width = 80, int base_height = 50)
        : current_depth(1),
        current_map(nullptr),
        base_map_width(base_width),
        base_map_height(base_height),
        rng(std::random_device{}()) {
    }

    bool has_level(int depth) const {
        return level_cache.has_level(depth);
    }

    // Generate or retrieve level
    Map* generate_level(int depth, Position* player_exit_pos = nullptr) {
        current_depth = depth;

        // Check if level is cached
        LevelData* cached = level_cache.get_level(depth);
        if (cached) {
            current_map = cached->map.get();

            // Update player_exit_pos if provided
            if (player_exit_pos) {
                player_exit_pos->x = cached->entrance_x;
                player_exit_pos->y = cached->entrance_y;
            }

            return current_map;
        }

        // Generate new level
        int width = base_map_width + (depth - 1) * 10;
        int height = base_map_height + (depth - 1) * 5;

        width = std::min(width, 120);
        height = std::min(height, 80);

        auto new_map = std::make_unique<Map>(width, height, rng());

        std::unique_ptr<MapGenerator> generator = choose_generator(depth);
        new_map->generate(*generator);

        place_stairs(*new_map, depth);

        // Find entrance position (where stairs are)
        int entrance_x = 5;
        int entrance_y = 5;

        if (depth > 1) {
            // Find up-stairs position
            for (int y = 0; y < new_map->get_height(); y++) {
                for (int x = 0; x < new_map->get_width(); x++) {
                    if (new_map->get_tile(x, y) == TileType::STAIRS_UP) {
                        entrance_x = x;
                        entrance_y = y;
                        break;
                    }
                }
            }
        }
        else {
            // First level, use first room center
            if (!new_map->get_rooms().empty()) {
                const Room& first_room = new_map->get_rooms()[0];
                entrance_x = first_room.center_x();
                entrance_y = first_room.center_y();
            }
        }

        current_map = new_map.get();

        // Cache the level
        level_cache.cache_level(depth, std::move(new_map), entrance_x, entrance_y);

        // Update player position if provided
        if (player_exit_pos) {
            player_exit_pos->x = entrance_x;
            player_exit_pos->y = entrance_y;
        }

        return current_map;
    }

    Map* get_current_map() {
        return current_map;
    }

    int get_current_depth() const {
        return current_depth;
    }

    Position get_player_spawn_position() {
        LevelData* cached = level_cache.get_level(current_depth);
        if (cached) {
            return Position{ cached->entrance_x, cached->entrance_y };
        }

        // Fallback if not cached
        if (current_map && !current_map->get_rooms().empty()) {
            const Room& first_room = current_map->get_rooms()[0];
            return Position{ first_room.center_x(), first_room.center_y() };
        }

        return Position{ 5, 5 };
    }

    void spawn_enemies(EnemySpawner& spawner, World& world) {
        // Only spawn if this is a newly generated level
        if (!level_cache.has_level(current_depth)) {
            return;  // Level was cached, enemies already exist
        }

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
        }
        else if (depth % 3 == 2) {
            return std::make_unique<LarnMazeGenerator>(true, 2 + depth / 2);
        }
        else {
            return std::make_unique<CellularAutomataGenerator>(0.45f, 4);
        }
    }

    void place_stairs(Map& map, int depth) {
        const auto& rooms = map.get_rooms();
        if (rooms.empty()) return;

        if (depth > 1) {
            const Room& first_room = rooms[0];
            map.set_tile(first_room.center_x(), first_room.center_y(),
                TileType::STAIRS_UP);
        }

        const Room& last_room = rooms[rooms.size() - 1];
        map.set_tile(last_room.center_x(), last_room.center_y(),
            TileType::STAIRS_DOWN);
    }

    std::vector<std::string> get_enemy_pool_for_depth(int depth) {
        std::vector<std::string> pool;

        if (depth <= 2) {
            pool = { "rat", "goblin" };
        }
        else if (depth <= 5) {
            pool = { "goblin", "goblin", "orc" };
        }
        else {
            pool = { "orc", "goblin", "orc" };
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