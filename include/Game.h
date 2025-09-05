// CHANGES since last full view:
// - Added UILayout struct (padding, sidebar, minimap, hud sizes, scale).
// - Added UI compute fields (map area origin/size).
// - Added spell sidebar slots + hover state.
// - Added new draw helpers: drawUIPlaceholders (bg + status), drawMiniMap, drawSpellSidebar.

#pragma once
#include <SDL3/SDL.h>
#include "TileSheet.h"
#include "Map.h"
#include "TileAtlas.h"
#include "Player.h"
#include "Hud.h"
#include "Camera.h"
#include <vector>

struct UILayout {
    int pad = 12;
    int sidebarW = 120;
    int hudH = 96;
    int minimapSize = 200;
    float scale = 2.0f;

    int screenW = 1280;
    int screenH = 720;

    int mapX = 0;
    int mapY = 0;
    int mapW = 0;
    int mapH = 0;

    void compute() {
        mapX = pad;
        mapY = pad;
        mapW = screenW - sidebarW - pad * 2;
        mapH = screenH - hudH - pad * 2;
        if (mapW < 1) mapW = 1;
        if (mapH < 1) mapH = 1;
    }
};

class Game {
public:
    Game(SDL_Window* win, SDL_Renderer* rend);
    ~Game();
    void run();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = false;

    UILayout ui;

    TileSheet tiles;
    Map map;
    TileAtlas atlas;
    Player player;
    Hud hud;
    Camera cam;

    std::vector<SDL_FRect> spellSlots;
    int hoveredSlot = -1;

    void init();
    void handleEvents();
    void update();
    void render();
    void drawUIPlaceholders();
    void drawMiniMap();       // NEW
    void drawSpellSidebar();  // NEW
    void cleanup();
};
