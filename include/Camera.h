// CHANGES: none since last full view.

#pragma once
#include <algorithm>
#include "TileSheet.h"

struct Camera {
    int x = 0, y = 0;
    int wTiles = 0, hTiles = 0;
    float scale = 1.0f;

    void init(int screenW, int screenH, float s = 1.0f) {
        scale = s;
        int tw = int(TILE_WIDTH * scale);
        int th = int(TILE_HEIGHT * scale);
        wTiles = std::max(1, screenW / std::max(1, tw));
        hTiles = std::max(1, screenH / std::max(1, th));
    }

    void centerOn(int px, int py, int mapW, int mapH) {
        x = px - wTiles / 2;
        y = py - hTiles / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + wTiles > mapW) x = std::max(0, mapW - wTiles);
        if (y + hTiles > mapH) y = std::max(0, mapH - hTiles);
    }
};
