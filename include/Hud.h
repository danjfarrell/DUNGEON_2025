#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "Player.h"

class Hud {
public:
    bool loadFont(const std::string& path, int size);
    void render(SDL_Renderer* r, const Player& player);

private:
    TTF_Font* font = nullptr;
    void drawText(SDL_Renderer* r, const std::string& text, int x, int y);
    void renderInventory(SDL_Renderer* r, const Player& player, int originX, int originY);
};

