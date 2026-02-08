// src/core/Game.cpp
// Implementation of Game class methods
// Extracted from main_refactored.cpp for better organization

#include "Game.h"
#include "../utils/Logger.h"
#include "GameInitializer.h"

// ECS and entities
#include "../ecs/World.h"

// Graphics and rendering
#include "../graphics/SpriteManager.h"
#include "../systems/Camera.h"
#include "../systems/MapRenderSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/SpriteUpdateSystem.h"

// World and map
#include "../world/Map.h"
#include "../world/DungeonManager.h"
#include "../world/TileVisibility.h"
#include "../data/EnemyData.h"
#include "../spawning/EnemySpawner.h"

// Systems
#include "../systems/TurnManager.h"
#include "../systems/CombatSystem.h"
#include "../systems/MagicSystem.h"
#include "../systems/ExperienceSystem.h"
#include "../systems/StairSystem.h"
#include "../systems/ConsumableSystem.h"
#include "../systems/AISystem.h"
#include "../systems/ItemPickupSystem.h"
#include "../systems/DeathSystem.h"

// UI
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../ui/HealthBar.h"

// Components
#include "../components/Components.h"

// ============================================================================
// Constructor & Destructor
// ============================================================================

Game::Game()
    : running(false),
    player(0),
    current_map(nullptr),
    combat_system(nullptr),
    magic_system(nullptr),
    xp_system(nullptr),
    stair_system(nullptr),
    consumable_system(nullptr),
    map_render_system(nullptr),
    render_system(nullptr),
    tile_vis(nullptr) {
    // Don't log here - logger not initialized yet!
}

Game::~Game() {
    LOG_INFO("=== Game shutting down ===");
    // RAII handles cleanup automatically via unique_ptrs
    // No manual cleanup needed!
}

// ============================================================================
// Initialization Methods
// ============================================================================

bool Game::initialize() {
    // CRITICAL: Initialize logger FIRST before any LOG_* calls
    Logger::get_instance("game_log.txt", LogLevel::DEBUG);

    LOG_INFO("=== Initializing Game ===");

    // Load configuration
    if (!load_config()) {
        return false;
    }

    // Re-initialize logger with config settings
    Logger::get_instance(config.logging.log_file, config.get_log_level());

    // Initialize SDL and create window (RAII)
    if (!init_resources()) {
        return false;
    }

    // Initialize game managers
    if (!init_managers()) {
        return false;
    }

    // Create UI layout
    init_ui_layout();

    // Initialize sprite system
    auto sprite_result = GameInitializer::init_sprites(*sprite_manager, config);
    if (!sprite_result.success) {
        LOG_ERROR(sprite_result.error_message);
        return false;
    }

    // Load enemy data
    GameInitializer::init_enemy_data(*enemy_data, config);

    // Create world and generate first level
    if (!init_world()) {
        return false;
    }

    // Initialize UI components
    init_ui_components();

    // Create player
    init_player();

    // Spawn enemies
    dungeon_manager->spawn_enemies(*enemy_spawner, *world);

    // Send welcome messages
    GameInitializer::send_welcome_messages(*message_log, config.gameplay.starting_depth);

    LOG_INFO("=== Game initialization complete ===");
    return true;
}

bool Game::load_config() {
    config.load_from_file("assets/config.json");
    // Always succeeds - uses defaults if file not found
    return true;
}

