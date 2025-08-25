#pragma once
#include <SDL3/SDL.h>
#include <string>

constexpr int TILE_WIDTH = 16;
constexpr int TILE_HEIGHT = 16;

class TileSheet {
public:
    bool load(SDL_Renderer* r, const std::string& path);
    void renderTile(int tileX, int tileY, int dstX, int dstY, SDL_Renderer* r);
    void cleanup();

private:
    SDL_Texture* tex_ = nullptr;
};

