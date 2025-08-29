#pragma once
#include <SDL3/SDL.h>
#include "TileSheet.h"
#include "TileAtlas.h"
#include "Map.h"
#include "Inventory.h"


class Player {
public:
    void setPosition(int tx, int ty) { x = tx; y = ty; }
    void move(int dx, int dy, const Map& map);
    //void render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas) const;
    void render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas, const Camera& cam) const;


    int getX() const { return x; }
    int getY() const { return y; }
    int  getHealth() const { return health; }
    void setHealth(int h) { health = h; }

    Inventory& inventory() { return inv_; }
    const Inventory& inventory() const { return inv_; }

private:
    int x = 1, y = 1;   // tile coords
    int health = 100;

    Inventory inv_;
};

