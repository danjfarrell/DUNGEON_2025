// src/components/MagicComponents.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Spell definition
struct Spell {
    std::string id;
    std::string name;
    std::string description;
    int tier;
    int mana_cost;
    int damage;
    int heal_amount;
    int range;
    bool is_aoe;
    int aoe_radius;
    
    Spell(const std::string& spell_id = "", const std::string& spell_name = "",
          const std::string& desc = "", int t = 1, int cost = 5)
        : id(spell_id), name(spell_name), description(desc), tier(t), 
          mana_cost(cost), damage(0), heal_amount(0), range(0),
          is_aoe(false), aoe_radius(0) {}
};

// Mana pool component
struct Mana {
    int current;
    int maximum;
    int regen_per_turn;
    
    Mana(int max = 20, int regen = 5)
        : current(max), maximum(max), regen_per_turn(regen) {}
    
    bool can_cast(int cost) const {
        return current >= cost;
    }
    
    void spend(int cost) {
        current -= cost;
        if (current < 0) current = 0;
    }
    
    void restore(int amount) {
        current += amount;
        if (current > maximum) current = maximum;
    }
    
    void regenerate() {
        restore(regen_per_turn);
    }
    
    float get_percentage() const {
        return static_cast<float>(current) / maximum;
    }
};

// Known spells
struct SpellBook {
    std::vector<std::string> known_spells;
    int equipped_slots[5];
    
    SpellBook() {
        for (int i = 0; i < 5; i++) {
            equipped_slots[i] = -1;
        }
    }
    
    void learn_spell(const std::string& spell_id) {
        for (const auto& known : known_spells) {
            if (known == spell_id) return;
        }
        known_spells.push_back(spell_id);
    }
    
    bool knows_spell(const std::string& spell_id) const {
        for (const auto& known : known_spells) {
            if (known == spell_id) return true;
        }
        return false;
    }
    
    void equip_to_slot(int slot, int spell_index) {
        if (slot >= 0 && slot < 5) {
            equipped_slots[slot] = spell_index;
        }
    }
    
    std::string get_spell_in_slot(int slot) const {
        if (slot >= 0 && slot < 5 && equipped_slots[slot] >= 0 
            && equipped_slots[slot] < static_cast<int>(known_spells.size())) {
            return known_spells[equipped_slots[slot]];
        }
        return "";
    }
};

// Intelligence stat
struct Intelligence {
    int value;
    
    Intelligence(int val = 10) : value(val) {}
    
    int get_max_mana_bonus() const {
        return value * 5;
    }
};