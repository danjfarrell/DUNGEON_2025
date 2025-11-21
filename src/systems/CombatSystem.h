// src/systems/CombatSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include <algorithm>

class CombatSystem : public System {
private:
    MessageLog* message_log;

public:
    CombatSystem(MessageLog* log) : message_log(log) {}

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

    // Mark entity as dead
    void handle_death(ComponentManager& components, Entity entity) {
        // Add Dead tag
        components.add_component(entity, Dead{});

        // Remove blocking
        if (components.has_component<BlocksMovement>(entity)) {
            components.remove_component<BlocksMovement>(entity);
        }

        // Remove AI
        if (components.has_component<AI>(entity)) {
            components.remove_component<AI>(entity);
        }

        // Change sprite to corpse (if you have one)
        SpriteBase* sprite = components.get_component<SpriteBase>(entity);
        if (sprite) {
            sprite->base_name = "corpse";
            sprite->variant = "";
        }
    }
};