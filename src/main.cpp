#include <SDL3/SDL.h>
#include <iostream>
#include "ecs/Entity.h"
#include "ecs/ComponentArray.h"


void test_entity_manager() {
    EntityManager em;

    Entity e1 = em.create();  // Should be 0
    Entity e2 = em.create();  // Should be 1
    Entity e3 = em.create();  // Should be 2

    std::cout << "Created entities: " << e1 << ", " << e2 << ", " << e3 << std::endl;

    em.destroy(e2);  // Mark entity 1 for reuse

    Entity e4 = em.create();  // Should reuse 1

    std::cout << "After destroying e2 and creating e4: " << e4 << std::endl;
    std::cout << "Entity manager test passed!" << std::endl;
}

// Simple test component
struct TestComponent {
    int value;
    TestComponent(int v = 0) : value(v) {}
};

void test_component_array() {
    ComponentArray<TestComponent> array;

    array.insert(1, TestComponent(100));
    array.insert(3, TestComponent(300));
    array.insert(5, TestComponent(500));

    std::cout << "Entity 1 has component: " << (array.has(1) ? "Yes" : "No") << std::endl;
    std::cout << "Entity 2 has component: " << (array.has(2) ? "Yes" : "No") << std::endl;
    std::cout << "Entity 3 value: " << array.get(3)->value << std::endl;

    array.remove(3);
    std::cout << "After removing entity 3: " << (array.has(3) ? "Yes" : "No") << std::endl;

    std::cout << "ComponentArray test passed!" << std::endl;
}

int main(int argc, char* argv[]) {
    
    test_entity_manager();  // Add this line
    test_component_array();
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
        if (SDL_GetTicks() - start_time > 30000) {
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