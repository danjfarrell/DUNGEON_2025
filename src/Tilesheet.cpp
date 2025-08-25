#include "TileSheet.h"
#include <SDL3_image/SDL_image.h>

bool TileSheet::load(SDL_Renderer* r, const std::string& path) {
    SDL_Surface* surf = IMG_Load(path.c_str());      // SDL3_image: no IMG_Init()
    if (!surf) return false;
    tex_ = SDL_CreateTextureFromSurface(r, surf);
    SDL_DestroySurface(surf);
    return tex_ != nullptr;
}

void TileSheet::renderTile(int tileX, int tileY, int dstX, int dstY, SDL_Renderer* r) {
    if (!tex_) return;
    SDL_FRect src{
    (float)(tileX * TILE_WIDTH),
    (float)(tileY * TILE_HEIGHT),
    (float)TILE_WIDTH,
    (float)TILE_HEIGHT
    };
    SDL_FRect dst{ (float)dstX, (float)dstY, (float)TILE_WIDTH, (float)TILE_HEIGHT };
    SDL_RenderTexture(r, tex_, &src, &dst);
}

void TileSheet::cleanup() {
    if (tex_) { SDL_DestroyTexture(tex_); tex_ = nullptr; }
}
