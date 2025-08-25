//#include <SDL3/SDL.h>
//#include <SDL3_image/SDL_image.h>
//#include <SDL3_ttf/SDL_ttf.h>
//#include <iostream>
//#include "Game.h"
//
//int main(int argc, char* argv[]) {
//    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
//        std::cerr << "[SDL Init Error] " << SDL_GetError() << "\n";
//        return 1;
//    }
//
//    if (TTF_Init() == false) {
//        std::cerr << "[TTF Init Error] " << SDL_GetError() << "\n";
//        SDL_Quit();
//        return 1;
//    }
//
//    SDL_Window* window = nullptr;
//    SDL_Renderer* renderer = nullptr;
//    if (SDL_CreateWindowAndRenderer("dungeon ai",800, 600, 0, &window, &renderer) != 0) {
//        std::cerr << "[Window Error] " << SDL_GetError() << "\n";
//        TTF_Quit();
//        SDL_Quit();
//        return 1;
//    }
//
//    Game game(window, renderer);
//    game.run();
//
//    SDL_DestroyRenderer(renderer);
//    SDL_DestroyWindow(window);
//    TTF_Quit();
//    SDL_Quit();
//    return 0;
//}

#include <SDL3/SDL.h>
#include <iostream>

int main() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[SDL_Init dummy failed] " << SDL_GetError() << "\n";
        return 1;
    }
    std::cerr << "SDL_Init OK with dummy.\n";
    SDL_Quit();
    return 0;
}


