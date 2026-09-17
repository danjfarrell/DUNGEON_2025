// src/systems/MagicSystem.h
#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../components/MagicComponents.h"
#include "../magic/SpellDatabase.h"
#include "../ui/MessageLog.h"
#include "../world/Map.h"
#include "StatusEffectHelpers.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class MagicSystem : public System {
private:
    SpellDatabase spell_db;
    MessageLog* message_log;
    Map* game_map;
    
public:
    MagicSystem(MessageLog* log, Map* map)
        : message_log(log), game_map(map) {}

    // game_map was stored at construction time (level 1's map) and never
    // updated when the player changes depth -- harmless while nothing
    // dereferenced it, but blink now does (see cast_blink()). Called from
    // LevelTransitionSystem::update_all_map_pointers() alongside the other
    // systems that hold a Map*.
    void set_map(Map* new_map) { game_map = new_map; }
    
    void update(ComponentManager& components, float dt) override {
        // Mana regen is per-TURN (see Mana::regen_per_turn), not per rendered frame.
        // World::update() is called every render frame (Game::render()) as well as
        // once per turn (Game::update()), so regenerating here made mana refill up
        // to ~60x too fast. Regeneration now happens explicitly via
        // regenerate_mana(), called exactly once per turn from Game::update().
    }

    // Called once per turn (from Game::update(), when the enemy turn finishes)
    // rather than every render frame.
    void regenerate_mana(ComponentManager& components) {
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
        if (spell->id == "haste") {
            StatusEffects::apply_haste(components, caster);
            if (message_log) {
                message_log->add_success("You feel incredibly fast!");
            }
            return;
        }

        if (spell->id == "stone_skin") {
            StatusEffects::apply_stone_skin(components, caster);
            if (message_log) {
                message_log->add_success("Your skin turns to stone!");
            }
            return;
        }

        if (spell->id == "blink") {
            cast_blink(components, caster, spell);
            return;
        }

        if (spell->id == "detect_enemies") {
            cast_detect_enemies(components, caster);
            return;
        }

        if (message_log) {
            message_log->add_info(spell->name + " effect!");
        }
    }

    // Teleports the caster to a random reachable tile within spell->range
    // tiles (walkable per Map::is_walkable(), and not occupied by another
    // BlocksMovement entity). Doesn't require a facing direction -- the
    // player's Facing component is set once at spawn and never updated by
    // InputController (see CLAUDE.md), so it can't be trusted for "blink
    // forward"; "short random hop" matches the spell's own flavor text
    // ("Teleport a short distance") just as well.
    void cast_blink(ComponentManager& components, Entity caster, const Spell* spell) {
        Position* pos = components.get_component<Position>(caster);
        if (!pos || !game_map) {
            if (message_log) {
                message_log->add_warning("You can't find anywhere to blink to.");
            }
            return;
        }

        int radius = spell->range > 0 ? spell->range : 5;

        std::vector<std::pair<int, int>> candidates;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx == 0 && dy == 0) continue;
                if (dx * dx + dy * dy > radius * radius) continue;

                int tx = pos->x + dx;
                int ty = pos->y + dy;
                if (!game_map->is_walkable(tx, ty)) continue;
                if (is_tile_occupied(components, tx, ty)) continue;

                candidates.emplace_back(tx, ty);
            }
        }

        if (candidates.empty()) {
            if (message_log) {
                message_log->add_warning("There's nowhere safe to blink to!");
            }
            return;
        }

        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        auto [new_x, new_y] = candidates[dist(rng)];

        pos->x = new_x;
        pos->y = new_y;

        if (message_log) {
            message_log->add_success("You blink through space!");
        }
    }

    // Reports nearby living enemies (anything with EnemyType) by name,
    // distance, and rough compass direction. Doesn't touch TileVisibility --
    // this is "sense", not "see": it doesn't reveal map tiles, just tells
    // the caster where things are, closest first.
    void cast_detect_enemies(ComponentManager& components, Entity caster) {
        Position* caster_pos = components.get_component<Position>(caster);
        auto* positions = components.get_array<Position>();
        auto* combat_stats = components.get_array<CombatStats>();

        if (!caster_pos || !positions || !combat_stats) {
            if (message_log) {
                message_log->add_info("You sense nothing nearby.");
            }
            return;
        }

        struct Detected {
            std::string name;
            float distance;
            int dx, dy;
        };
        std::vector<Detected> found;

        for (Entity target : positions->get_entities()) {
            if (target == caster) continue;
            if (!components.has_component<EnemyType>(target)) continue;

            Position* target_pos = positions->get(target);
            CombatStats* target_stats = combat_stats->get(target);
            if (!target_pos || !target_stats || !target_stats->is_alive()) continue;

            int dx = target_pos->x - caster_pos->x;
            int dy = target_pos->y - caster_pos->y;
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

            Name* name = components.get_component<Name>(target);
            found.push_back({ name ? name->name : "Something", dist, dx, dy });
        }

        if (!message_log) return;

        if (found.empty()) {
            message_log->add_info("You sense no enemies nearby.");
            return;
        }

        std::sort(found.begin(), found.end(),
            [](const Detected& a, const Detected& b) { return a.distance < b.distance; });

        message_log->add_success("You sense " + std::to_string(found.size()) +
            (found.size() == 1 ? " enemy nearby:" : " enemies nearby:"));

        const size_t shown = std::min<size_t>(found.size(), 5);
        for (size_t i = 0; i < shown; i++) {
            const Detected& d = found[i];
            message_log->add_info("  " + d.name + " - " +
                std::to_string(static_cast<int>(d.distance)) + " tiles " +
                compass_direction(d.dx, d.dy));
        }
        if (found.size() > shown) {
            message_log->add_info("  ...and " + std::to_string(found.size() - shown) + " more.");
        }
    }

    std::string compass_direction(int dx, int dy) const {
        std::string ns = dy < 0 ? "north" : (dy > 0 ? "south" : "");
        std::string ew = dx > 0 ? "east" : (dx < 0 ? "west" : "");
        if (ns.empty() && ew.empty()) return "right on top of you";
        return ns + ew;
    }

    bool is_tile_occupied(ComponentManager& components, int x, int y) const {
        auto* positions = components.get_array<Position>();
        auto* blockers = components.get_array<BlocksMovement>();
        if (!positions || !blockers) return false;

        for (Entity entity : blockers->get_entities()) {
            Position* p = positions->get(entity);
            if (p && p->x == x && p->y == y) return true;
        }
        return false;
    }
};