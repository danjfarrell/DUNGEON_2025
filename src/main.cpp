#include <SDL3/SDL.h>
#include <iostream>
#include <functional>
#include "ecs/World.h"
#include "components/Components.h"
#include "systems/RenderSystem.h"
#include "systems/SpriteUpdateSystem.h"
#include "graphics/SpriteManager.h"
#include "world/Map.h"
#include "world/MapGenerators.h"
#include "systems/MapRenderSystem.h"
#include "utils/Logger.h"




int main(int argc, char* argv[]) {
    
    Logger::get_instance("game_log.txt", LogLevel::DEBUG);

    LOG_INFO("=== Starting Roguelike ===");
    
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        //std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
        LOG_ERROR("SDL_Init failed: " + std::string(SDL_GetError()));
        
        return 1;
    }
    LOG_INFO("SDL initialized successfully");

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Roguelike - Tileset Version",
        800, 600,
        0
    );

    if (!window) {
        //std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        LOG_ERROR("SDL_CreateWindow failed: " + std::string(SDL_GetError()));
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer) {
        //std::cout << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        LOG_ERROR("SDL_CreateRenderer failed: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create sprite manager
    SpriteManager sprite_manager(renderer, 16, 16);

    // Load configuration - this now loads BOTH the config AND all sprite sheets!
    if (!sprite_manager.load_config("assets/sprites.json")) {
        //std::cout << "Failed to load sprite configuration!" << std::endl;
        //std::cout << "Make sure sprites.json is in: output/assets/" << std::endl;
        LOG_ERROR("Failed to load sprite configuration!" + std::string(SDL_GetError()));
        LOG_ERROR("Make sure sprites.json is in: output/assets/" + std::string(SDL_GetError()));
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    sprite_manager.dump_config_to_log();

    // Create map and generate
    Map game_map(50, 30, 12345);
    RoomCorridorGenerator gen(8, 4, 10);
    game_map.generate(gen);

    game_map.dump_to_log();

   


    // Create world and add systems
    World world;
    world.add_system<SpriteUpdateSystem>(&sprite_manager);
    //world.add_system<MapRenderSystem>(&sprite_manager, &game_map, 2);  // Map first
    auto* map_render_system = world.add_system<MapRenderSystem>(&sprite_manager, &game_map, 2);



    world.add_system<RenderSystem>(&sprite_manager, 2);                // Entities on top

    // DEBUG: Detailed tile-by-tile with sprite names
    LOG_INFO("--- Debug: Tile-by-Tile Sprite Resolution ---");
    game_map.dump_with_sprites_to_log(
        [map_render_system](int x, int y, TileType tile) {
            return map_render_system->get_sprite_name_for_tile(x, y, tile);
        },
        50,  // width
        30   // height
    );

    game_map.dump_sprite_grid_to_log(
        [map_render_system](int x, int y, TileType tile) {
            return map_render_system->get_sprite_name_for_tile(x, y, tile);
        },
        0, 0,   // start position
        50, 30  // size
    );


    // Spawn player in first room
    Entity player = world.create_entity();
    const Room* first_room = nullptr;
    if (!game_map.get_rooms().empty()) {
        first_room = &game_map.get_rooms()[0];
        world.add_component(player, Position{ first_room->center_x(), first_room->center_y() });
    }
    else {
        world.add_component(player, Position{ 5, 5 });
    }
    world.add_component(player, SpriteBase{ "player", "south" });
    world.add_component(player, Facing{ Facing::SOUTH });
    world.add_component(player, sprite_manager.create_renderable("player.south"));
    world.add_component(player, PlayerControlled{});
    world.add_component(player, BlocksMovement{});

    // Spawn goblins in rooms
    const auto& rooms = game_map.get_rooms();
    for (size_t i = 1; i < rooms.size() && i < 4; i++) {
        Entity goblin = world.create_entity();
        world.add_component(goblin, Position{ rooms[i].center_x(), rooms[i].center_y() });
        world.add_component(goblin, sprite_manager.create_renderable("goblin.idle"));
        world.add_component(goblin, BlocksMovement{});
    }


    // Game loop
    bool running = true;
    SDL_Event event;

    //std::cout << "\nControls:" << std::endl;
    //std::cout << "\tArrow Keys - Move player & change facing direction" << std::endl;
    //std::cout << "\tESC - Quit" << std::endl;
    LOG_INFO("\nControls:" + std::string(SDL_GetError()));
    LOG_INFO("\tArrow Keys - Move player & change facing direction" + std::string(SDL_GetError()));
    LOG_INFO("\tESC - Quit" + std::string(SDL_GetError()));

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
                    

                    // Then check entity collision as before
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

                    // Before moving, check map
                    if (!game_map.is_walkable(new_x, new_y)) {

                        continue;  // Can't walk through walls
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