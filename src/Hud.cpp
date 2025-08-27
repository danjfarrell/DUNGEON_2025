#include "Hud.h"

bool Hud::loadFont(const std::string& path, int size) {
    font = TTF_OpenFont(path.c_str(), size);
    return font != nullptr;
}

void Hud::drawText(SDL_Renderer* r, const std::string& text, int x, int y) {
    if (!font) return;
    SDL_Color white{ 255,255,255,255 };
    SDL_Surface* surf = TTF_RenderText_Solid(font, text.c_str(), text.size(), white);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FRect dst{ (float)x, (float)y, (float)surf->w, (float)surf->h };
    SDL_RenderTexture(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_DestroySurface(surf);
}

void Hud::render(SDL_Renderer* r, const Player& player) {
    // panel
    SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
    SDL_FRect hud{ 0.f, 160.f, 800.f, 440.f };
    SDL_RenderFillRect(r, &hud);

    // HP bar
    SDL_SetRenderDrawColor(r, 200, 0, 0, 255);
    SDL_FRect hp{ 20.f, 170.f, (float)(player.getHealth() * 2), 16.f };
    SDL_RenderFillRect(r, &hp);

    drawText(r, "HP: " + std::to_string(player.getHealth()), 20, 190);

    renderInventory(r, player);
}


void Hud::renderInventory(SDL_Renderer* r, const Player& p) {
    const auto& items = p.inventory().items();
    int x = 20, y = 220;
    for (int i = 0; i < (int)items.size(); ++i) {
        drawText(r, std::to_string(i + 1) + ": " + items[i].name, x, y);
        y += 20;
    }
}