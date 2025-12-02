// src/systems/MagicSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../components/MagicComponents.h"
#include "../magic/SpellDatabase.h"
#include "../ui/MessageLog.h"
#include "../world/Map.h"
#include <cmath>

class MagicSystem : public System {
private:
    SpellDatabase spell_db;
    MessageLog* message_log;
    Map* game_map;
    
public:
    MagicSystem(MessageLog* log, Map* map) 
        : message_log(log), game_map(map) {}
    
    void update(ComponentManager& components, float dt) override {
        auto* mana_components = components.get_array<Mana>();
        if (!mana_components) return;
        
        auto& mana_data = mana_components->get_components();
        for (auto& mana : mana_data) {
            mana.regenerate();
        }
    }
    
    bool cast_spell(ComponentManager& components, Entity caster, int slot) {
        SpellBook* spellbook = components.get_component<SpellBook>(caster);
        Mana* mana = components.get_component<Mana>(caster);
        
        if (!spellbook || !mana) return false;
        
        std::string spell_id = spellbook->get_spell_in_slot(slot);
        if (spell_id.empty()) {
            if (message_log) {
                message_log->add_warning("No spell in that slot!");
            }
            return false;
        }
        
        return cast_spell_by_id(components, caster, spell_id);
    }
    
    bool cast_spell_by_id(ComponentManager& components, Entity caster, 
                          const std::string& spell_id) {
        const Spell* spell = spell_db.get_spell(spell_id);
        Mana* mana = components.get_component<Mana>(caster);
        Position* pos = components.get_component<Position>(caster);
        
        if (!spell || !mana || !pos) return false;
        
        if (!mana->can_cast(spell->mana_cost)) {
            if (message_log) {
                message_log->add_warning("Not enough mana! (" + 
                    std::to_string(mana->current) + "/" + 
                    std::to_string(spell->mana_cost) + ")");
            }
            return false;
        }
        
        mana->spend(spell->mana_cost);
        
        if (spell->heal_amount > 0) {
            cast_heal_spell(components, caster, spell);
        } else if (spell->damage > 0) {
            cast_damage_spell(components, caster, spell);
        } else {
            cast_utility_spell(components, caster, spell);
        }
        
        if (message_log) {
            message_log->add_info("You cast " + spell->name + "!");
        }
        
        return true;
    }
    
    void learn_spell(ComponentManager& components, Entity learner, 
                     const std::string& spell_id) {
        SpellBook* spellbook = components.get_component<SpellBook>(learner);
        if (!spellbook) return;
        
        const Spell* spell = spell_db.get_spell(spell_id);
        if (!spell) return;
        
        if (spellbook->knows_spell(spell_id)) {
            if (message_log) {
                message_log->add_info("You already know " + spell->name + ".");
            }
            return;
        }
        
        spellbook->learn_spell(spell_id);
        
        if (message_log) {
            message_log->add_success("You learned " + spell->name + "!");
            message_log->add_info("Tier " + std::to_string(spell->tier) + 
                                 " | Cost: " + std::to_string(spell->mana_cost) + " MP");
        }
    }
    
    const SpellDatabase& get_spell_database() const {
        return spell_db;
    }
    
private:
    void cast_heal_spell(ComponentManager& components, Entity caster, 
                        const Spell* spell) {
        CombatStats* stats = components.get_component<CombatStats>(caster);
        if (!stats) return;
        
        int heal = spell->heal_amount;
        if (heal == 9999) heal = stats->max_hp;
        
        stats->heal(heal);
        
        if (message_log) {
            message_log->add_success("Restored " + std::to_string(heal) + " HP!");
        }
    }
    
    void cast_damage_spell(ComponentManager& components, Entity caster,
                          const Spell* spell) {
        Position* caster_pos = components.get_component<Position>(caster);
        if (!caster_pos) return;
        
        auto* positions = components.get_array<Position>();
        auto* combat_stats = components.get_array<CombatStats>();
        
        if (!positions || !combat_stats) return;
        
        auto& entities = positions->get_entities();
        int targets_hit = 0;
        
        for (Entity target : entities) {
            if (target == caster) continue;
            
            Position* target_pos = positions->get(target);
            CombatStats* target_stats = combat_stats->get(target);
            
            if (!target_pos || !target_stats) continue;
            if (!target_stats->is_alive()) continue;
            
            int dx = target_pos->x - caster_pos->x;
            int dy = target_pos->y - caster_pos->y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            bool in_range = false;
            if (spell->is_aoe) {
                in_range = (dist <= spell->aoe_radius);
            } else if (spell->range == -1) {
                in_range = true;
            } else {
                in_range = (dist <= spell->range);
            }
            
            if (in_range) {
                target_stats->take_damage(spell->damage);
                targets_hit++;
                
                if (!target_stats->is_alive()) {
                    if (message_log) {
                        Name* name = components.get_component<Name>(target);
                        std::string target_name = name ? name->name : "Enemy";
                        message_log->add_combat(target_name + " is destroyed!");
                    }
                }
            }
        }
        
        if (message_log && targets_hit > 0) {
            message_log->add_combat("Hit " + std::to_string(targets_hit) + 
                                   " enemies for " + std::to_string(spell->damage) + 
                                   " damage!");
        }
    }
    
    void cast_utility_spell(ComponentManager& components, Entity caster,
                           const Spell* spell) {
        if (message_log) {
            message_log->add_info(spell->name + " effect!");
        }
    }
};