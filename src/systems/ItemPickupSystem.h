// src/systems/ItemPickupSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"

class ItemPickupSystem : public System {
private:
    MessageLog* message_log;

public:
    ItemPickupSystem(MessageLog* log) : message_log(log) {}

    void update(ComponentManager& components, float dt) override {
        // Find the player
        auto* player_tags = components.get_array<PlayerControlled>();
        if (!player_tags || player_tags->size() == 0) return;

        Entity player = player_tags->get_entities()[0];
        Position* player_pos = components.get_component<Position>(player);
        Inventory* inventory = components.get_component<Inventory>(player);

        if (!player_pos || !inventory) return;

        // Find all items on the same tile as player
        auto* items = components.get_array<Item>();
        auto* positions = components.get_array<Position>();

        if (!items || !positions) return;

        auto& item_entities = items->get_entities();
        auto& item_data = items->get_components();

        // Collect items to pick up (can't modify while iterating)
        std::vector<Entity> items_to_pickup;

        for (size_t i = 0; i < item_data.size(); i++) {
            Entity item_entity = item_entities[i];
            Position* item_pos = positions->get(item_entity);

            if (!item_pos) continue;

            // Check if player is on same tile
            if (item_pos->x == player_pos->x && item_pos->y == player_pos->y) {
                items_to_pickup.push_back(item_entity);
            }
        }

        // Pick up all items
        for (Entity item_entity : items_to_pickup) {
            pickup_item(components, player, item_entity, inventory);
        }
    }

private:
    void pickup_item(ComponentManager& components, Entity player,
        Entity item_entity, Inventory* inventory) {
        Item* item = components.get_component<Item>(item_entity);
        Name* item_name = components.get_component<Name>(item_entity);

        if (!item) return;

        std::string name = item_name ? item_name->name : "item";

        // Handle different item types
        if (item->item_type == "gold") {
            inventory->gold += item->quantity;

            if (message_log) {
                message_log->add_success("You pick up " + std::to_string(item->quantity) + " gold.");
            }

            // Remove the gold entity from the world
            // Hide it by removing renderable and position
            if (components.has_component<Renderable>(item_entity)) {
                components.remove_component<Renderable>(item_entity);
            }
            if (components.has_component<Position>(item_entity)) {
                components.remove_component<Position>(item_entity);
            }

            // Mark as picked up (you could add a "PickedUp" tag component)
            components.add_component(item_entity, Dead{});  // Reuse Dead tag for cleanup
        }
        else {
            // Other items go into inventory
            inventory->add_item(item_entity);

            if (message_log) {
                message_log->add_success("You pick up " + name + ".");
            }

            // Remove from map
            if (components.has_component<Position>(item_entity)) {
                components.remove_component<Position>(item_entity);
            }
            if (components.has_component<Renderable>(item_entity)) {
                components.remove_component<Renderable>(item_entity);
            }
        }
    }
};