bool Game::init_resources() {
    try {
        resources = std::make_unique<ResourceManager>(
            "Roguelike - Refactored",
            config.display.screen_width,
            config.display.screen_height,
            config.assets.font_path,
            config.assets.font_fallback,
            config.assets.font_size,
            config.assets.title_font_size
        );
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Resource initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool Game::init_managers() {
    sprite_manager = std::make_unique<SpriteManager>(
        resources->renderer.get(),
        config.display.tile_size,
        config.display.tile_size
    );

    enemy_data = std::make_unique<EnemyDataManager>();

    dungeon_manager = std::make_unique<DungeonManager>(
        config.gameplay.map_width,
        config.gameplay.map_height
    );

    enemy_spawner = std::make_unique<EnemySpawner>(
        nullptr,  // Will set world pointer later
        sprite_manager.get(),
        enemy_data.get()
    );

    return true;
}

void Game::init_ui_layout() {
    ui_layout = std::make_unique<UILayout>(
        config.display.screen_width,
        config.display.screen_height
    );

    LOG_INFO("UI Layout: Game viewport " +
        std::to_string(ui_layout->game_viewport.w) + "x" +
        std::to_string(ui_layout->game_viewport.h));
}

bool Game::init_world() {
    world = std::make_unique<World>();

    // Update enemy spawner with world pointer
    enemy_spawner = std::make_unique<EnemySpawner>(
        world.get(),
        sprite_manager.get(),
        enemy_data.get()
    );

    // Generate first level
    current_map = dungeon_manager->generate_level(config.gameplay.starting_depth);

    // Initialize visibility
    world->initialize_tile_visibility(
        current_map->get_width(),
        current_map->get_height()
    );
    tile_vis = world->get_tile_visibility();

    // Create camera
    camera = std::make_unique<Camera>(
        ui_layout->game_viewport.w,
        ui_layout->game_viewport.h,
        current_map->get_width(),
        current_map->get_height(),
        config.display.get_scaled_tile_size()
    );

    // Initialize message log
    message_log = std::make_unique<MessageLog>(
        resources->renderer.get(),
        ui_layout->message_log.x,
        ui_layout->message_log.y,
        ui_layout->message_log.w,
        ui_layout->message_log.h
    );

    GameInitializer::init_message_log(*message_log, config);

    // Create turn manager with RAII
    turn_manager = std::make_unique<TurnManager>(message_log.get());

    // Add all systems and capture pointers
    xp_system = world->add_system<ExperienceSystem>(message_log.get());
    combat_system = world->add_system<CombatSystem>(
        message_log.get(), world.get(), sprite_manager.get(),
        enemy_data.get(), xp_system
    );
    magic_system = world->add_system<MagicSystem>(message_log.get(), current_map);
    consumable_system = world->add_system<ConsumableSystem>(message_log.get());

    // AI and gameplay systems
    world->add_system<AISystem>(current_map, combat_system);
    world->add_system<ItemPickupSystem>(message_log.get());
    world->add_system<DeathSystem>();
    stair_system = world->add_system<StairSystem>(
        current_map, dungeon_manager.get(), message_log.get()
    );

    // Rendering systems
    world->add_system<SpriteUpdateSystem>(sprite_manager.get());
    map_render_system = world->add_system<MapRenderSystem>(
        sprite_manager.get(), current_map, 2, camera.get(),
        ui_layout.get(), tile_vis
    );
    render_system = world->add_system<RenderSystem>(
        sprite_manager.get(), 2, camera.get(),
        ui_layout.get(), tile_vis
    );

    return true;
}

void Game::init_ui_components() {
    minimap = std::make_unique<Minimap>(
        resources->renderer.get(),
        current_map,
        world.get(),
        ui_layout->minimap.x,
        ui_layout->minimap.y,
        ui_layout->minimap.w,
        ui_layout->minimap.h
    );

    hotbar = std::make_unique<UnifiedHotbar>(
        resources->renderer.get(),
        message_log->get_font(),
        world.get(),
        &magic_system->get_spell_database(),
        ui_layout->hotbar.x,
        ui_layout->hotbar.y,
        ui_layout->hotbar.w,
        ui_layout->hotbar.h
    );

    inventory_panel = std::make_unique<InventoryPanel>(
        resources->renderer.get(),
        message_log->get_font(),
        resources->title_font.get(),
        world.get(),
        200, 50, 880, 600
    );

    health_bar = std::make_unique<HealthBar>(
        resources->renderer.get(),
        message_log->get_font(),
        20, 20, 250, 35
    );
}

void Game::init_player() {
    Position spawn_pos = dungeon_manager->get_player_spawn_position();

    player = GameInitializer::init_player(
        *world,
        *sprite_manager,
        spawn_pos,
        config
    );

    // Initialize spells
    GameInitializer::init_player_spells(player, *world, magic_system);

    // Set camera and visibility
    camera->center_on(spawn_pos.x, spawn_pos.y);
    tile_vis->update_fov(spawn_pos.x, spawn_pos.y, config.gameplay.player_vision_range);
    minimap->center_on(spawn_pos.x, spawn_pos.y);
    minimap->update_from_fov(tile_vis);
}

// ============================================================================
// Main Game Loop
// ============================================================================

void Game::run() {
    running = true;
    SDL_Event event;

    const float frame_time = 1000.0f / config.gameplay.target_fps;

    LOG_INFO("=== Starting game loop ===");

    while (running) {
        Uint64 frame_start = SDL_GetTicks();

        // Handle input
        while (SDL_PollEvent(&event)) {
            handle_event(event);
        }

        // Update game state
        update();

        // Render
        render();

        // Frame rate limiting
        Uint64 frame_end = SDL_GetTicks();
        float elapsed = static_cast<float>(frame_end - frame_start);

        if (elapsed < frame_time) {
            SDL_Delay(static_cast<Uint32>(frame_time - elapsed));
        }
    }

    LOG_INFO("=== Game loop ended ===");
}

// ============================================================================
// Event Handling
// ============================================================================

void Game::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) {
        running = false;
        return;
    }

    if (event.type != SDL_EVENT_KEY_DOWN) {
        return;  // Only handle key presses
    }

    // Global hotkeys (work even when inventory is open)
    if (event.key.key == SDLK_ESCAPE) {
        if (inventory_panel->is_visible()) {
            inventory_panel->hide();
            return;
        }
        running = false;
        return;
    }

    if (event.key.key == SDLK_I) {
        inventory_panel->toggle();
        return;
    }

    // Skip game input if inventory is open
    if (inventory_panel->is_visible()) {
        return;
    }

    // Only allow player input during player turn
    if (!turn_manager->is_player_turn()) {
        return;
    }

    // Handle different input types
    handle_spell_and_item_hotkeys(event);
    handle_stair_navigation(event);
    handle_minimap_toggle(event);
    handle_player_movement(event);
}

