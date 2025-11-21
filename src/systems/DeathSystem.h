// src/systems/DeathSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"

class DeathSystem : public System {
public:
    void update(ComponentManager& components, float dt) override {
        auto* dead_tags = components.get_array<Dead>();
        if (!dead_tags) return;

        auto& dead_entities = dead_tags->get_entities();

        // Copy list since we'll be modifying it
        std::vector<Entity> to_process(dead_entities.begin(), dead_entities.end());

        for (Entity entity : to_process) {
            // Change sprite to corpse or make invisible
            Renderable* rend = components.get_component<Renderable>(entity);
            if (rend) {
                rend->layer = 0;  // Put corpses on bottom layer
            }

            // Optional: Actually destroy the entity after some time
            // For now, we'll just leave corpses around
            // You could add a "decay timer" component later
        }
    }
};