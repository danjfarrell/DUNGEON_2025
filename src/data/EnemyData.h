// src/data/EnemyData.h
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "../utils/Logger.h"

using json = nlohmann::json;

// Item drop info
struct ItemDrop {
    std::string item_type;
    int quantity;
    float chance;  // 0.0 to 1.0

    ItemDrop(const std::string& type = "", int qty = 1, float ch = 1.0f)
        : item_type(type), quantity(qty), chance(ch) {
    }
};

// Gold drop info
struct GoldDrop {
    int min;
    int max;
    float chance;

    GoldDrop(int mn = 0, int mx = 0, float ch = 1.0f)
        : min(mn), max(mx), chance(ch) {
    }
};

// Complete loot table
struct LootTable {
    GoldDrop gold;
    std::vector<ItemDrop> items;
};

// Enemy stats
struct EnemyStats {
    int hp;
    int attack;
    int defense;
    int speed;
};

// Complete enemy definition
struct EnemyDefinition {
    std::string name;
    std::string sprite;
    EnemyStats stats;
    std::string ai_type;  // "aggressive", "patrol", "idle"
    int xp_reward;
    LootTable loot_table;
};

// Manager class to load and access enemy data
class EnemyDataManager {
private:
    std::unordered_map<std::string, EnemyDefinition> enemies;

public:
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open enemy data: " + filepath);
            return false;
        }

        try {
            json data = json::parse(file);

            if (!data.contains("enemies")) {
                LOG_ERROR("Enemy data missing 'enemies' section");
                return false;
            }

            // Parse each enemy
            for (const auto& item : data["enemies"].items()) {
                std::string enemy_id = item.key();
                const json& enemy_json = item.value();

                EnemyDefinition def;
                def.name = enemy_json["name"];
                def.sprite = enemy_json["sprite"];

                // Parse stats
                def.stats.hp = enemy_json["stats"]["hp"];
                def.stats.attack = enemy_json["stats"]["attack"];
                def.stats.defense = enemy_json["stats"]["defense"];
                def.stats.speed = enemy_json["stats"]["speed"];

                // Parse AI type
                def.ai_type = enemy_json["ai"];
                def.xp_reward = enemy_json["xp_reward"];

                // Parse loot table
                if (enemy_json.contains("loot_table")) {
                    const json& loot = enemy_json["loot_table"];

                    // Parse gold
                    if (loot.contains("gold")) {
                        def.loot_table.gold.min = loot["gold"]["min"];
                        def.loot_table.gold.max = loot["gold"]["max"];
                        def.loot_table.gold.chance = loot["gold"]["chance"];
                    }

                    // Parse items
                    if (loot.contains("items")) {
                        for (const auto& item_json : loot["items"]) {
                            ItemDrop drop;
                            drop.item_type = item_json["item_type"];
                            drop.quantity = item_json["quantity"];
                            drop.chance = item_json["chance"];
                            def.loot_table.items.push_back(drop);
                        }
                    }
                }

                enemies[enemy_id] = def;
                LOG_INFO("Loaded enemy: " + enemy_id + " (" + def.name + ")");
            }

            LOG_INFO("Successfully loaded " + std::to_string(enemies.size()) + " enemy types");
            return true;
        }
        catch (const json::exception& e) {
            LOG_ERROR("JSON parsing error in enemy data: " + std::string(e.what()));
            return false;
        }
    }

    // Get enemy definition by ID
    const EnemyDefinition* get_enemy(const std::string& enemy_id) const {
        auto it = enemies.find(enemy_id);
        if (it != enemies.end()) {
            return &it->second;
        }
        LOG_WARN("Enemy not found: " + enemy_id);
        return nullptr;
    }

    // Check if enemy exists
    bool has_enemy(const std::string& enemy_id) const {
        return enemies.find(enemy_id) != enemies.end();
    }

    // Get all enemy IDs (useful for spawning)
    std::vector<std::string> get_all_enemy_ids() const {
        std::vector<std::string> ids;
        for (const auto& pair : enemies) {
            ids.push_back(pair.first);
        }
        return ids;
    }
};