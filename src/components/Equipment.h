// src/components/Equipment.h
#pragma once

#include "../ecs/Entity.h"
#include <array>

// Equipment slot types
enum class EquipmentSlot {
    HEAD = 0,       // Helmet
    NECK,           // Amulet
    CHEST,          // Armor
    MAIN_HAND,      // Weapon
    OFF_HAND,       // Shield
    RING1,          // Ring slot 1
    RING2,          // Ring slot 2
    BELT,           // Belt
    COUNT           // Total number of slots
};

// Equipment component - tracks what items are equipped
struct Equipment {
    std::array<Entity, static_cast<size_t>(EquipmentSlot::COUNT)> slots;

    Equipment() {
        // Initialize all slots to 0 (empty)
        for (auto& slot : slots) {
            slot = 0;
        }
    }

    // Equip an item to a slot
    void equip(EquipmentSlot slot, Entity item_entity) {
        slots[static_cast<size_t>(slot)] = item_entity;
    }

    // Unequip an item from a slot
    Entity unequip(EquipmentSlot slot) {
        Entity old_item = slots[static_cast<size_t>(slot)];
        slots[static_cast<size_t>(slot)] = 0;
        return old_item;
    }

    // Get equipped item in slot
    Entity get_equipped(EquipmentSlot slot) const {
        return slots[static_cast<size_t>(slot)];
    }

    // Check if slot is empty
    bool is_empty(EquipmentSlot slot) const {
        return slots[static_cast<size_t>(slot)] == 0;
    }
};

// Item stat modifiers (for equipment items)
struct ItemStats {
    int attack_bonus;
    int defense_bonus;
    int hp_bonus;
    int mana_bonus;
    int speed_bonus;

    ItemStats(int atk = 0, int def = 0, int hp = 0, int mp = 0, int spd = 0)
        : attack_bonus(atk), defense_bonus(def), hp_bonus(hp),
        mana_bonus(mp), speed_bonus(spd) {
    }
};

// Equipment type (what slot it goes in)
struct EquippableItem {
    EquipmentSlot slot;
    ItemStats stats;

    EquippableItem(EquipmentSlot s = EquipmentSlot::MAIN_HAND,
        ItemStats st = ItemStats())
        : slot(s), stats(st) {
    }
};
