// src/world/LevelCache.h
#pragma once

#include "Map.h"
#include <memory>
#include <unordered_map>

struct LevelData {
    std::unique_ptr<Map> map;
    int entrance_x;  // Where player entered from
    int entrance_y;

    LevelData(std::unique_ptr<Map> m, int x, int y)
        : map(std::move(m)), entrance_x(x), entrance_y(y) {
    }
};

class LevelCache {
private:
    std::unordered_map<int, std::unique_ptr<LevelData>> cached_levels;
    int max_cached_levels;

public:
    LevelCache(int max_cache = 20) : max_cached_levels(max_cache) {}

    // Store a level
    void cache_level(int depth, std::unique_ptr<Map> map, int entrance_x, int entrance_y) {
        // Evict oldest if cache full
        if (cached_levels.size() >= max_cached_levels) {
            // Simple eviction: remove furthest from current depth
            // (You could make this smarter - LRU, etc.)
            cached_levels.clear();  // For now, just clear all
        }

        cached_levels[depth] = std::make_unique<LevelData>(
            std::move(map), entrance_x, entrance_y
        );
    }

    // Retrieve a level (returns nullptr if not cached)
    LevelData* get_level(int depth) {
        auto it = cached_levels.find(depth);
        if (it != cached_levels.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    // Check if level is cached
    bool has_level(int depth) const {
        return cached_levels.find(depth) != cached_levels.end();
    }

    // Remove a level from cache
    void evict_level(int depth) {
        cached_levels.erase(depth);
    }

    // Clear all cached levels
    void clear() {
        cached_levels.clear();
    }
};