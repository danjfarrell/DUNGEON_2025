// CHANGES since last full view:
// - unchanged: compact bottom overlay with HP + inventory list.
// - TTF_RenderText_Solid uses SDL3_ttf signature (includes length).

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
    SDL_FRect dst{ (float)x,(float)y,(float)surf->w,(float)surf->h };
    SDL_RenderTexture(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_DestroySurface(surf);
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
        drawText(r, std::to_string(i + 1) + ") " + items[i].name, x, y);
        y += 18;
    }
}

void Hud::render(SDL_Renderer* r, const Player& player) {
    int sw = 0, sh = 0; SDL_GetRenderOutputSize(r, &sw, &sh);
    const int hudH = 96, pad = 12, barH = 16, barW = 220;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 20, 20, 20, 200);
    SDL_FRect bg{ 0.f,(float)(sh - hudH),(float)sw,(float)hudH };
    SDL_RenderFillRect(r, &bg);

    SDL_SetRenderDrawColor(r, 180, 30, 30, 255);
    float hpw = (float)std::max(0, std::min(100, player.getHealth())) * (barW / 100.f);
    SDL_FRect hp{ (float)pad,(float)(sh - hudH + pad),hpw,(float)barH };
    SDL_RenderFillRect(r, &hp);

    drawText(r, "HP: " + std::to_string(player.getHealth()), pad, sh - hudH + pad + barH + 6);

    // inventory to the right of the HP bar
    renderInventory(r, player, pad + barW + 24, sh - hudH + pad);
}