void Game::handle_spell_and_item_hotkeys(const SDL_Event& event) {
    auto& components = world->get_component_manager();

    // Keys 1-9
    if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9) {
        int key_num = event.key.key - SDLK_1 + 1;  // 1-9

        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            // SHIFT + 1-5: Cast spells
            if (key_num >= 1 && key_num <= 5) {
                magic_system->cast_spell(components, player, key_num - 1);
            }
        }
        else {
            // 6-9: Use items (inventory slots 0-3)
            if (key_num >= 6 && key_num <= 9) {
                int item_slot = key_num - 6;
                if (use_consumable(item_slot)) {
                    end_player_turn();
                }
            }
        }
    }
    // Key 0: Use item from slot 4
    else if (event.key.key == SDLK_0) {
        if (use_consumable(4)) {
            end_player_turn();
        }
    }
}

void Game::handle_stair_navigation(const SDL_Event& event) {
    Position* pos = world->get_component<Position>(player);
    if (!pos) return;

    // '>' - Descend stairs
    if (event.key.key == SDLK_PERIOD && SDL_GetModState() & SDL_KMOD_SHIFT) {
        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_DOWN) {
            stair_system->trigger_stairs_down();
        }
    }
    // '<' - Ascend stairs
    else if (event.key.key == SDLK_COMMA && SDL_GetModState() & SDL_KMOD_SHIFT) {
        if (current_map->get_tile(pos->x, pos->y) == TileType::STAIRS_UP) {
            stair_system->trigger_stairs_up();
        }
    }
}

void Game::handle_minimap_toggle(const SDL_Event& event) {
    if (event.key.key == SDLK_F) {
        minimap->set_show_fog(!minimap->get_show_fog());
        message_log->add_info(minimap->get_show_fog() ?
            "Fog of war enabled" : "Fog of war disabled");
    }
}

