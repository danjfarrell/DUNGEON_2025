// src/data/EquipmentDatabase.h
// Hardcoded equipment catalog, mirroring ConsumableSystem's own hardcoded
// consumable map (not everything is JSON-driven yet — see CLAUDE.md).
// Nothing in the codebase spawned an EquippableItem before this; the equip
// UI/input existed with no content to act on. This gives it content.
#pragma once

#include "../components/Equipment.h"
#include <string>
#include <unordered_map>

struct EquipmentDefinition {
    std::string display_name;
    EquippableItem item;
};

class EquipmentDatabase {
private:
    std::unordered_map<std::string, EquipmentDefinition> items;

public:
    EquipmentDatabase() {
        initialize_items();
    }

    const EquipmentDefinition* get_item(const std::string& item_type) const {
        auto it = items.find(item_type);
        return it != items.end() ? &it->second : nullptr;
    }

    bool is_equippable(const std::string& item_type) const {
        return items.find(item_type) != items.end();
    }

private:
    void initialize_items() {
        items["rusty_dagger"] = {
            "Rusty Dagger",
            EquippableItem{ EquipmentSlot::MAIN_HAND, ItemStats{ 2, 0, 0, 0, 0 } }
        };
        items["steel_sword"] = {
            "Steel Sword",
            EquippableItem{ EquipmentSlot::MAIN_HAND, ItemStats{ 5, 0, 0, 0, 0 } }
        };
        items["wooden_shield"] = {
            "Wooden Shield",
            EquippableItem{ EquipmentSlot::OFF_HAND, ItemStats{ 0, 2, 0, 0, 0 } }
        };
        items["leather_armor"] = {
            "Leather Armor",
            EquippableItem{ EquipmentSlot::CHEST, ItemStats{ 0, 1, 5, 0, 0 } }
        };
        items["chainmail"] = {
            "Chainmail",
            EquippableItem{ EquipmentSlot::CHEST, ItemStats{ 0, 4, 10, 0, 0 } }
        };
        items["iron_ring"] = {
            "Iron Ring",
            EquippableItem{ EquipmentSlot::RING1, ItemStats{ 1, 1, 0, 0, 0 } }
        };
    }
};
