#include <SDL3/SDL.h>
#include <iostream>
#include "ecs/World.h"
#include "components/Components.h"
#include "systems/RenderSystem.h"
#include "systems/SpriteUpdateSystem.h"
#include "graphics/SpriteManager.h"

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Roguelike - Tileset Version",
        800, 600,
        0
    );

    if (!window) {
        std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer) {
        std::cout << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create sprite manager
    SpriteManager sprite_manager(renderer, 16, 16);

    // Load configuration - this now loads BOTH the config AND all sprite sheets!
    if (!sprite_manager.load_config("assets/sprites.json")) {
        std::cout << "Failed to load sprite configuration!" << std::endl;
        std::cout << "Make sure sprites.json is in: output/assets/" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create the world and add systems
    World world;
    world.add_system<SpriteUpdateSystem>(&sprite_manager);
    world.add_system<RenderSystem>(&sprite_manager, 2);

    // Create player with directional sprites
    Entity player = world.create_entity();
    world.add_component(player, Position{ 10, 10 });
    world.add_component(player, SpriteBase{ "player", "south" });
    world.add_component(player, Facing{ Facing::SOUTH });
    world.add_component(player, sprite_manager.create_renderable("player.south"));
    world.add_component(player, PlayerControlled{});
    world.add_component(player, BlocksMovement{});
    world.add_component(player, Health{ 100, 100 });
    world.add_component(player, Name{ "Hero" });

    // Create some goblins
    Entity goblin1 = world.create_entity();
    world.add_component(goblin1, Position{ 15, 12 });
    world.add_component(goblin1, SpriteBase{ "goblin", "idle" });
    world.add_component(goblin1, sprite_manager.create_renderable("goblin.idle"));
    world.add_component(goblin1, BlocksMovement{});
    world.add_component(goblin1, Health{ 30, 30 });
    world.add_component(goblin1, Name{ "Goblin" });

    Entity goblin2 = world.create_entity();
    world.add_component(goblin2, Position{ 8, 8 });
    world.add_component(goblin2, SpriteBase{ "goblin", "idle" });
    world.add_component(goblin2, sprite_manager.create_renderable("goblin.idle"));
    world.add_component(goblin2, BlocksMovement{});
    world.add_component(goblin2, Health{ 30, 30 });
    world.add_component(goblin2, Name{ "Goblin" });

    // Create floor tiles
    for (int y = 5; y < 15; y++) {
        for (int x = 5; x < 20; x++) {
            Entity floor = world.create_entity();
            world.add_component(floor, Position{ x, y });
            world.add_component(floor, sprite_manager.create_renderable("floor.stone"));
        }
    }

    // Create walls (stone walls)
    for (int x = 5; x < 20; x++) {
        Entity wall_top = world.create_entity();
        world.add_component(wall_top, Position{ x, 5 });
        world.add_component(wall_top, sprite_manager.create_renderable("wall.stone"));
        world.add_component(wall_top, BlocksMovement{});

        Entity wall_bottom = world.create_entity();
        world.add_component(wall_bottom, Position{ x, 14 });
        world.add_component(wall_bottom, sprite_manager.create_renderable("wall.stone"));
        world.add_component(wall_bottom, BlocksMovement{});
    }

    for (int y = 6; y < 14; y++) {
        Entity wall_left = world.create_entity();
        world.add_component(wall_left, Position{ 5, y });
        world.add_component(wall_left, sprite_manager.create_renderable("wall.stone"));
        world.add_component(wall_left, BlocksMovement{});

        Entity wall_right = world.create_entity();
        world.add_component(wall_right, Position{ 19, y });
        world.add_component(wall_right, sprite_manager.create_renderable("wall.stone"));
        world.add_component(wall_right, BlocksMovement{});
    }

    // Create a potion
    Entity potion = world.create_entity();
    world.add_component(potion, Position{ 12, 10 });
    world.add_component(potion, sprite_manager.create_renderable("potion.small"));
    world.add_component(potion, Name{ "Health Potion" });

    // Create a sword
    Entity sword = world.create_entity();
    world.add_component(sword, Position{ 14, 11 });
    world.add_component(sword, sprite_manager.create_renderable("sword"));
    world.add_component(sword, Name{ "Iron Sword" });

    // Game loop
    bool running = true;
    SDL_Event event;

    std::cout << "\nControls:" << std::endl;
    std::cout << "  Arrow Keys - Move player & change facing direction" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }

                // Get player components
                Position* pos = world.get_component<Position>(player);
                Facing* facing = world.get_component<Facing>(player);

                if (pos && facing) {
                    int new_x = pos->x;
                    int new_y = pos->y;

                    // Update direction and calculate new position
                    switch (event.key.key) {
                    case SDLK_UP:
                        facing->dir = Facing::NORTH;
                        new_y -= 1;
                        break;
                    case SDLK_DOWN:
                        facing->dir = Facing::SOUTH;
                        new_y += 1;
                        break;
                    case SDLK_LEFT:
                        facing->dir = Facing::WEST;
                        new_x -= 1;
                        break;
                    case SDLK_RIGHT:
                        facing->dir = Facing::EAST;
                        new_x += 1;
                        break;
                    }

                    // Simple collision detection
                    bool can_move = true;
                    auto* blockers = world.get_component_manager().get_array<BlocksMovement>();
                    auto* positions = world.get_component_manager().get_array<Position>();

                    if (blockers && positions) {
                        auto& blocker_entities = blockers->get_entities();
                        for (Entity blocker : blocker_entities) {
                            if (blocker == player) continue;

                            Position* blocker_pos = positions->get(blocker);
                            if (blocker_pos && blocker_pos->x == new_x && blocker_pos->y == new_y) {
                                can_move = false;
                                break;
                            }
                        }
                    }

                    // Move if valid
                    if (can_move) {
                        pos->x = new_x;
                        pos->y = new_y;
                    }
                }
            }
        }

        // Clear screen to black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update all systems (sprite update, then render)
        world.update(0.016f);

        // Present the frame
        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // ~60 FPS
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}