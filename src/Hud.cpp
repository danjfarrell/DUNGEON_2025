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
    //// panel
    //SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
    //SDL_FRect hud{ 0.f, 160.f, 800.f, 440.f };
    //SDL_RenderFillRect(r, &hud);

    //// HP bar
    //SDL_SetRenderDrawColor(r, 200, 0, 0, 255);
    //SDL_FRect hp{ 20.f, 170.f, (float)(player.getHealth() * 2), 16.f };
    //SDL_RenderFillRect(r, &hp);

    //drawText(r, "HP: " + std::to_string(player.getHealth()), 20, 190);
    // Query output size so HUD anchors to bottom
    int sw = 0, sh = 0;
    SDL_GetRenderOutputSize(r, &sw, &sh);

    const int hudH = 72;                 // compact height
    const int pad = 12;
    const int barH = 16;
    const int barW = 200;

    // Enable alpha and draw a translucent bar background
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 20, 20, 20, 200);
    SDL_FRect bg{ 0.f, (float)(sh - hudH), (float)sw, (float)hudH };
    SDL_RenderFillRect(r, &bg);

    // HP bar
    SDL_SetRenderDrawColor(r, 180, 30, 30, 255);
    float hpw = (float)std::max(0, std::min(100, player.getHealth())) * (barW / 100.f);
    SDL_FRect hp{ (float)pad, (float)(sh - hudH + pad), hpw, (float)barH };
    SDL_RenderFillRect(r, &hp);

    // Text
    drawText(r, "HP: " + std::to_string(player.getHealth()),
        pad, sh - hudH + pad + barH + 6);


    // Inventory list (to the right of HP bar)
    renderInventory(r, player, pad + barW + 24, sh - hudH + pad);
}


void Hud::renderInventory(SDL_Renderer* r, const Player& player, int originX, int originY) {
    const auto& items = player.inventory().items();

    int x = originX;
    int y = originY;

    if (items.empty()) {
        drawText(r, "Inventory: (empty)", x, y);
        return;
    }

    drawText(r, "Inventory:", x, y);
    y += 20;

    for (int i = 0; i < (int)items.size(); ++i) {
        const std::string line =
            std::to_string(i + 1) + ") " + items[i].name;
        drawText(r, line, x, y);
        y += 18;
    }
}