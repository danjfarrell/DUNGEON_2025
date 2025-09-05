// CHANGES: window size updated to 1280x720 (from 800x600). No other changes.

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include "Game.h"

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[SDL_Init failed] " << SDL_GetError() << "\n";
        return 1;
    }

    if (!TTF_Init()) {
        std::cerr << "[TTF_Init failed] " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer("Dungeon AI", 1280, 720, 0, &window, &renderer)) {
        std::cerr << "[SDL_CreateWindowAndRenderer failed] " << SDL_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Game game(window, renderer);
    game.run();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
