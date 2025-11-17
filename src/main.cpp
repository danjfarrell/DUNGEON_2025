#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <functional>
#include "ecs/World.h"
#include "components/Components.h"
#include "systems/RenderSystem.h"
#include "systems/SpriteUpdateSystem.h"
#include "systems/Camera.h"
#include "graphics/SpriteManager.h"
#include "world/Map.h"
#include "world/MapGenerators.h"
#include "systems/MapRenderSystem.h"
#include "utils/Logger.h"
#include "ui/MessageLog.h" 
#include "ui/UILayout.h"  // NEW




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
    
    if (!TTF_Init()) {  // NEW
        LOG_ERROR("TTF_Init failed: " + std::string(SDL_GetError()));
        SDL_Quit();
        return 1;
    }
    LOG_INFO("TTF initialized successfully");

    //const int SCREEN_WIDTH = 800;
    //const int SCREEN_HEIGHT = 600;
    //  INCREASED RESOLUTION OPTIONS:
    // Option 1: 1280x720 (HD)
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;
    // Option 2: 1920x1080 (Full HD) - Uncomment to use
    // const int SCREEN_WIDTH = 1920;
    // const int SCREEN_HEIGHT = 1080;

    // Option 3: 1600x900 (Good middle ground) - Uncomment to use
    // const int SCREEN_WIDTH = 1600;
    // const int SCREEN_HEIGHT = 900;


    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Roguelike - Tileset Version",
        SCREEN_WIDTH, SCREEN_HEIGHT,
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

    //  Create UI Layout with new resolution
    UILayout ui_layout(SCREEN_WIDTH, SCREEN_HEIGHT);

    LOG_INFO("UI Layout calculated for " + std::to_string(SCREEN_WIDTH) + "x" +
        std::to_string(SCREEN_HEIGHT) + ":");
    LOG_INFO("  Game viewport: " + std::to_string(ui_layout.game_viewport.w) + "x" +
        std::to_string(ui_layout.game_viewport.h));
    LOG_INFO("  Minimap: " + std::to_string(ui_layout.minimap.w) + "x" +
        std::to_string(ui_layout.minimap.h));
    LOG_INFO("  Top bar: " + std::to_string(ui_layout.top_bar.h) + "px");
    LOG_INFO("  Message log: " + std::to_string(ui_layout.message_log.h) + "px");


    // Create sprite manager
    SpriteManager sprite_manager(renderer, 16, 16);

    // Load configuration - this now loads BOTH the config AND all sprite sheets!
    if (!sprite_manager.load_config("assets/sprites.json")) {
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

    // NEW: Create camera
    const int TILE_SIZE = 16 * 2;  // tile_width * scale
    //Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT,
    //    game_map.get_width(), game_map.get_height(),
    //    TILE_SIZE);
    Camera camera(
        ui_layout.game_viewport.w,
        ui_layout.game_viewport.h,
        game_map.get_width(),
        game_map.get_height(),
        TILE_SIZE
    );
    // Create message log using layout dimensions
    MessageLog message_log(
        renderer,
        ui_layout.message_log.x,
        ui_layout.message_log.y,
        ui_layout.message_log.w,
        ui_layout.message_log.h
    );

    // NEW: Create message log (bottom of screen)
    //const int LOG_HEIGHT = 150;
   // MessageLog message_log(renderer, 10, SCREEN_HEIGHT - LOG_HEIGHT - 10,
   //     SCREEN_WIDTH - 20, LOG_HEIGHT);

    // Try to load a font - provide fallback paths
    bool font_loaded = false;
    std::vector<std::string> font_paths = {
        "assets/fonts/DejaVuSansMono.ttf",
        "C:/Windows/Fonts/consola.ttf",  // Windows
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",  // Linux
        "/System/Library/Fonts/Monaco.dfont"  // macOS
    };

    for (const auto& path : font_paths) {
        if (message_log.init_font(path, 14)) {
            LOG_INFO("Loaded font: " + path);
            font_loaded = true;
            break;
        }
    }

    if (!font_loaded) {
        LOG_WARN("Could not load any font - message log will not render");
        // Continue anyway - game still playable
    }

    // Create world and add systems
    World world;
    //world.add_system<SpriteUpdateSystem>(&sprite_manager);
    //world.add_system<MapRenderSystem>(&sprite_manager, &game_map, 2);  // Map first
    //auto* map_render_system = world.add_system<MapRenderSystem>(&sprite_manager, &game_map, 2);

    //world.add_system<RenderSystem>(&sprite_manager, 2);                // Entities on top
    world.add_system<SpriteUpdateSystem>(&sprite_manager);
    auto* map_render_system = world.add_system<MapRenderSystem>(
        &sprite_manager, &game_map, 2, &camera, &ui_layout);  // Pass camera
    auto* render_system = world.add_system<RenderSystem>(
        &sprite_manager, 2, &camera, &ui_layout);  // Pass camera


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
        // NEW: Center camera on player at start
        camera.center_on(first_room->center_x(), first_room->center_y());
    }
    else {
        world.add_component(player, Position{ 6, 6 });
        camera.center_on(6, 6);
    }
    world.add_component(player, SpriteBase{ "player", "south" });
    world.add_component(player, Facing{ Facing::SOUTH });
    world.add_component(player, sprite_manager.create_renderable("player.south"));
    world.add_component(player, PlayerControlled{});
    world.add_component(player, BlocksMovement{});

    // NEW: Welcome message
    message_log.add_success("Welcome to the dungeon!");
    message_log.add_info("Use arrow keys to move. Press ESC to quit.");


    // Spawn goblins in rooms
    const auto& rooms = game_map.get_rooms();
    int goblin_count = 0;
    for (size_t i = 1; i < rooms.size() && i < 4; i++) {
        Entity goblin = world.create_entity();
        world.add_component(goblin, Position{ rooms[i].center_x(), rooms[i].center_y() });
        world.add_component(goblin, sprite_manager.create_renderable("goblin.idle"));
        world.add_component(goblin, BlocksMovement{});
        goblin_count++;
    }

    message_log.add_warning("You sense " + std::to_string(goblin_count) +
        " goblins lurking in the darkness...");

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
                        TileType tile = game_map.get_tile(new_x, new_y);
                        if (tile == TileType::WALL) {
                            message_log.add_info("You bump into a wall.");
                        }
                        else if (tile == TileType::DOOR_CLOSED) {
                            message_log.add_warning("The door is locked.");
                        }
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
                                // NEW: Message when bumping into entity
                                message_log.add_combat("You bump into a goblin! (Combat not yet implemented)");
                                can_move = false;
                                break;
                            }
                        }
                    }

                    // Move if valid
                    if (can_move) {
                        pos->x = new_x;
                        pos->y = new_y;
                        // NEW: Update camera to follow player
                        camera.center_on(new_x, new_y);

                        // NEW: Advance turn and occasionally add flavor text
                        message_log.next_turn();

                        // Random flavor messages (10% chance)
                        if (rand() % 10 == 0) {
                            const char* flavor[] = {
                                "Your footsteps echo in the dungeon.",
                                "You hear water dripping somewhere.",
                                "A cold breeze brushes past you.",
                                "The torch flickers."
                            };
                            message_log.add_lore(flavor[rand() % 4]);
                        }
                        // OR for smooth following:
                        // camera.smooth_follow(new_x, new_y, 0.15f);
                    }
                }
            }
        }

        // Clear screen to black
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);

        // Update all systems (sprite update, then render)
        //world.update(0.016f);

        // Reset viewport to full screen for UI
        //SDL_SetRenderViewport(renderer, nullptr);

        //  Draw UI panel backgrounds

        // Top bar background
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_FRect top_bar_rect = {
            static_cast<float>(ui_layout.top_bar.x),
            static_cast<float>(ui_layout.top_bar.y),
            static_cast<float>(ui_layout.top_bar.w),
            static_cast<float>(ui_layout.top_bar.h)
        };
        SDL_RenderFillRect(renderer, &top_bar_rect);

        // Top bar border
        SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
        SDL_RenderRect(renderer, &top_bar_rect);

        //  Minimap background (placeholder - will use actual minimap later)
        SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
        SDL_FRect minimap_rect = {
            static_cast<float>(ui_layout.minimap.x),
            static_cast<float>(ui_layout.minimap.y),
            static_cast<float>(ui_layout.minimap.w),
            static_cast<float>(ui_layout.minimap.h)
        };
        SDL_RenderFillRect(renderer, &minimap_rect);

        // Minimap border
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
        SDL_RenderRect(renderer, &minimap_rect);

        // Minimap label (temporary text - replace with actual minimap)
        // TODO: Add actual Minimap rendering here

        // Render game world (systems already add viewport offset)
        world.update(0.016f);

        message_log.render();
        // NEW: DEBUG - Render colored borders around all UI sections
        ui_layout.render_debug_borders(renderer);

        // NEW: DEBUG - Optionally render labels (requires font)
        if (font_loaded) {
            // You'll need to expose the font from MessageLog or create a separate debug font
            // For now, just borders without labels is helpful
            TTF_Font* ui_font = TTF_OpenFont("assets/fonts/DejaVuSansMono.ttf", 14);
            if (!ui_font) {
                ui_font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 14);
            }

            if (ui_font) {
                ui_layout.render_placeholders(renderer, ui_font);
                TTF_CloseFont(ui_font);
            }
        }
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