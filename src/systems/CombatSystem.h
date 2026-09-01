// src/systems/CombatSystem.h
#pragma once

#include "../ecs/System.h"
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include "../graphics/SpriteManager.h"
#include "../data/EnemyData.h"
#include "../data/EquipmentDatabase.h"
#include "StatusEffectHelpers.h"
#include "ExperienceSystem.h"  // ADD THIS
#include <algorithm>
#include <random>
#include <unordered_map>




class CombatSystem : public System {
private:
    MessageLog* message_log;
    World* world;  // NEW: Need world to spawn items
    SpriteManager* sprite_manager;  // NEW: For item sprites
    EnemyDataManager* enemy_data;  // NEW!
    ExperienceSystem* xp_system;  // Now this is recognized
    std::mt19937 rng;  // NEW: For random gold amounts
    EquipmentDatabase equipment_db;  // hardcoded catalog, see EquipmentDatabase.h

public:
    CombatSystem(MessageLog* log, World* w, SpriteManager* sm, EnemyDataManager* ed, ExperienceSystem* xp)
        : message_log(log), world(w), sprite_manager(sm), enemy_data(ed), xp_system(xp), rng(std::random_device{}()) {
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

        // Poison on hit: data-driven per enemy type (EnemyStats::poison_chance,
        // 0 by default -- only "rat" has one set right now). No point poisoning
        // a corpse, so only rolls if the defender survived this hit.
        if (defender_stats->is_alive() && enemy_data) {
            EnemyType* attacker_type = components.get_component<EnemyType>(attacker);
            if (attacker_type) {
                const EnemyDefinition* def = enemy_data->get_enemy(attacker_type->enemy_id);
                if (def && def->stats.poison_chance > 0.0f) {
                    std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);
                    if (chance_dist(rng) < def->stats.poison_chance) {
                        StatusEffects::apply_poison(components, defender);
                        if (message_log) {
                            message_log->add_warning(defender_str + " is poisoned!");
                        }
                    }
                }
            }
        }

        // Combat message
        if (message_log) {
            std::string msg = attacker_str + " attacks " + defender_str +
                " for " + std::to_string(damage) + " damage!";
            message_log->add_combat(msg);
        }

        // Check if defender died
        if (!defender_stats->is_alive()) {
            handle_death(components, defender, attacker);

            if (message_log) {
                std::string msg = defender_str + " dies!";
                message_log->add_warning(msg);
            }
        }

        return true;
    }

    // Mark entity as dead and spawn loot
    void handle_death(ComponentManager& components, Entity entity, Entity killer) {
        // Tag as dead either way: DeathSystem reacts to this for enemy corpse
        // visuals, and Game::update() reacts to it on the player to switch to
        // the GAME_OVER state.
        components.add_component(entity, Dead{});

        if (components.has_component<PlayerControlled>(entity)) {
            // The player isn't a corpse to sweep off the map. Leave their
            // Position/Renderable/etc. alone (unlike the enemy cleanup below)
            // so the game-over screen can still show where they fell, and so
            // Game::update() can keep reading their CombatStats.
            return;
        }

        // IMPORTANT: Copy position VALUES, not pointer!
        int death_x = -1;
        int death_y = -1;
        Position* death_pos = components.get_component<Position>(entity);
        if (death_pos) {
            death_x = death_pos->x;
            death_y = death_pos->y;
        }

        // Check if this is an enemy with loot data
        EnemyType* enemy_type = components.get_component<EnemyType>(entity);
        Name* entity_name = components.get_component<Name>(entity);

        // IMPORTANT: Completely remove entity from map to fix corpse bug
        if (components.has_component<BlocksMovement>(entity)) {
            components.remove_component<BlocksMovement>(entity);
        }
        if (components.has_component<AI>(entity)) {
            components.remove_component<AI>(entity);
        }
        if (components.has_component<Energy>(entity)) {
            components.remove_component<Energy>(entity);
        }
        if (components.has_component<Renderable>(entity)) {
            components.remove_component<Renderable>(entity);
        }
        if (components.has_component<SpriteBase>(entity)) {
            components.remove_component<SpriteBase>(entity);
        }
        if (components.has_component<Position>(entity)) {
            components.remove_component<Position>(entity);
        }

        // Award XP
        if (enemy_type && killer != 0 && xp_system && enemy_data) {
            int xp_reward = xp_system->get_xp_for_kill(enemy_type->enemy_id, enemy_data);
            if (xp_reward > 0) {
                xp_system->award_xp(components, killer, xp_reward);
            }
        }

        // Drop loot if this was an enemy (use the copied values!)
        if (enemy_type && death_x >= 0 && death_y >= 0 && world && enemy_data) {
            const EnemyDefinition* def = enemy_data->get_enemy(enemy_type->enemy_id);
            if (def) {
                drop_loot(*def, death_x, death_y);
            }
        }
    }

