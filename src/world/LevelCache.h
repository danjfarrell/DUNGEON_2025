// src/world/LevelCache.h
#pragma once

#include "Map.h"
#include "TileVisibility.h"  // ADD THIS
#include <memory>
#include <unordered_map>

struct LevelData {
    std::unique_ptr<Map> map;
    int up_stairs_x;    // CHANGED: Where up-stairs are (spawn here when descending)
    int up_stairs_y;
    int down_stairs_x;  // NEW: Where down-stairs are (spawn here when ascending)
    int down_stairs_y;
    std::unique_ptr<TileVisibility> exploration;  // NEW: Save exploration state
    bool enemies_spawned;  // NEW: Track if enemies generated

    LevelData(std::unique_ptr<Map> m, int ux, int uy, int dx, int dy, bool spawned = false)
        : map(std::move(m)),
        up_stairs_x(ux), up_stairs_y(uy),
        down_stairs_x(dx), down_stairs_y(dy), 
        exploration(nullptr), 
        enemies_spawned(spawned) {}  // Initialize as null
    
};

class LevelCache {
private:
    std::unordered_map<int, std::unique_ptr<LevelData>> cached_levels;
    int max_cached_levels;

public:
    LevelCache(int max_cache = 20) : max_cached_levels(max_cache) {}



    // Store a level with BOTH stair positions
    void cache_level(int depth, std::unique_ptr<Map> map,
        int up_stairs_x, int up_stairs_y,
        int down_stairs_x, int down_stairs_y, bool enemies_spawned = false) {
        if (cached_levels.size() >= max_cached_levels) {
            cached_levels.clear();
        }

        cached_levels[depth] = std::make_unique<LevelData>(
            std::move(map),
            up_stairs_x, up_stairs_y,
            down_stairs_x, down_stairs_y,
            enemies_spawned
        );
    }

    LevelData* get_level(int depth) {
        auto it = cached_levels.find(depth);
        if (it != cached_levels.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool has_level(int depth) const {
        return cached_levels.find(depth) != cached_levels.end();
    }

    void evict_level(int depth) {
        cached_levels.erase(depth);
    }

    void clear() {
        cached_levels.clear();
    }
};