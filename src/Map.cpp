#include "Map.h"
#include <fstream>

bool Map::loadFromFile(const std::string& path) {
    grid_.clear();
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) grid_.push_back(line);
    return !grid_.empty();
}

void Map::drawTile(char c, int x, int y, SDL_Renderer* r, TileSheet& tiles, const TileAtlas& a) {
    int tx = a.floorX, ty = a.floorY;
    switch (c) {
    case '#': tx = a.wallX;  ty = a.wallY;  break;
    case '.': tx = a.floorX; ty = a.floorY; break;
    case 'D': tx = a.doorX;  ty = a.doorY;  break;
    }
    tiles.renderTile(tx, ty, x * TILE_WIDTH, y * TILE_HEIGHT, r);
}

void Map::render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas) {
    for (size_t y = 0; y < grid_.size(); ++y)
        for (size_t x = 0; x < grid_[y].size(); ++x)
            drawTile(grid_[y][x], (int)x, (int)y, r, tiles, atlas);

    // items on top:
    for (const auto& wi : items_) {
        int tx = 5, ty = 0; // default to Potion tile
        if (wi.item.type == ItemType::KEY) { tx = 6; ty = 0; }
        if (wi.item.type == ItemType::SCROLL) { tx = 7; ty = 0; }
        tiles.renderTile(tx, ty, wi.x * TILE_WIDTH, wi.y * TILE_HEIGHT, r);
    }


}

int Map::itemIndexAt(int x, int y) const {
    for (int i = 0; i < (int)items_.size(); ++i)
        if (items_[i].x == x && items_[i].y == y) return i;
    return -1;
}
void Map::removeItemAt(int idx) {
    if (idx >= 0 && idx < (int)items_.size()) items_.erase(items_.begin() + idx);
}
