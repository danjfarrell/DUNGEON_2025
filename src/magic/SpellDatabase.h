// src/magic/SpellDatabase.h
#pragma once

#include "../components/MagicComponents.h"
#include <unordered_map>

class SpellDatabase {
private:
    std::unordered_map<std::string, Spell> spells;
    
public:
    SpellDatabase() {
        initialize_spells();
    }
    
    const Spell* get_spell(const std::string& spell_id) const {
        auto it = spells.find(spell_id);
        if (it != spells.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    std::vector<std::string> get_all_spell_ids() const {
        std::vector<std::string> ids;
        for (const auto& pair : spells) {
            ids.push_back(pair.first);
        }
        return ids;
    }
    
private:
    void initialize_spells() {
        // Tier 1 (5 MP)
        {
            Spell spell("magic_missile", "Magic Missile", 
                       "Fires a magical bolt at an enemy.", 1, 5);
            spell.damage = 8;
            spell.range = 10;
            spells["magic_missile"] = spell;
        }
        
        {
            Spell spell("minor_heal", "Minor Heal",
                       "Restore a small amount of health.", 1, 5);
            spell.heal_amount = 10;
            spell.range = 0;
            spells["minor_heal"] = spell;
        }
        
        {
            Spell spell("detect_enemies", "Detect Enemies",
                       "Reveal nearby enemies.", 1, 5);
            spell.range = 0;
            spells["detect_enemies"] = spell;
        }
        
        // Tier 2 (10 MP)
        {
            Spell spell("lightning_bolt", "Lightning Bolt",
                       "A bolt of lightning.", 2, 10);
            spell.damage = 15;
            spell.range = -1;
            spells["lightning_bolt"] = spell;
        }
        
        {
            Spell spell("blink", "Blink",
                       "Teleport a short distance.", 2, 10);
            spell.range = 5;
            spells["blink"] = spell;
        }
        
        {
            Spell spell("cure_wounds", "Cure Wounds",
                       "Restore moderate health.", 2, 10);
            spell.heal_amount = 25;
            spell.range = 0;
            spells["cure_wounds"] = spell;
        }
        
        // Tier 3 (15 MP)
        {
            Spell spell("fireball", "Fireball",
                       "Explosive ball of fire!", 3, 15);
            spell.damage = 25;
            spell.range = 8;
            spell.is_aoe = true;
            spell.aoe_radius = 2;
            spells["fireball"] = spell;
        }
        
        {
            Spell spell("haste", "Haste",
                       "Increases speed temporarily.", 3, 15);
            spell.range = 0;
            spells["haste"] = spell;
        }
        
        {
            Spell spell("serious_heal", "Serious Healing",
                       "Restore large health.", 3, 15);
            spell.heal_amount = 50;
            spell.range = 0;
            spells["serious_heal"] = spell;
        }
        
        // Tier 4 (20 MP)
        {
            Spell spell("chain_lightning", "Chain Lightning",
                       "Lightning that jumps between enemies.", 4, 20);
            spell.damage = 30;
            spell.range = 10;
            spells["chain_lightning"] = spell;
        }
        
        {
            Spell spell("stone_skin", "Stone Skin",
                       "Increases defense greatly.", 4, 20);
            spell.range = 0;
            spells["stone_skin"] = spell;
        }
        
        // Tier 5 (30 MP)
        {
            Spell spell("meteor", "Meteor Storm",
                       "Call down destruction!", 5, 30);
            spell.damage = 50;
            spell.range = 0;
            spell.is_aoe = true;
            spell.aoe_radius = 5;
            spells["meteor"] = spell;
        }
        
        {
            Spell spell("full_heal", "Complete Restoration",
                       "Restore all health.", 5, 30);
            spell.heal_amount = 9999;
            spell.range = 0;
            spells["full_heal"] = spell;
        }
    }
};