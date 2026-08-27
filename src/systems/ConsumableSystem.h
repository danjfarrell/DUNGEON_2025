// src/systems/ConsumableSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include "StatusEffectHelpers.h"

// Consumable effect types
enum class ConsumableEffect {
    HEAL_HP,
    RESTORE_MANA,
    CURE_POISON,
    GRANT_HASTE
};

// Data about a consumable item
struct ConsumableData {
    ConsumableEffect effect;
    int amount;
    std::string use_message;

    ConsumableData(ConsumableEffect eff = ConsumableEffect::HEAL_HP,
        int amt = 0,
        const std::string& msg = "")
        : effect(eff), amount(amt), use_message(msg) {
    }
};

class ConsumableSystem : public System {
private:
    MessageLog* message_log;

    // Database of consumable items
    std::unordered_map<std::string, ConsumableData> consumables;

public:
    ConsumableSystem(MessageLog* log) : message_log(log) {
        initialize_consumables();
    }

    void update(ComponentManager& components, float dt) override {
        // This system doesn't run automatically
        // Call use_item() manually when player uses item
    }

    // Use a consumable item
    bool use_item(ComponentManager& components, Entity user, Entity item_entity) {
        Item* item = components.get_component<Item>(item_entity);
        if (!item) return false;

        // An already-depleted item should never apply its effect again.
        if (item->quantity <= 0) return false;

        // Look up consumable data
        auto it = consumables.find(item->item_type);
        if (it == consumables.end()) {
            if (message_log) {
                message_log->add_warning("This item cannot be used.");
            }
            return false;
        }

        const ConsumableData& data = it->second;

        // Apply effect
        bool success = apply_effect(components, user, data);

        if (success) {
            // Show message
            if (message_log && !data.use_message.empty()) {
                message_log->add_success(data.use_message);
            }

            // Decrease quantity
            item->quantity--;

            // Remove item if quantity is 0
            if (item->quantity <= 0) {
                // Mark for removal
                return true;  // Signal caller to remove item
            }
        }

        return success;
    }

    // Use item from inventory slot
    bool use_from_inventory(ComponentManager& components, Entity user, int slot_index) {
        Inventory* inv = components.get_component<Inventory>(user);
        if (!inv) return false;

        if (slot_index < 0 || slot_index >= static_cast<int>(inv->items.size())) {
            return false;
        }

        Entity item_entity = inv->items[slot_index];
        bool used = use_item(components, user, item_entity);

        // Remove from inventory if consumed
        if (used) {
            Item* item = components.get_component<Item>(item_entity);
            if (!item || item->quantity <= 0) {
                inv->items.erase(inv->items.begin() + slot_index);

                // Clean up entity
                if (components.has_component<Item>(item_entity)) {
                    components.remove_component<Item>(item_entity);
                }
                if (components.has_component<Name>(item_entity)) {
                    components.remove_component<Name>(item_entity);
                }
            }
        }

        return used;
    }

    // Check if item is consumable
    bool is_consumable(const std::string& item_type) const {
        return consumables.find(item_type) != consumables.end();
    }

private:
    void initialize_consumables() {
        // Health potions
        consumables["health_potion"] = ConsumableData{
            ConsumableEffect::HEAL_HP,
            25,
            "You drink a health potion and recover 25 HP."
        };

        consumables["greater_health_potion"] = ConsumableData{
            ConsumableEffect::HEAL_HP,
            50,
            "You drink a greater health potion and recover 50 HP."
        };

        consumables["superior_health_potion"] = ConsumableData{
            ConsumableEffect::HEAL_HP,
            100,
            "You drink a superior health potion and fully recover!"
        };

        // Mana potions
        consumables["mana_potion"] = ConsumableData{
            ConsumableEffect::RESTORE_MANA,
            30,
            "You drink a mana potion and restore 30 MP."
        };

        consumables["greater_mana_potion"] = ConsumableData{
            ConsumableEffect::RESTORE_MANA,
            60,
            "You drink a greater mana potion and restore 60 MP."
        };
    }

    bool apply_effect(ComponentManager& components, Entity target,
        const ConsumableData& data) {
        switch (data.effect) {
        case ConsumableEffect::HEAL_HP: {
            CombatStats* stats = components.get_component<CombatStats>(target);
            if (stats) {
                int before = stats->current_hp;
                stats->heal(data.amount);
                int healed = stats->current_hp - before;

                if (healed > 0) {
                    if (message_log) {
                        message_log->add_success("Restored " +
                            std::to_string(healed) + " HP!");
                    }
                    return true;
                }
                else {
                    if (message_log) {
                        message_log->add_info("Already at full health.");
                    }
                    return false;
                }
            }
            break;
        }

        case ConsumableEffect::RESTORE_MANA: {
            Mana* mana = components.get_component<Mana>(target);
            if (mana) {
                int before = mana->current;
                mana->restore(data.amount);
                int restored = mana->current - before;

                if (restored > 0) {
                    if (message_log) {
                        message_log->add_success("Restored " +
                            std::to_string(restored) + " MP!");
                    }
                    return true;
                }
                else {
                    if (message_log) {
                        message_log->add_info("Already at full mana.");
                    }
                    return false;
                }
            }
            break;
        }

        case ConsumableEffect::CURE_POISON: {
            if (components.has_component<Poisoned>(target)) {
                components.remove_component<Poisoned>(target);
                if (message_log) {
                    message_log->add_success("The poison is cleansed from your body!");
                }
                return true;
            }
            if (message_log) {
                message_log->add_info("You aren't poisoned.");
            }
            return false;  // nothing to cure -- don't consume the potion
        }

        case ConsumableEffect::GRANT_HASTE: {
            if (!components.has_component<Energy>(target)) {
                return false;
            }
            StatusEffects::apply_haste(components, target);
            if (message_log) {
                message_log->add_success("You feel incredibly fast!");
            }
            return true;
        }
        }

        return false;
    }
};