void Game::handle_player_movement(const SDL_Event& event) {
    Position* pos = world->get_component<Position>(player);
    Facing* facing = world->get_component<Facing>(player);
    Energy* energy = world->get_component<Energy>(player);

    if (!pos || !facing || !energy) return;

    int new_x = pos->x;
    int new_y = pos->y;
    bool tried_to_move = false;

    // Calculate new position based on direction
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

    if (!tried_to_move) return;

    // Check if tile is walkable
    if (!current_map->is_walkable(new_x, new_y)) {
        handle_wall_collision(new_x, new_y);
        return;
    }

    // Check for entity collision (attack or blocked)
    if (check_entity_collision(new_x, new_y)) {
        end_player_turn();
        return;
    }

    // Move player
    move_player(new_x, new_y);
    end_player_turn();
    add_flavor_text();
}

void Game::handle_wall_collision(int x, int y) {
    TileType tile = current_map->get_tile(x, y);
    if (tile == TileType::WALL) {
        message_log->add_info("You bump into a wall.");
    }
    else if (tile == TileType::DOOR_CLOSED) {
        message_log->add_warning("The door is locked.");
    }
}

bool Game::check_entity_collision(int new_x, int new_y) {
    auto& components = world->get_component_manager();
    auto* blockers = components.get_array<BlocksMovement>();
    auto* positions = components.get_array<Position>();

    if (!blockers || !positions) return false;

    auto& blocker_entities = blockers->get_entities();
    for (Entity blocker : blocker_entities) {
        if (blocker == player) continue;

        Position* blocker_pos = positions->get(blocker);
        if (blocker_pos && blocker_pos->x == new_x && blocker_pos->y == new_y) {
            // Attack the blocking entity
            combat_system->try_attack(components, player, blocker);
            return true;
        }
    }

    return false;
}

void Game::move_player(int new_x, int new_y) {
    Position* pos = world->get_component<Position>(player);
    pos->x = new_x;
    pos->y = new_y;

    // Update camera and visibility
    camera->center_on(new_x, new_y);
    tile_vis->update_fov(new_x, new_y, config.gameplay.player_vision_range);
    minimap->center_on(new_x, new_y);
    minimap->update_from_fov(tile_vis);

    message_log->next_turn();
}

void Game::add_flavor_text() {
    // Random flavor messages (10% chance)
    if (rand() % 10 == 0) {
        const char* flavor[] = {
            "Your footsteps echo in the dungeon.",
            "You hear water dripping somewhere.",
            "A cold breeze brushes past you.",
            "The torch flickers."
        };
        message_log->add_lore(flavor[rand() % 4]);
    }
}

bool Game::use_consumable(int slot_index) {
    auto& components = world->get_component_manager();
    return consumable_system->use_from_inventory(components, player, slot_index);
}

void Game::end_player_turn() {
    Energy* energy = world->get_component<Energy>(player);
    if (energy) {
        energy->consume_turn();
        turn_manager->end_player_turn();
    }
}

// ============================================================================
// Update Logic
// ============================================================================

void Game::update() {
    // Process enemy turns
    if (turn_manager->is_enemy_turn()) {
        turn_manager->process_turn(*world);
        world->update(0.016f);  // Runs AISystem
        turn_manager->end_enemy_turn();
    }

    // Check for level transitions
    if (stair_system->was_triggered()) {
        handle_level_transition();
    }

    // Check for player death
    check_player_death();
}

