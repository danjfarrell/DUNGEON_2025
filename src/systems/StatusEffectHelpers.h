// src/systems/StatusEffectHelpers.h
// Shared apply logic for Poisoned/Hasted so CombatSystem (poison on hit),
// ConsumableSystem (potions), and MagicSystem (the haste spell) don't each
// reimplement it slightly differently.
#pragma once

#include "../components/Components.h"
#include "../ecs/ComponentManager.h"

namespace StatusEffects {

constexpr int POISON_DEFAULT_DAMAGE = 2;
constexpr int POISON_DEFAULT_DURATION = 3;   // turns
constexpr int HASTE_DURATION_TURNS = 5;      // turns
constexpr float HASTE_SPEED_MULTIPLIER = 1.5f;
constexpr int STONE_SKIN_DURATION_TURNS = 6; // turns
constexpr int STONE_SKIN_DEFENSE_BONUS = 8;

// Applies (or refreshes the duration of) Poisoned on target.
inline void apply_poison(ComponentManager& components, Entity target,
    int damage_per_turn = POISON_DEFAULT_DAMAGE,
    int duration = POISON_DEFAULT_DURATION) {
    Poisoned* existing = components.get_component<Poisoned>(target);
    if (existing) {
        existing->turns_remaining = duration;  // refresh, don't stack damage
        return;
    }
    components.add_component(target, Poisoned{ damage_per_turn, duration });
}

// Applies (or refreshes the duration of) Hasted on target, boosting
// Energy::speed for the duration. Requires an Energy component.
inline void apply_haste(ComponentManager& components, Entity target,
    int duration = HASTE_DURATION_TURNS) {
    Energy* energy = components.get_component<Energy>(target);
    if (!energy) return;

    Hasted* existing = components.get_component<Hasted>(target);
    if (existing) {
        existing->turns_remaining = duration;  // refresh, don't re-stack speed
        return;
    }

    components.add_component(target, Hasted{ energy->speed, duration });
    energy->speed = static_cast<int>(energy->speed * HASTE_SPEED_MULTIPLIER);
}

// Applies (or refreshes the duration of) StoneSkin on target, boosting
// CombatStats::defense for the duration. Requires a CombatStats component.
inline void apply_stone_skin(ComponentManager& components, Entity target,
    int bonus = STONE_SKIN_DEFENSE_BONUS,
    int duration = STONE_SKIN_DURATION_TURNS) {
    CombatStats* stats = components.get_component<CombatStats>(target);
    if (!stats) return;

    StoneSkin* existing = components.get_component<StoneSkin>(target);
    if (existing) {
        existing->turns_remaining = duration;  // refresh, don't re-stack defense
        return;
    }

    components.add_component(target, StoneSkin{ stats->defense, duration });
    stats->defense += bonus;
}

}  // namespace StatusEffects
