// src/systems/ExperienceSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../components/MagicComponents.h"
#include "../ui/MessageLog.h"
#include "MagicSystem.h"
#include <string>
#include <unordered_map>

struct LevelUpBonus {
    int hp_increase;
    int attack_increase;
    int defense_increase;

    LevelUpBonus(int hp = 5, int atk = 1, int def = 1)
        : hp_increase(hp), attack_increase(atk), defense_increase(def) {}
};

class ExperienceSystem : public System {
private:
    MessageLog* message_log;
    MagicSystem* magic_system = nullptr;  // set post-construction; see set_magic_system()

public:
    ExperienceSystem(MessageLog* log) : message_log(log) {}

    // MagicSystem doesn't exist yet when ExperienceSystem is constructed
    // (see GameBootstrap.cpp's system construction order), so it's wired in
    // afterward rather than taken as a constructor argument.
    void set_magic_system(MagicSystem* ms) { magic_system = ms; }

    void update(ComponentManager& components, float dt) override {
        // Called manually via award_xp()
    }

    void award_xp(ComponentManager& components, Entity entity, int xp_amount) {
        Experience* exp = components.get_component<Experience>(entity);
        if (!exp) return;

        if (message_log) {
            message_log->add_success("You gain " + std::to_string(xp_amount) + " XP!");
        }

        bool leveled_up = exp->add_xp(xp_amount);

        if (leveled_up) {
            handle_level_up(components, entity);
        }
    }

    void handle_level_up(ComponentManager& components, Entity entity) {
        Experience* exp = components.get_component<Experience>(entity);
        CombatStats* stats = components.get_component<CombatStats>(entity);
        
        if (!exp || !stats) return;

        exp->level_up();

        LevelUpBonus bonus;
        
        stats->max_hp += bonus.hp_increase;
        stats->current_hp += bonus.hp_increase;
        stats->attack += bonus.attack_increase;
        stats->defense += bonus.defense_increase;

        if (message_log) {
            message_log->add_success("*** LEVEL UP! ***");
            message_log->add_success("You are now level " + std::to_string(exp->level) + "!");
            message_log->add_info("HP +" + std::to_string(bonus.hp_increase) + 
                                " ATK +" + std::to_string(bonus.attack_increase) +
                                " DEF +" + std::to_string(bonus.defense_increase));
            message_log->add_info("All wounds healed!");
        }

        grant_level_up_spell(components, entity, exp->level);
    }

    int get_xp_for_kill(const std::string& enemy_id, class EnemyDataManager* enemy_data) {
        if (!enemy_data) return 0;

        const struct EnemyDefinition* def = enemy_data->get_enemy(enemy_id);
        if (def) {
            return def->xp_reward;
        }
        return 0;
    }

private:
    // Level -> spell learned at that level. The player starts knowing
    // magic_missile/minor_heal (see GameInitializer::init_player_spells);
    // this is the path to the other 11 spells in SpellDatabase. Only the
    // first three (levels 2-4) land in an open SpellBook hotbar slot --
    // there are 5 slots total and 2 are already full at game start, and
    // there's no spell-slot-management UI yet to reassign them, so spells
    // learned after that are known but not directly castable until that UI
    // exists (Phase 2/3 follow-up).
    void grant_level_up_spell(ComponentManager& components, Entity entity, int new_level) {
        if (!magic_system) return;

        static const std::unordered_map<int, std::string> level_spells = {
            { 2, "lightning_bolt" },
            { 3, "cure_wounds" },
            { 4, "haste" },
            { 5, "detect_enemies" },
            { 6, "fireball" },
            { 7, "blink" },
            { 8, "serious_heal" },
            { 9, "chain_lightning" },
            { 10, "stone_skin" },
            { 11, "meteor" },
            { 12, "full_heal" },
        };

        auto it = level_spells.find(new_level);
        if (it == level_spells.end()) return;

        const std::string& spell_id = it->second;
        magic_system->learn_spell(components, entity, spell_id);

        SpellBook* spellbook = components.get_component<SpellBook>(entity);
        if (!spellbook) return;

        for (int slot = 0; slot < 5; slot++) {
            if (!spellbook->get_spell_in_slot(slot).empty()) continue;

            for (size_t i = 0; i < spellbook->known_spells.size(); i++) {
                if (spellbook->known_spells[i] == spell_id) {
                    spellbook->equip_to_slot(slot, static_cast<int>(i));
                    if (message_log) {
                        message_log->add_info("It's ready in slot " +
                            std::to_string(slot + 1) + " (Shift+" +
                            std::to_string(slot + 1) + ")!");
                    }
                    break;
                }
            }
            break;
        }
    }
};