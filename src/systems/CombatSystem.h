// src/systems/CombatSystem.h
#pragma once

#include "../ecs/System.h"
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include "../graphics/SpriteManager.h"
#include <algorithm>
#include <random>

class CombatSystem : public System {
private:
    MessageLog* message_log;
    World* world;  // NEW: Need world to spawn items
    SpriteManager* sprite_manager;  // NEW: For item sprites
    std::mt19937 rng;  // NEW: For random gold amounts

public:
    CombatSystem(MessageLog* log, World* w, SpriteManager* sm)
        : message_log(log), world(w), sprite_manager(sm), rng(std::random_device{}()) {
    }

    void update(ComponentManager& components, float dt) override {
        // This system doesn't run automatically - combat is triggered manually
        // See try_attack() method
    }

    // Attempt an attack from attacker to defender
    // Returns true if attack succeeded
    bool try_attack(ComponentManager& components, Entity attacker, Entity defender) {
        CombatStats* attacker_stats = components.get_component<CombatStats>(attacker);
        CombatStats* defender_stats = components.get_component<CombatStats>(defender);

        if (!attacker_stats || !defender_stats) {
            return false;  // One doesn't have combat stats
        }

        if (!defender_stats->is_alive()) {
            return false;  // Already dead
        }

        // Calculate damage: attack - defense, minimum 1
        int damage = std::max(1, attacker_stats->attack - defender_stats->defense);

        defender_stats->take_damage(damage);

        // Get names for combat message
        Name* attacker_name = components.get_component<Name>(attacker);
        Name* defender_name = components.get_component<Name>(defender);

        std::string attacker_str = attacker_name ? attacker_name->name : "Something";
        std::string defender_str = defender_name ? defender_name->name : "something";

        // Combat message
        if (message_log) {
            std::string msg = attacker_str + " attacks " + defender_str +
                " for " + std::to_string(damage) + " damage!";
            message_log->add_combat(msg);
        }

        // Check if defender died
        if (!defender_stats->is_alive()) {
            handle_death(components, defender);

            if (message_log) {
                std::string msg = defender_str + " dies!";
                message_log->add_warning(msg);
            }
        }

        return true;
    }

    // Mark entity as dead and spawn loot
    void handle_death(ComponentManager& components, Entity entity) {
        // Get position before we do anything
        Position* death_pos = components.get_component<Position>(entity);

        // Check if this was a goblin (or other enemy that drops loot)
        Name* entity_name = components.get_component<Name>(entity);
        bool was_goblin = (entity_name && entity_name->name == "Goblin");

        // Add Dead tag
        components.add_component(entity, Dead{});

        // Remove blocking so player can walk over the corpse/gold
        if (components.has_component<BlocksMovement>(entity)) {
            components.remove_component<BlocksMovement>(entity);
        }

        // Remove AI
        if (components.has_component<AI>(entity)) {
            components.remove_component<AI>(entity);
        }

        // Remove Energy
        if (components.has_component<Energy>(entity)) {
            components.remove_component<Energy>(entity);
        }

        // If goblin, spawn gold pile at death location
        if (was_goblin && death_pos && world) {
            std::uniform_int_distribution<int> gold_dist(5, 15);
            int gold_amount = gold_dist(rng);

            spawn_gold(death_pos->x, death_pos->y, gold_amount);

            if (message_log) {
                message_log->add_success("The goblin drops " + std::to_string(gold_amount) + " gold!");
            }
        }

        // Change sprite to corpse/invisible (for non-loot-dropping enemies)
        if (!was_goblin) {
            SpriteBase* sprite = components.get_component<SpriteBase>(entity);
            if (sprite) {
                sprite->base_name = "corpse";
                sprite->variant = "";
            }
        }
        // For goblins, we'll hide the sprite since gold replaced it
        else {
            Renderable* rend = components.get_component<Renderable>(entity);
            if (rend) {
                rend->layer = -100;  // Hide it way below everything
            }
        }
    }

private:
    void spawn_gold(int x, int y, int amount) {
        Entity gold = world->create_entity();
        world->add_component(gold, Position{ x, y });
        world->add_component(gold, Name{ "Gold" });
        world->add_component(gold, Item{ "gold", amount });

        // Use gold sprite (you'll need to add this to sprites.json)
        if (sprite_manager->has_sprite("gold")) {
            world->add_component(gold, sprite_manager->create_renderable("gold"));
        }
        else {
            // Fallback: use a simple sprite
            world->add_component(gold, Renderable{ 0, 4, 1, 0 });  // Adjust coords as needed
        }
    }
};