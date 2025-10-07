#include <SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow(
        "SDL3 Test",
        800, 600,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer) {
        std::cout << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "SDL3 is working! Window created successfully." << std::endl;

    // Simple game loop - show window for 3 seconds
    bool running = true;
    SDL_Event event;
    Uint64 start_time = SDL_GetTicks();

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Clear screen to blue
        SDL_SetRenderDrawColor(renderer, 0, 100, 200, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        // Close after 3 seconds automatically (or you can close the window)
        if (SDL_GetTicks() - start_time > 3000) {
            running = false;
        }

        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Shutting down..." << std::endl;

    return 0;
}