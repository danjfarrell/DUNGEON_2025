#include <SDL3/SDL.h>
#include <iostream>
#include "ecs/Entity.h"
#include "ecs/ComponentArray.h"
#include "ecs/ComponentManager.h"
#include "ecs/World.h"
#include "ecs/System.h"
#include "components/Components.h"  // Add this at the top



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

void test_component_manager() {
    ComponentManager cm;

    // Create some test components
    struct Position { int x, y; };
    struct Health { int current, max; };

    Entity player = 1;
    Entity enemy = 2;

    // Add different components to different entities
    cm.add_component(player, Position{ 10, 20 });
    cm.add_component(player, Health{ 100, 100 });

    cm.add_component(enemy, Position{ 50, 50 });
    // Enemy has no Health component

    // Test retrieval
    Position* player_pos = cm.get_component<Position>(player);
    std::cout << "Player position: (" << player_pos->x << ", " << player_pos->y << ")" << std::endl;

    // Test has_component
    std::cout << "Player has Health: " << (cm.has_component<Health>(player) ? "Yes" : "No") << std::endl;
    std::cout << "Enemy has Health: " << (cm.has_component<Health>(enemy) ? "Yes" : "No") << std::endl;

    // Test modification
    player_pos->x += 5;
    std::cout << "After moving, player x: " << cm.get_component<Position>(player)->x << std::endl;

    std::cout << "ComponentManager test passed!" << std::endl;
}

// Simple test system
class TestSystem : public System {
public:
    void update(ComponentManager& components, float dt) override {
        std::cout << "TestSystem::update called with dt=" << dt << std::endl;
    }
};




void test_world() {
    World world;

    // Create entities with real components
    Entity player = world.create_entity();
    world.add_component(player, Position{ 10, 10 });
    world.add_component(player, Renderable{ {255, 255, 0, 255}, '@', 10 });
    world.add_component(player, Health{ 100, 100 });
    world.add_component(player, PlayerControlled{});
    world.add_component(player, Name{ "Hero" });

    Entity goblin = world.create_entity();
    world.add_component(goblin, Position{ 20, 15 });
    world.add_component(goblin, Renderable{ {0, 255, 0, 255}, 'g', 5 });
    world.add_component(goblin, Health{ 30, 30 });
    world.add_component(goblin, AI{ AI::AGGRESSIVE, player });
    world.add_component(goblin, BlocksMovement{});
    world.add_component(goblin, Name{ "Goblin" });

    // Test access
    Position* pos = world.get_component<Position>(player);
    Name* name = world.get_component<Name>(player);

    std::cout << name->name << " is at (" << pos->x << ", " << pos->y << ")" << std::endl;
    std::cout << "Player is player-controlled: "
        << (world.has_component<PlayerControlled>(player) ? "Yes" : "No") << std::endl;
    std::cout << "Goblin is player-controlled: "
        << (world.has_component<PlayerControlled>(goblin) ? "Yes" : "No") << std::endl;

    std::cout << "Component test passed!" << std::endl;
}

int main(int argc, char* argv[]) {
    
    test_entity_manager();  // Add this line
    test_component_array();
    test_component_manager();
    test_world();  // Add this line

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