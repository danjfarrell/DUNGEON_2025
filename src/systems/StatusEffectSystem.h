// src/systems/StatusEffectSystem.h
// Ticks Poisoned/Hasted once per turn. NOT registered with World::add_system
// -- Game owns an instance directly and calls process_turn() explicitly from
// its per-turn block, the same reason MagicSystem::regenerate_mana() is
// called explicitly rather than living in update(): World::update() runs
// both once per turn AND once per rendered frame (see CLAUDE.md), and a
// damage-per-turn effect ticking 60x/second would be exactly the mana-regen
// bug again.
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include <vector>

class StatusEffectSystem : public System {
private:
    MessageLog* message_log;

public:
    StatusEffectSystem(MessageLog* log) : message_log(log) {}

    void update(ComponentManager& components, float dt) override {
        // No-op by design -- see the comment above. Call process_turn().
    }

    void process_turn(ComponentManager& components) {
        tick_poison(components);
        tick_haste(components);
    }

private:
    void tick_poison(ComponentManager& components) {
        auto* poisoned = components.get_array<Poisoned>();
        if (!poisoned) return;

        auto& entities = poisoned->get_entities();
        std::vector<Entity> to_process(entities.begin(), entities.end());

        for (Entity entity : to_process) {
            Poisoned* p = components.get_component<Poisoned>(entity);
            if (!p) continue;

            CombatStats* stats = components.get_component<CombatStats>(entity);
            if (stats && stats->is_alive()) {
                stats->take_damage(p->damage_per_turn);
                if (message_log && components.has_component<PlayerControlled>(entity)) {
                    message_log->add_combat("Poison deals " +
                        std::to_string(p->damage_per_turn) + " damage!");
                }
            }

            p->turns_remaining--;
            if (p->turns_remaining <= 0) {
                components.remove_component<Poisoned>(entity);
                if (message_log && components.has_component<PlayerControlled>(entity)) {
                    message_log->add_info("The poison wears off.");
                }
            }
        }
    }

    void tick_haste(ComponentManager& components) {
        auto* hasted = components.get_array<Hasted>();
        if (!hasted) return;

        auto& entities = hasted->get_entities();
        std::vector<Entity> to_process(entities.begin(), entities.end());

        for (Entity entity : to_process) {
            Hasted* h = components.get_component<Hasted>(entity);
            if (!h) continue;

            h->turns_remaining--;
            if (h->turns_remaining <= 0) {
                Energy* energy = components.get_component<Energy>(entity);
                if (energy) {
                    energy->speed = h->original_speed;
                }
                components.remove_component<Hasted>(entity);
                if (message_log && components.has_component<PlayerControlled>(entity)) {
                    message_log->add_info("You feel your haste fade.");
                }
            }
        }
    }
};
