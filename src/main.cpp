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
#include "ui/Minimap.h"
#include "world/TileVisibility.h"  // ADD THIS INCLUD
#include "ui/HealthBar.h"
#include "systems/TurnManager.h"
#include "systems/CombatSystem.h"
#include "systems/AISystem.h"
#include "systems/DeathSystem.h"
#include "systems/ItemPickupSystem.h"  // NEW!


// Add this helper function to main.cpp (before main() function)
// Renders HP text in the top bar

void render_hp_display(SDL_Renderer* renderer, TTF_Font* font,
    int current_hp, int max_hp, int x, int y) {
    if (!font) return;

    std::string hp_text = "HP: " + std::to_string(current_hp) + "/" + std::to_string(max_hp);

    SDL_Color color = { 255, 255, 255, 255 };

    // Change color based on HP percentage
    float hp_percent = static_cast<float>(current_hp) / max_hp;
    if (hp_percent < 0.25f) {
        color = { 255, 50, 50, 255 };  // Red (critical)
    }
    else if (hp_percent < 0.50f) {
        color = { 255, 200, 50, 255 };  // Yellow (warning)
    }
    else {
        color = { 100, 255, 100, 255 };  // Green (healthy)
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, hp_text.c_str(), 0, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dest = {
                static_cast<float>(x),
                static_cast<float>(y),
                static_cast<float>(surface->w),
                static_cast<float>(surface->h)
            };
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
}


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

   

    // Create health bar (using same font as message log)
    HealthBar health_bar(
        renderer,
        message_log.get_font(),  // Need to add get_font() to MessageLog
        20,                      // x position (top-left area)
        20,                      // y position
        250,                     // width
        35                       // height
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

    // After creating message_log and loading its font:
    TTF_Font* ui_font = nullptr;
    for (const auto& path : font_paths) {
        ui_font = TTF_OpenFont(path.c_str(), 20);  // Larger font for UI
        if (ui_font) {
            LOG_INFO("Loaded UI font: " + path);
            break;
        }
    }


    // Create world and add systems
    World world;

    // Initialize visibility system
    world.initialize_tile_visibility(game_map.get_width(), game_map.get_height());
    TileVisibility* tile_vis = world.get_tile_visibility();

    // Option 1: Reveal all for testing (comment out for real fog of war)
    tile_vis->reveal_all();

    // Option 2: Start with everything unexplored (uncomment for real fog)
    // tile_vis will start unexplored by default

    // ========================================

    // Create turn manager
    TurnManager turn_manager(&message_log);

    // Add combat system (doesn't run in update loop, called manually)
    // IMPORTANT: Pass world and sprite_manager now!
    auto* combat_system = new CombatSystem(&message_log, &world, &sprite_manager);
    

    // Add systems to world
    world.add_system<AISystem>(&game_map, combat_system);  // NEW!
    world.add_system<ItemPickupSystem>(&message_log);  // NEW! Runs every frame
    world.add_system<DeathSystem>();  // NEW!

    world.add_system<SpriteUpdateSystem>(&sprite_manager);
    auto* map_render_system = world.add_system<MapRenderSystem>(
        &sprite_manager, &game_map, 2, &camera, &ui_layout, tile_vis);  // Pass camera
    auto* render_system = world.add_system<RenderSystem>(
        &sprite_manager, 2, &camera, &ui_layout);  // Pass camera

    // Create minimap
    Minimap minimap(
        renderer,
        &game_map,
        &world,
        ui_layout.minimap.x,
        ui_layout.minimap.y,
        ui_layout.minimap.w,
        ui_layout.minimap.h
    );
    // Temporary: reveal all tiles (remove this once FOV is implemented)
    minimap.reveal_all();

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
        tile_vis->update_fov(first_room->center_x(), first_room->center_y(), 10);
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
    // Add Health component to player (if not already added):
    world.add_component(player, Health{ 100, 100 });  // 100/100 HP
    world.add_component(player, Name{ "Player" });  // NEW!
    world.add_component(player, CombatStats{ 5, 1, 30 });  // NEW! (attack=5, defense=1, hp=30)
    world.add_component(player, Energy{ 100 });  // NEW!
    world.add_component(player, Inventory{});  // NEW!

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
        world.add_component(goblin, Name{ "Goblin" });  // NEW!
        world.add_component(goblin, CombatStats{ 3, 0, 10 });  // NEW! (attack=3, defense=0, hp=10)
        world.add_component(goblin, AI{ AI::AGGRESSIVE });  // NEW!
        world.add_component(goblin, Energy{ 100 });  // NEW!
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
                // Optional: Add keybind to toggle fog
                // In input handling:
                if (event.key.key == SDLK_F) {
                    minimap.set_show_fog(!minimap.get_show_fog());
                    message_log.add_info(minimap.get_show_fog() ?
                        "Fog of war enabled" : "Fog of war disabled");
                }
                //if (event.key.key == SDLK_D) {
                //    auto* hp = world.get_component<Health>(player);
                //    if (hp && hp->current > 0) {
                //        hp->current -= 10;
                //        health_bar.trigger_flash();  // Flash effect
                //        message_log.add_combat("You take 10 damage! (Test)");

                //        if (hp->current <= 0) {
                //            hp->current = 0;
                //            message_log.add_combat("You died!");
                //        }
                //    }
                //}

                // Only allow player input during player turn
                if (!turn_manager.is_player_turn()) {
                    continue;
                }

                // Get player components
                Position* pos = world.get_component<Position>(player);
                Facing* facing = world.get_component<Facing>(player);
                Energy* player_energy = world.get_component<Energy>(player);

                
                //if (pos && facing) {
                if (pos && facing && player_energy) {
                    int new_x = pos->x;
                    int new_y = pos->y;
                    bool tried_to_move = false;

                    // Then check entity collision as before
                    // Update direction and calculate new position
                    switch (event.key.key) {
                    case SDLK_UP:
                        facing->dir = Facing::NORTH;
                        new_y -= 1;
                        tried_to_move = true;
                        break;
                    case SDLK_DOWN:
                        facing->dir = Facing::SOUTH;
                        new_y += 1;
                        tried_to_move = true;
                        break;
                    case SDLK_LEFT:
                        facing->dir = Facing::WEST;
                        new_x -= 1;
                        tried_to_move = true;
                        break;
                    case SDLK_RIGHT:
                        facing->dir = Facing::EAST;
                        new_x += 1;
                        tried_to_move = true;
                        break;
                    }

                    if (!tried_to_move) continue;

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
                    //bool can_move = true;
                    //bool attacked = false;
                    //auto* blockers = world.get_component_manager().get_array<BlocksMovement>();
                    //auto* positions = world.get_component_manager().get_array<Position>();
                    // Check for entity collision (might be an enemy to attack!)
                    bool attacked = false;
                    auto* blockers = world.get_component_manager().get_array<BlocksMovement>();
                    auto* positions = world.get_component_manager().get_array<Position>();

                    if (blockers && positions) {
                        auto& blocker_entities = blockers->get_entities();
                        for (Entity blocker : blocker_entities) {
                            if (blocker == player) continue;

                            Position* blocker_pos = positions->get(blocker);
                            if (blocker_pos && blocker_pos->x == new_x && blocker_pos->y == new_y) {
                                // Try to attack this entity!
                                if (combat_system->try_attack(world.get_component_manager(), player, blocker)) {
                                    attacked = true;
                                    player_energy->consume_turn();
                                    turn_manager.end_player_turn();
                                }
                                break;
                            }
                        }
                    }




                    if (blockers && positions) {
                        auto& blocker_entities = blockers->get_entities();
                        for (Entity blocker : blocker_entities) {
                            if (blocker == player) continue;

                            Position* blocker_pos = positions->get(blocker);
                            if (blocker_pos && blocker_pos->x == new_x && blocker_pos->y == new_y) {
                                // NEW: Message when bumping into entity
                                //message_log.add_combat("You bump into a goblin! (Combat not yet implemented)");
                                //can_move = false;
                                attacked = true;
                                player_energy->consume_turn();
                                turn_manager.end_player_turn();

                                break;
                            }
                        }
                    }

                    // Move if valid
                    //if (can_move) {
                    if (!attacked) {
                        pos->x = new_x;
                        pos->y = new_y;

                        player_energy->consume_turn();
                        turn_manager.end_player_turn();

                        // NEW: Update camera to follow player
                        camera.center_on(new_x, new_y);

                        // Update minimap center
                        minimap.center_on(new_x, new_y);

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
                        tile_vis->update_fov(new_x, new_y, 10);  // Update vis
                        // OR for smooth following:
                        // camera.smooth_follow(new_x, new_y, 0.15f);
                    }
                }
            }
        }

        // Process enemy turns
        if (turn_manager.is_enemy_turn()) {
            turn_manager.process_turn(world);
            world.update(0.016f);  // This runs AISystem
            turn_manager.end_enemy_turn();
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

        // Display player HP
        CombatStats* player_stats = world.get_component<CombatStats>(player);
        if (player_stats && ui_font) {
            render_hp_display(renderer, ui_font,
                player_stats->current_hp,
                player_stats->max_hp,
                ui_layout.top_bar.x + 20,
                ui_layout.top_bar.y + 25);
        }
        // NEW: Display gold count
        Inventory* player_inventory = world.get_component<Inventory>(player);
        if (player_inventory && ui_font) {
            std::string gold_text = "Gold: " + std::to_string(player_inventory->gold);
            SDL_Color gold_color = { 255, 215, 0, 255 };  // Gold color!

            SDL_Surface* surface = TTF_RenderText_Blended(ui_font, gold_text.c_str(), 0, gold_color);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    SDL_FRect dest = {
                        static_cast<float>(ui_layout.top_bar.x + 250),  // To the right of HP
                        static_cast<float>(ui_layout.top_bar.y + 25),
                        static_cast<float>(surface->w),
                        static_cast<float>(surface->h)
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &dest);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
        }



        // Check for player death
        if (player_stats && !player_stats->is_alive()) {
            message_log.add_warning("YOU DIED!");
            message_log.add_info("Press ESC to quit.");
            // You could add a game over state here
        }


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
        // Render game world (only during player turn to avoid flickering)
        if (turn_manager.is_player_turn()) {
            world.update(0.016f);
        }

        message_log.render();
        // NEW: DEBUG - Render colored borders around all UI sections
        ui_layout.render_debug_borders(renderer);
        
        // Render health bar
        auto* player_health = world.get_component<Health>(player);
        if (player_health) {
            //health_bar.render(player_health->current, player_health->maximum);
        }


        // ADD NEW MINIMAP RENDER:
        minimap.render();  // Draws the actual minimap with tiles!

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

    // ============================================================================
    // And cleanup at end:
    // ============================================================================
    if (ui_font) {
        TTF_CloseFont(ui_font);
    }

    // Cleanup
    delete combat_system;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}