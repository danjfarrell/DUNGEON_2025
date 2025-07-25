#pragma once


#include <SDL3/SDL.h>
#include "Input.h"
#include "Renderer.h"
#include "TileSheet.h"
#include "Map.h"
#include "Player.h"
#include "Hud.h"
#include "TileAtlas.h"
#include "Enemy.h"

#include <vector>

class Game {
public:
    Game(SDL_Window* window, SDL_Renderer* renderer);
    ~Game();

    void run();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;

    bool running = false;

    Input input;
    Renderer screen;
    TileSheet tiles;
    Map map;
    Player player;
    Hud hud;
    TileAtlas atlas;
    std::vector<Enemy> enemies;

    void init();
    void handleEvents();
    void update();
    void render();
    void cleanup();
};
