// CHANGES since last full view:
// - loadFromFile: trims CR, pads ragged rows to max width (safety).
// - render: uses per-row width, origin offsets, and guards against OOB.
// - items render honors origin and camera.

#include "Map.h"
#include <fstream>
#include <algorithm>

bool Map::loadFromFile(const std::string& path) {
    grid_.clear();

    std::ifstream f(path);
    if (!f.is_open()) {
        return false;
    }

    std::string line;
    size_t maxW = 0;

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        grid_.push_back(line);
        maxW = std::max(maxW, line.size());
    }

    if (grid_.empty()) {
        return false;
    }

    for (auto& row : grid_) {
        if (row.size() < maxW) {
            row.append(maxW - row.size(), '#');
        }
    }

    return true;
}

void Map::drawTile(char c,
    int sx,
    int sy,
    SDL_Renderer* r,
    TileSheet& tiles,
    const TileAtlas& a) {
    int tx = a.floorX;
    int ty = a.floorY;

    if (c == '#') {
        tx = a.wallX;
        ty = a.wallY;
    }
    else if (c == 'D') {
        tx = a.doorX;
        ty = a.doorY;
    }

    tiles.renderTile(tx, ty, sx, sy, r);
}

void Map::render(SDL_Renderer* r,
    TileSheet& tiles,
    const TileAtlas& atlas,
    const Camera& cam,
    int originX,
    int originY) {
    if (grid_.empty()) {
        return;
    }

    int y0 = std::max(0, cam.y);
    int y1 = std::min(height(), cam.y + cam.hTiles + 1);

    for (int y = y0; y < y1; ++y) {
        const int rowW = (int)grid_[y].size();
        if (rowW == 0) {
            continue;
        }

        int x0 = std::max(0, cam.x);
        int x1 = std::min(rowW, cam.x + cam.wTiles + 1);

        for (int x = x0; x < x1; ++x) {
            int sx = originX + (x - cam.x) * int(TILE_WIDTH * cam.scale);
            int sy = originY + (y - cam.y) * int(TILE_HEIGHT * cam.scale);

            drawTile(grid_[y][x], sx, sy, r, tiles, atlas);
        }
    }

    for (const auto& wi : items_) {
        if (wi.y < 0 || wi.y >= (int)grid_.size()) {
            continue;
        }
        const int rowW = (int)grid_[wi.y].size();
        if (rowW == 0 || wi.x < 0 || wi.x >= rowW) {
            continue;
        }
        if (wi.x < cam.x || wi.y < cam.y) {
            continue;
        }
        if (wi.x >= cam.x + cam.wTiles + 1 || wi.y >= cam.y + cam.hTiles + 1) {
            continue;
        }

        int sx = originX + (wi.x - cam.x) * int(TILE_WIDTH * cam.scale);
        int sy = originY + (wi.y - cam.y) * int(TILE_HEIGHT * cam.scale);

        int tx = 5;
        int ty = 0;

        if (wi.item.type == ItemType::KEY) {
            tx = 6;
        }
        else if (wi.item.type == ItemType::SCROLL) {
            tx = 7;
        }

        tiles.renderTile(tx, ty, sx, sy, r);
    }
}

int Map::itemIndexAt(int x, int y) const {
    for (int i = 0; i < (int)items_.size(); ++i) {
        if (items_[i].x == x && items_[i].y == y) {
            return i;
        }
    }
    return -1;
}

void Map::removeItemAt(int idx) {
    if (idx >= 0 && idx < (int)items_.size()) {
        items_.erase(items_.begin() + idx);
    }
}
