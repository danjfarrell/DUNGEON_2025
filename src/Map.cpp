#include "Map.h"
#include <fstream>

bool Map::loadFromFile(const std::string& path) {
    grid_.clear();

    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    size_t maxW = 0;

    while (std::getline(f, line)) {
        // strip CR if present (Windows CRLF)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        grid_.push_back(line);
        if (line.size() > maxW) maxW = line.size();
    }
    if (grid_.empty()) return false;

    // pad shorter rows with walls so all rows align
    for (auto& row : grid_) {
        if (row.size() < maxW) row.append(maxW - row.size(), '#');
    }
    return true;
}

void Map::drawTile(char c, int sx, int sy, SDL_Renderer* r, TileSheet& tiles, const TileAtlas& a) {
    int tx = a.floorX, ty = a.floorY;
    switch (c) { case '#': tx = a.wallX; ty = a.wallY; break; case 'D': tx = a.doorX; ty = a.doorY; break; }
                         tiles.renderTile(tx, ty, sx, sy, r);
}

void Map::render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas, const Camera& cam) {
    if (grid_.empty()) return;

    int y0 = std::max(0, cam.y);
    int y1 = std::min(height(), cam.y + cam.hTiles + 1);

    for (int y = y0; y < y1; ++y) {
        const int rowW = (int)grid_[y].size();
        if (rowW == 0) continue;

        int x0 = std::max(0, cam.x);
        int x1 = std::min(rowW, cam.x + cam.wTiles + 1);

        for (int x = x0; x < x1; ++x) {
            int sx = (x - cam.x) * int(TILE_WIDTH * cam.scale);
            int sy = (y - cam.y) * int(TILE_HEIGHT * cam.scale);
            drawTile(grid_[y][x], sx, sy, r, tiles, atlas);
        }
    }

    // items (guard each row too)
    for (const auto& wi : items_) {
        if (wi.y < 0 || wi.y >= (int)grid_.size()) continue;
        int rowW = (int)grid_[wi.y].size();
        if (rowW == 0 || wi.x < 0 || wi.x >= rowW) continue;
        if (wi.x < cam.x || wi.y < cam.y || wi.x >= cam.x + cam.wTiles + 1 || wi.y >= cam.y + cam.hTiles + 1) continue;

        int sx = (wi.x - cam.x) * int(TILE_WIDTH * cam.scale);
        int sy = (wi.y - cam.y) * int(TILE_HEIGHT * cam.scale);
        int tx = 5, ty = 0;
        switch (wi.item.type) {
        case ItemType::KEY:    tx = 6; break;
        case ItemType::SCROLL: tx = 7; break;
        default:               tx = 5; break; // Potion as default
        }
        tiles.renderTile(tx, ty, sx, sy, r);
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
