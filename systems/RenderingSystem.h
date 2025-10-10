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

// Visual representation
struct Renderable {
    SDL_Color color;
    char glyph;
    int layer;

    Renderable(SDL_Color c = { 255, 255, 255, 255 }, char g = '?', int l = 0)
        : color(c), glyph(g), layer(l) {
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