private:
    // Roll loot based on enemy's loot table
    void drop_loot(const EnemyDefinition& enemy, int x, int y) {
        std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);

        // Drop gold
        if (chance_dist(rng) < enemy.loot_table.gold.chance) {
            std::uniform_int_distribution<int> gold_dist(
                enemy.loot_table.gold.min,
                enemy.loot_table.gold.max
            );
            int gold_amount = gold_dist(rng);

            if (gold_amount > 0) {
                spawn_gold(x, y, gold_amount);

                if (message_log) {
                    message_log->add_success(enemy.name + " drops " +
                        std::to_string(gold_amount) + " gold!");
                }
            }
        }

        // Drop items
        for (const ItemDrop& item_drop : enemy.loot_table.items) {
            if (chance_dist(rng) < item_drop.chance) {
                spawn_item(x, y, item_drop.item_type, item_drop.quantity);

                if (message_log) {
                    message_log->add_success(enemy.name + " drops " + item_drop.item_type + "!");
                }
            }
        }
    }

    void spawn_gold(int x, int y, int amount) {
        Entity gold = world->create_entity();
        world->add_component(gold, Position{ x, y });
        world->add_component(gold, Name{ "Gold" });
        world->add_component(gold, Item{ "gold", amount });

        if (sprite_manager->has_sprite("gold")) {
            world->add_component(gold, sprite_manager->create_renderable("gold"));
        }
        else {
            world->add_component(gold, Renderable{ 0, 4, 1, 0 });
        }
    }

    // Friendly names for dropped items that aren't equippable (potions and
    // the like). CombatSystem doesn't hold a ConsumableSystem reference, so
    // this is a small local lookup rather than plumbing one through -- keep
    // it in sync with ConsumableSystem::initialize_consumables() display
    // names if those change.
    std::string display_name_for_item(const std::string& item_type) const {
        static const std::unordered_map<std::string, std::string> names = {
            { "health_potion", "Health Potion" },
            { "greater_health_potion", "Greater Health Potion" },
            { "superior_health_potion", "Superior Health Potion" },
            { "mana_potion", "Mana Potion" },
            { "greater_mana_potion", "Greater Mana Potion" },
            { "cure_poison_potion", "Antidote" },
            { "haste_potion", "Potion of Haste" },
        };
        auto it = names.find(item_type);
        return it != names.end() ? it->second : item_type;
    }

    void spawn_item(int x, int y, const std::string& item_type, int quantity) {
        Entity item = world->create_entity();
        world->add_component(item, Position{ x, y });

        const EquipmentDefinition* eq_def = equipment_db.get_item(item_type);
        world->add_component(item, Name{ eq_def ? eq_def->display_name : display_name_for_item(item_type) });
        world->add_component(item, Item{ item_type, quantity });
        if (eq_def) {
            world->add_component(item, eq_def->item);
        }

        std::string sprite_name = "item." + item_type;
        if (sprite_manager->has_sprite(sprite_name)) {
            world->add_component(item, sprite_manager->create_renderable(sprite_name));
        }
        else {
            world->add_component(item, Renderable{ 0, 5, 1, 0 });
        }
    }
};