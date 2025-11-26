// src/systems/AISystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../world/Map.h"
#include "CombatSystem.h"
#include <cmath>
#include <random>
#include <iostream>

class AISystem : public System {
private:
    Map* game_map;
    CombatSystem* combat_system;
    std::mt19937 rng;

public:
    AISystem(Map* map, CombatSystem* combat)
        : game_map(map), combat_system(combat), rng(std::random_device{}()) {
    }

    void update(ComponentManager& components, float dt) override {
        // Find player position
		//std::cout << "AISystem Update Called" << std::endl;
        Entity player = find_player(components);
        if (player == 0) 
        {
            std::cout << "No player found" << std::endl;
            return;  // No player found
        }
       
        Position* player_pos = components.get_component<Position>(player);
        if (!player_pos) return;
        std::cout << player_pos << std::endl;
        // Process all entities that want to act
        auto* wants_to_act = components.get_array<WantsToAct>();
        if (!wants_to_act) return;
        std::cout << wants_to_act << std::endl;

        auto& acting_entities = wants_to_act->get_entities();

        // Copy the list because we'll be modifying components
        std::vector<Entity> entities_to_process(acting_entities.begin(), acting_entities.end());

        for (Entity entity : entities_to_process) {
            // Skip if dead
            if (components.has_component<Dead>(entity)) {
                components.remove_component<WantsToAct>(entity);
                continue;
            }

            AI* ai = components.get_component<AI>(entity);
            Position* pos = components.get_component<Position>(entity);
            Energy* energy = components.get_component<Energy>(entity);

            if (!ai || !pos || !energy) continue;

            // Take action based on AI type
            switch (ai->type) {
            case AI::AGGRESSIVE:
                aggressive_behavior(components, entity, pos, player, player_pos);
                break;

            case AI::PATROL:
                patrol_behavior(components, entity, pos);
                break;

            case AI::IDLE:
                // Do nothing
                break;
            }

            // Consume energy and remove tag
            energy->consume_turn();
            components.remove_component<WantsToAct>(entity);
        }
    }

private:
    Entity find_player(ComponentManager& components) {
        auto* player_tags = components.get_array<PlayerControlled>();
        if (!player_tags || player_tags->size() == 0) return 0;

        return player_tags->get_entities()[0];
    }

    float distance(Position* a, Position* b) {
        int dx = a->x - b->x;
        int dy = a->y - b->y;
        return std::sqrt(dx * dx + dy * dy);
    }

    void aggressive_behavior(ComponentManager& components, Entity entity,
        Position* pos, Entity player, Position* player_pos) {
        float dist = distance(pos, player_pos);

        // If adjacent to player, attack!
        if (dist <= 1.5f) {  // 1.5 catches diagonals
            combat_system->try_attack(components, entity, player);
            return;
        }

        // If player is within 8 tiles, chase
        if (dist < 8.0f) {
            move_towards(components, entity, pos, player_pos);
        }
        else {
            // Wander randomly
            wander(components, entity, pos);
        }
    }

    void patrol_behavior(ComponentManager& components, Entity entity, Position* pos) {
        // Simple patrol: just wander
        wander(components, entity, pos);
    }

    void move_towards(ComponentManager& components, Entity entity,
        Position* pos, Position* target) {
        int dx = target->x - pos->x;
        int dy = target->y - pos->y;

        // Normalize to -1, 0, or 1
        int step_x = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
        int step_y = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;

        // Try to move in the most direct direction
        int new_x = pos->x + step_x;
        int new_y = pos->y + step_y;

        if (can_move_to(components, new_x, new_y, entity)) {
            pos->x = new_x;
            pos->y = new_y;
            return;
        }

        // If blocked, try cardinal directions only
        if (step_x != 0 && can_move_to(components, pos->x + step_x, pos->y, entity)) {
            pos->x += step_x;
        }
        else if (step_y != 0 && can_move_to(components, pos->x, pos->y + step_y, entity)) {
            pos->y += step_y;
        }
    }

    void wander(ComponentManager& components, Entity entity, Position* pos) {
        std::uniform_int_distribution<int> dir_dist(0, 3);
        int dir = dir_dist(rng);

        int dx = 0, dy = 0;
        switch (dir) {
        case 0: dy = -1; break;  // North
        case 1: dy = 1;  break;  // South
        case 2: dx = -1; break;  // West
        case 3: dx = 1;  break;  // East
        }

        int new_x = pos->x + dx;
        int new_y = pos->y + dy;

        if (can_move_to(components, new_x, new_y, entity)) {
            pos->x = new_x;
            pos->y = new_y;
        }
    }

    bool can_move_to(ComponentManager& components, int x, int y, Entity mover) {
        // Check map
        if (!game_map->is_walkable(x, y)) {
            return false;
        }

        // Check for blocking entities
        auto* blockers = components.get_array<BlocksMovement>();
        auto* positions = components.get_array<Position>();

        if (blockers && positions) {
            auto& blocker_entities = blockers->get_entities();
            for (Entity blocker : blocker_entities) {
                if (blocker == mover) continue;  // Don't block yourself

                Position* blocker_pos = positions->get(blocker);
                if (blocker_pos && blocker_pos->x == x && blocker_pos->y == y) {
                    return false;
                }
            }
        }

        return true;
    }
};