// src/spawning/EnemySpawner.h
#pragma once

#include "../ecs/World.h"
#include "../components/Components.h"
#include "../graphics/SpriteManager.h"
#include "../data/EnemyData.h"
#include "../utils/Logger.h"

class EnemySpawner {
private:
    World* world;
    SpriteManager* sprite_manager;
    EnemyDataManager* enemy_data;

    // Helper to convert AI string to enum
    AI::Type parse_ai_type(const std::string& ai_str) {
        if (ai_str == "aggressive") return AI::AGGRESSIVE;
        if (ai_str == "patrol") return AI::PATROL;
        if (ai_str == "defensive") return AI::DEFENSIVE;
        return AI::IDLE;
    }

public:
    EnemySpawner(World* w, SpriteManager* sm, EnemyDataManager* ed)
        : world(w), sprite_manager(sm), enemy_data(ed) {
    }

    // Spawn a single enemy at a position
    Entity spawn(const std::string& enemy_id, int x, int y) {
        const EnemyDefinition* def = enemy_data->get_enemy(enemy_id);
        if (!def) {
            LOG_ERROR("Failed to spawn enemy: " + enemy_id + " not found");
            return 0;  // Invalid entity
        }

        Entity enemy = world->create_entity();

        // Position
        world->add_component(enemy, Position{ x, y });

        // Name
        world->add_component(enemy, Name{ def->name });

        // Sprite
        if (sprite_manager->has_sprite(def->sprite)) {
            world->add_component(enemy, sprite_manager->create_renderable(def->sprite));
        }
        else {
            LOG_WARN("Sprite not found for " + enemy_id + ": " + def->sprite);
            world->add_component(enemy, Renderable{ 0, 1, 0, 1 });  // Fallback
        }

        // Combat stats
        world->add_component(enemy, CombatStats{
            def->stats.attack,
            def->stats.defense,
            def->stats.hp
            });

        // Energy (for turn system)
        world->add_component(enemy, Energy{ def->stats.speed });

        // AI
        world->add_component(enemy, AI{ parse_ai_type(def->ai_type) });

        // Enemy type (for loot/xp lookup on death)
        world->add_component(enemy, EnemyType{ enemy_id });

        // Blocks movement
        world->add_component(enemy, BlocksMovement{});

        LOG_INFO("Spawned " + def->name + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");

        return enemy;
    }

    // Spawn multiple enemies of the same type
    std::vector<Entity> spawn_multiple(const std::string& enemy_id,
        const std::vector<std::pair<int, int>>& positions) {
        std::vector<Entity> spawned;
        for (const auto& pos : positions) {
            Entity e = spawn(enemy_id, pos.first, pos.second);
            if (e != 0) {
                spawned.push_back(e);
            }
        }
        return spawned;
    }

    // Spawn a random enemy from a list
    Entity spawn_random(const std::vector<std::string>& enemy_ids, int x, int y) {
        if (enemy_ids.empty()) {
            LOG_ERROR("spawn_random called with empty enemy list");
            return 0;
        }

        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<size_t> dist(0, enemy_ids.size() - 1);

        const std::string& chosen = enemy_ids[dist(rng)];
        return spawn(chosen, x, y);
    }

    // Check if an enemy type exists
    bool can_spawn(const std::string& enemy_id) const {
        return enemy_data->has_enemy(enemy_id);
    }

    // Get all available enemy types
    std::vector<std::string> get_available_enemies() const {
        return enemy_data->get_all_enemy_ids();
    }
};