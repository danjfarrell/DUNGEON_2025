#pragma once
#include <vector>
#include <string>
#include "TileSheet.h"
#include "TileAtlas.h"

class Map {
public:
    bool loadFromFile(const std::string& path);
    void render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas);
    const std::vector<std::string>& data() const { return grid_; }

private:
    std::vector<std::string> grid_;
    void drawTile(char c, int x, int y, SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas);
};

