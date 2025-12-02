// src/systems/ExperienceSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../ui/MessageLog.h"
#include <string>

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

public:
    ExperienceSystem(MessageLog* log) : message_log(log) {}

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
    }

    int get_xp_for_kill(const std::string& enemy_id, class EnemyDataManager* enemy_data) {
        if (!enemy_data) return 0;

        const struct EnemyDefinition* def = enemy_data->get_enemy(enemy_id);
        if (def) {
            return def->xp_reward;
        }
        return 0;
    }
};