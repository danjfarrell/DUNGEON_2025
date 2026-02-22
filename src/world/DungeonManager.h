// src/world/DungeonManager.h
#pragma once

#include "Map.h"
#include "MapGenerators.h"
#include "LevelCache.h"
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../spawning/EnemySpawner.h"
#include "../graphics/SpriteManager.h"
#include <memory>
#include <random>

class DungeonManager {
private:
    int current_depth;
    Map* current_map;
    LevelCache level_cache;
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

    // ========================================
    // UPDATED: Now accepts direction parameter
    // ========================================
    Map* generate_level(int depth, Position* player_exit_pos = nullptr, bool descending = true) {
        current_depth = depth;



        // Check if level is cached
        LevelData* cached = level_cache.get_level(depth);
        if (cached) {
            current_map = cached->map.get();

            // Spawn at correct stairs based on direction
            if (player_exit_pos) {
                if (descending) {
                    // Coming down from above -> spawn at up-stairs
                    player_exit_pos->x = cached->up_stairs_x;
                    player_exit_pos->y = cached->up_stairs_y;
                }
                else {
                    // Coming up from below -> spawn at down-stairs
                    player_exit_pos->x = cached->down_stairs_x;
                    player_exit_pos->y = cached->down_stairs_y;
                }
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

        // Find BOTH stair positions
        int up_stairs_x = -1;
        int up_stairs_y = -1;
        int down_stairs_x = -1;
        int down_stairs_y = -1;

        // Search for stairs
        for (int y = 0; y < new_map->get_height(); y++) {
            for (int x = 0; x < new_map->get_width(); x++) {
                TileType tile = new_map->get_tile(x, y);

                if (tile == TileType::STAIRS_UP) {
                    up_stairs_x = x;
                    up_stairs_y = y;
                }
                else if (tile == TileType::STAIRS_DOWN) {
                    down_stairs_x = x;
                    down_stairs_y = y;
                }
            }
        }

        // Fallback: use first room center if no stairs found
        if (up_stairs_x == -1 && !new_map->get_rooms().empty()) {
            const Room& first_room = new_map->get_rooms()[0];
            up_stairs_x = first_room.center_x();
            up_stairs_y = first_room.center_y();
        }

        if (down_stairs_x == -1 && !new_map->get_rooms().empty()) {
            const Room& last_room = new_map->get_rooms().back();
            down_stairs_x = last_room.center_x();
            down_stairs_y = last_room.center_y();
        }

        //  FIX: Cache FIRST, then get pointer
        level_cache.cache_level(depth, std::move(new_map),
            up_stairs_x, up_stairs_y,
            down_stairs_x, down_stairs_y);

        // Get pointer from cache (valid memory)
        current_map = level_cache.get_level(depth)->map.get();

        // Set player spawn based on direction
        if (player_exit_pos) {
            if (descending) {
                player_exit_pos->x = up_stairs_x;
                player_exit_pos->y = up_stairs_y;
            }
            else {
                player_exit_pos->x = down_stairs_x;
                player_exit_pos->y = down_stairs_y;
            }
        }

        return current_map;
    }

    Map* get_current_map() {
        return current_map;
    }

    int get_current_depth() const {
        return current_depth;
    }

    // ========================================
    // UPDATED: Get spawn position based on direction
    // ========================================
    Position get_player_spawn_position(bool descending = true) {
        LevelData* cached = level_cache.get_level(current_depth);
        if (cached) {
            if (descending) {
                return Position{ cached->up_stairs_x, cached->up_stairs_y };
            }
            else {
                return Position{ cached->down_stairs_x, cached->down_stairs_y };
            }
        }

        // Fallback
        if (current_map && !current_map->get_rooms().empty()) {
            const Room& first_room = current_map->get_rooms()[0];
            return Position{ first_room.center_x(), first_room.center_y() };
        }

        return Position{ 5, 5 };
    }

    void spawn_enemies(EnemySpawner& spawner, World& world) {
        LevelData* cached = level_cache.get_level(current_depth);

        if (cached && cached->enemies_spawned) {
            return;
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
        if (cached) {
            cached->enemies_spawned = true;
        }
    }

    // ========================================
    // NEW: Save exploration state for current level
    // ========================================
    void save_exploration(int depth, std::unique_ptr<TileVisibility> exploration) {
        LevelData* level = level_cache.get_level(depth);
        if (level) {
            level->exploration = std::move(exploration);
        }
    }

    // ========================================
    // NEW: Get saved exploration state
    // ========================================
    std::unique_ptr<TileVisibility> get_exploration(int depth) {
        LevelData* level = level_cache.get_level(depth);
        if (level && level->exploration) {
            // Move it out (caller takes ownership)
            return std::move(level->exploration);
        }
        return nullptr;
    }

    bool has_enemies_spawned(int depth) {
        LevelData* level = level_cache.get_level(depth);
        return level && level->enemies_spawned;
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

        // Always place up-stairs (even on depth 1 for consistency)
        const Room& first_room = rooms[0];
        if (depth > 1) {
            map.set_tile(first_room.center_x(), first_room.center_y(),
                TileType::STAIRS_UP);
        }

        // Always place down-stairs
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