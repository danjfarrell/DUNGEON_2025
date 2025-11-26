#pragma once

#include "../ecs/Entity.h"
#include <string>
#include <SDL3/SDL.h>

// Position in the game world (tile coordinates)
struct Position {
    int x;
    int y;

    Position(int x = 0, int y = 0) : x(x), y(y) {}
};

// Visual representation using sprite tiles
struct Renderable {
    int sheet_id;    // Which sprite sheet to use
    int tile_x;      // Column in the sprite sheet (0-based)
    int tile_y;      // Row in the sprite sheet (0-based)
    int layer;       // Draw order (higher = on top)

    Renderable(int sheet = 0, int tx = 0, int ty = 0, int l = 0)
        : sheet_id(sheet), tile_x(tx), tile_y(ty), layer(l) {
    }
};

// Sprite base name and variant (for changing sprites)
struct SpriteBase {
    std::string base_name;  // e.g., "player", "goblin", "wall"
    std::string variant;    // e.g., "north", "ice", "attack"

    SpriteBase(const std::string& base = "", const std::string& var = "")
        : base_name(base), variant(var) {
    }

    // Helper to get full sprite name
    std::string get_full_name() const {
        if (variant.empty()) {
            return base_name;
        }
        return base_name + "." + variant;
    }

    // Change variant
    void set_variant(const std::string& new_variant) {
        variant = new_variant;
    }
};

// Direction the entity is facing
struct Facing {
    enum Direction { NORTH, SOUTH, EAST, WEST };
    Direction dir;

    Facing(Direction d = SOUTH) : dir(d) {}

    // Helper to get direction as string
    std::string to_string() const {
        switch (dir) {
        case NORTH: return "north";
        case SOUTH: return "south";
        case EAST:  return "east";
        case WEST:  return "west";
        default:    return "south";
        }
    }
};

// Health points
struct Health {
    int current;
    int maximum;

    Health(int current = 10, int maximum = 10)
        : current(current), maximum(maximum) {
    }
};

// AI behavior
struct AI {
    enum Type { AGGRESSIVE, DEFENSIVE, PATROL, IDLE };

    Type type;
    Entity target;

    AI(Type type = IDLE, Entity target = 0)
        : type(type), target(target) {
    }
};

// Tag: This entity is player-controlled
struct PlayerControlled {
    // Tag component - no data needed
};

// Tag: This entity blocks movement
struct BlocksMovement {
    // Tag component
};

// Entity name
struct Name {
    std::string name;

    Name(const std::string& n = "") : name(n) {}
};

// Combat statistics
struct CombatStats {
    int attack;
    int defense;
    int max_hp;
    int current_hp;

    CombatStats(int atk = 5, int def = 0, int hp = 10)
        : attack(atk), defense(def), max_hp(hp), current_hp(hp) {
    }

    bool is_alive() const {
        return current_hp > 0;
    }

    void take_damage(int damage) {
        current_hp -= damage;
        if (current_hp < 0) current_hp = 0;
    }

    void heal(int amount) {
        current_hp += amount;
        if (current_hp > max_hp) current_hp = max_hp;
    }
};

// Energy system for turn-based gameplay
// When energy >= 100, entity can act
struct Energy {
    int current;
    int speed;  // How much energy gained per turn tick

    Energy(int spd = 100) : current(0), speed(spd) {
    }

    bool can_act() const {
        return current >= 100;
    }

    void consume_turn() {
        current -= 100;
    }

    void gain_energy() {
        current += speed;
    }
};

// Tag: This entity is dead and should be removed
struct Dead {
    // Tag component
};

// Marks entities that need to act this turn (for AI)
struct WantsToAct {
    // Tag component
};

// Marks an entity as an item that can be picked up
struct Item {
    std::string item_type;  // "gold", "potion", "weapon", etc.
    int quantity;           // How many (for stackable items like gold)

    Item(const std::string& type = "unknown", int qty = 1)
        : item_type(type), quantity(qty) {
    }
};

// Player's inventory - stores list of item entities
struct Inventory {
    std::vector<Entity> items;
    int gold;  // Quick access to gold count

    Inventory() : gold(0) {}

    void add_item(Entity item_entity) {
        items.push_back(item_entity);
    }

    bool has_item(Entity item_entity) const {
        return std::find(items.begin(), items.end(), item_entity) != items.end();
    }

    void remove_item(Entity item_entity) {
        auto it = std::find(items.begin(), items.end(), item_entity);
        if (it != items.end()) {
            items.erase(it);
        }
    }
};


// NEW: Store enemy type ID for loot/xp lookup
struct EnemyType {
    std::string enemy_id;  // e.g., "goblin", "orc", "rat"

    EnemyType(const std::string& id = "") : enemy_id(id) {}
};