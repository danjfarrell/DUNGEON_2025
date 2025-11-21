// src/systems/TurnManager.h
#pragma once

#include "../ecs/World.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"

enum class TurnState {
    PLAYER_TURN,      // Waiting for player input
    ENEMY_TURN,       // Enemies are acting
    PROCESSING        // Resolving actions
};

class TurnManager {
private:
    TurnState state;
    int turn_number;
    MessageLog* message_log;

public:
    TurnManager(MessageLog* log = nullptr)
        : state(TurnState::PLAYER_TURN), turn_number(0), message_log(log) {
    }

    TurnState get_state() const { return state; }
    int get_turn_number() const { return turn_number; }

    // Player has completed their action
    void end_player_turn() {
        state = TurnState::ENEMY_TURN;
        if (message_log) {
            message_log->next_turn();
        }
    }

    // All enemies have acted
    void end_enemy_turn() {
        state = TurnState::PLAYER_TURN;
        turn_number++;
    }

    // Process all entities with energy
    void process_turn(World& world) {
        ComponentManager& components = world.get_component_manager();

        auto* energies = components.get_array<Energy>();
        if (!energies) return;

        // Give everyone energy
        auto& energy_data = energies->get_components();
        auto& energy_entities = energies->get_entities();

        for (size_t i = 0; i < energy_data.size(); i++) {
            energy_data[i].gain_energy();

            // Mark entities that can act
            if (energy_data[i].can_act()) {
                Entity entity = energy_entities[i];

                // Skip player (handled separately by input)
                if (!world.has_component<PlayerControlled>(entity)) {
                    world.add_component(entity, WantsToAct{});
                }
            }
        }
    }

    bool is_player_turn() const {
        return state == TurnState::PLAYER_TURN;
    }

    bool is_enemy_turn() const {
        return state == TurnState::ENEMY_TURN;
    }
};