void Game::handle_level_transition() {
    int old_depth = dungeon_manager->get_current_depth();
    int new_depth = old_depth;

    Position* player_pos = world->get_component<Position>(player);
    if (!player_pos) return;

    bool descending = stair_system->is_descending();

    // Calculate new depth
    if (descending) {
        new_depth++;
        message_log->add_success("You descend deeper...");
    }
    else {
        new_depth--;
        if (new_depth < 1) new_depth = 1;
        message_log->add_info("You ascend...");
    }

    // Save current exploration state
    auto old_exploration = world->take_tile_visibility();
    dungeon_manager->save_exploration(old_depth, std::move(old_exploration));

    // Clear entities only if level is new
    bool is_new_level = !dungeon_manager->has_level(new_depth);
    if (is_new_level) {
        clear_non_player_entities();
    }

    // Generate or load level
    Position spawn_pos;
    current_map = dungeon_manager->generate_level(new_depth, &spawn_pos, descending);

    // Update systems with new map
    update_systems_for_new_level();

    // Restore or create exploration state
    restore_or_create_exploration(new_depth);

    // Recreate camera for new map size
    recreate_camera();

    // Update minimap
    minimap->set_map(current_map);

    // Move player to spawn position
    player_pos->x = spawn_pos.x;
    player_pos->y = spawn_pos.y;

    // Update camera and visibility
    camera->center_on(spawn_pos.x, spawn_pos.y);
    tile_vis->update_fov(spawn_pos.x, spawn_pos.y, config.gameplay.player_vision_range);
    minimap->center_on(spawn_pos.x, spawn_pos.y);
    minimap->update_from_fov(tile_vis);

    // Spawn enemies if new level
    if (is_new_level) {
        dungeon_manager->spawn_enemies(*enemy_spawner, *world);
    }

    message_log->add_info("Dungeon Level " + std::to_string(new_depth));
}

void Game::clear_non_player_entities() {
    auto& components = world->get_component_manager();
    auto* all_entities = components.get_array<Position>();
    if (!all_entities) return;

    auto& entities = all_entities->get_entities();
    std::vector<Entity> to_remove;

    // Collect non-player entities
    for (Entity e : entities) {
        if (!world->has_component<PlayerControlled>(e)) {
            to_remove.push_back(e);
        }
    }

    // Remove all components from entities
    for (Entity e : to_remove) {
        // Remove all possible components
        if (components.has_component<Position>(e)) components.remove_component<Position>(e);
        if (components.has_component<Renderable>(e)) components.remove_component<Renderable>(e);
        if (components.has_component<SpriteBase>(e)) components.remove_component<SpriteBase>(e);
        if (components.has_component<AI>(e)) components.remove_component<AI>(e);
        if (components.has_component<BlocksMovement>(e)) components.remove_component<BlocksMovement>(e);
        if (components.has_component<CombatStats>(e)) components.remove_component<CombatStats>(e);
        if (components.has_component<Energy>(e)) components.remove_component<Energy>(e);
        if (components.has_component<EnemyType>(e)) components.remove_component<EnemyType>(e);
        if (components.has_component<Name>(e)) components.remove_component<Name>(e);
    }
}


void Game::update_systems_for_new_level() {
    stair_system->set_map(current_map);
    map_render_system->set_map(current_map);
}

void Game::restore_or_create_exploration(int depth) {
    auto restored_exploration = dungeon_manager->get_exploration(depth);
    if (restored_exploration) {
        world->set_tile_visibility(std::move(restored_exploration));
    }
    else {
        world->initialize_tile_visibility(
            current_map->get_width(),
            current_map->get_height()
        );
    }

    tile_vis = world->get_tile_visibility();
    map_render_system->set_tile_visibility(tile_vis);
    render_system->set_tile_visibility(tile_vis);
}

void Game::recreate_camera() {
    camera = std::make_unique<Camera>(
        ui_layout->game_viewport.w,
        ui_layout->game_viewport.h,
        current_map->get_width(),
        current_map->get_height(),
        config.display.get_scaled_tile_size()
    );

    map_render_system->set_camera(camera.get());
    render_system->set_camera(camera.get());
}

void Game::check_player_death() {
    CombatStats* stats = world->get_component<CombatStats>(player);
    if (stats && !stats->is_alive()) {
        message_log->add_warning("YOU DIED!");
        message_log->add_info("Press ESC to quit.");
        // Could add game over state here
    }
}

// ============================================================================
// Rendering
// ============================================================================

void Game::render() {
    auto* renderer = resources->renderer.get();

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Render UI backgrounds
    render_ui_backgrounds();

    // Render game world (only if inventory closed and player turn)
    if (turn_manager->is_player_turn() && !inventory_panel->is_visible()) {
        world->update(0.016f);
    }

    // Render UI elements
    render_ui_elements();

    // Present frame
    SDL_RenderPresent(renderer);
}

void Game::render_ui_backgrounds() {
    auto* renderer = resources->renderer.get();

    // Top bar background
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_FRect top_bar_rect = {
        static_cast<float>(ui_layout->top_bar.x),
        static_cast<float>(ui_layout->top_bar.y),
        static_cast<float>(ui_layout->top_bar.w),
        static_cast<float>(ui_layout->top_bar.h)
    };
    SDL_RenderFillRect(renderer, &top_bar_rect);

    // Top bar border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderRect(renderer, &top_bar_rect);

    // Minimap background
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
    SDL_FRect minimap_rect = {
        static_cast<float>(ui_layout->minimap.x),
        static_cast<float>(ui_layout->minimap.y),
        static_cast<float>(ui_layout->minimap.w),
        static_cast<float>(ui_layout->minimap.h)
    };
    SDL_RenderFillRect(renderer, &minimap_rect);

    // Minimap border
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderRect(renderer, &minimap_rect);
}

void Game::render_ui_elements() {
    render_player_stats();
    message_log->render();
    minimap->render();
    hotbar->render(player);
    inventory_panel->render(player);

    // Debug borders (optional)
    if (config.logging.log_level == "DEBUG") {
        ui_layout->render_debug_borders(resources->renderer.get());
    }
}

void Game::render_player_stats() {
    auto* renderer = resources->renderer.get();
    auto* font = resources->ui_font.get();

    CombatStats* stats = world->get_component<CombatStats>(player);
    Mana* mana = world->get_component<Mana>(player);
    Inventory* inventory = world->get_component<Inventory>(player);
    Experience* xp = world->get_component<Experience>(player);

    int x_offset = ui_layout->top_bar.x + 20;
    int y_base = ui_layout->top_bar.y + 25;

    // HP
    if (stats && font) {
        render_hp_display(stats->current_hp, stats->max_hp, x_offset, y_base);
    }

    // MP
    if (mana && font) {
        render_stat_text(renderer, font, "MP: " + std::to_string(mana->current) +
            "/" + std::to_string(mana->maximum),
            x_offset, y_base + 25, { 100, 150, 255, 255 });
    }

    // Gold
    if (inventory && font) {
        render_stat_text(renderer, font, "Gold: " + std::to_string(inventory->gold),
            x_offset + 230, y_base, { 255, 215, 0, 255 });
    }

    // Level
    if (xp && font) {
        render_stat_text(renderer, font, "Level: " + std::to_string(xp->level),
            x_offset + 430, y_base - 10, { 100, 200, 255, 255 });
    }

    // Depth
    std::string depth_text = "Depth: " + std::to_string(dungeon_manager->get_current_depth());
    render_stat_text(renderer, font, depth_text,
        x_offset + 630, y_base, { 255, 150, 50, 255 });
}

void Game::render_stat_text(SDL_Renderer* renderer, TTF_Font* font,
    const std::string& text, int x, int y,
    SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dest = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(surface->w), static_cast<float>(surface->h)
            };
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
}

void Game::render_hp_display(int current_hp, int max_hp, int x, int y) {
    auto* renderer = resources->renderer.get();
    auto* font = resources->ui_font.get();

    if (!font) return;

    std::string hp_text = "HP: " + std::to_string(current_hp) + "/" +
        std::to_string(max_hp);

    // Choose color based on HP percentage
    SDL_Color color;
    float hp_percent = static_cast<float>(current_hp) / max_hp;

    if (hp_percent < 0.25f) {
        color = { 255, 50, 50, 255 };      // Red (critical)
    }
    else if (hp_percent < 0.50f) {
        color = { 255, 200, 50, 255 };     // Yellow (warning)
    }
    else {
        color = { 100, 255, 100, 255 };    // Green (healthy)
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