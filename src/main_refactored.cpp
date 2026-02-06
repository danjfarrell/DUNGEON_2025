// src/main_refactored.cpp
// Modular, configuration-driven main file with RAII resource management
// 
// This replaces the monolithic main.cpp with:
// - Configuration file loading (config.json)
// - RAII-based resource management (automatic cleanup)
// - Modular initialization (GameInitializer)
// - Separation of concerns (Game class for game loop)

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <memory>

// Configuration and core systems
#include "config/GameConfig.h"
#include "core/ResourceManager.h"
#include "core/GameInitializer.h"

// ECS
#include "ecs/World.h"

// Components
#include "components/Components.h"
#include "components/MagicComponents.h"
#include "components/Equipment.h"

// Systems
#include "systems/RenderSystem.h"
#include "systems/SpriteUpdateSystem.h"
#include "systems/Camera.h"
#include "systems/MapRenderSystem.h"
#include "systems/TurnManager.h"
#include "systems/CombatSystem.h"
#include "systems/AISystem.h"
#include "systems/DeathSystem.h"
#include "systems/ItemPickupSystem.h"
#include "systems/MagicSystem.h"
#include "systems/ExperienceSystem.h"
#include "systems/StairSystem.h"
#include "systems/ConsumableSystem.h"

// Graphics and data
#include "graphics/SpriteManager.h"
#include "data/EnemyData.h"
#include "spawning/EnemySpawner.h"

// World
#include "world/Map.h"
#include "world/MapGenerators.h"
#include "world/DungeonManager.h"
#include "world/TileVisibility.h"

// UI
#include "ui/MessageLog.h"
#include "ui/UILayout.h"
#include "ui/Minimap.h"
#include "ui/UnifiedHotbar.h"
#include "ui/InventoryPanel.h"
#include "ui/HealthBar.h"

// Utilities
#include "utils/Logger.h"
#include "magic/SpellDatabase.h"

// ============================================================================
// Forward Declarations
// ============================================================================

class Game;
void render_hp_display(SDL_Renderer* renderer, TTF_Font* font,
                      int current_hp, int max_hp, int x, int y);

// ============================================================================
// Game Class - Encapsulates game state and main loop
// ============================================================================

class Game {
private:
    // Configuration
    GameConfig config;
    
    // Resources (RAII - automatic cleanup)
    std::unique_ptr<ResourceManager> resources;
    
    // Managers
    std::unique_ptr<SpriteManager> sprite_manager;
    std::unique_ptr<EnemyDataManager> enemy_data;
    std::unique_ptr<DungeonManager> dungeon_manager;
    std::unique_ptr<EnemySpawner> enemy_spawner;
    
    // World and entities
    std::unique_ptr<World> world;
    Entity player;
    Map* current_map;
    
    // Systems (pointers to systems owned by World)
    TurnManager* turn_manager;
    CombatSystem* combat_system;
    MagicSystem* magic_system;
    ExperienceSystem* xp_system;
    StairSystem* stair_system;
    ConsumableSystem* consumable_system;
    MapRenderSystem* map_render_system;
    RenderSystem* render_system;
    
    // Camera
    std::unique_ptr<Camera> camera;
    
    // UI
    std::unique_ptr<UILayout> ui_layout;
    std::unique_ptr<MessageLog> message_log;
    std::unique_ptr<Minimap> minimap;
    std::unique_ptr<UnifiedHotbar> hotbar;
    std::unique_ptr<InventoryPanel> inventory_panel;
    std::unique_ptr<HealthBar> health_bar;
    
    // Game state
    bool running;
    TileVisibility* tile_vis;
    
public:
    Game() : running(false), player(0), current_map(nullptr),
             turn_manager(nullptr), combat_system(nullptr), magic_system(nullptr),
             xp_system(nullptr), stair_system(nullptr), consumable_system(nullptr),
             map_render_system(nullptr), render_system(nullptr), tile_vis(nullptr) {
    }
    
    ~Game() {
        LOG_INFO("=== Game shutting down ===");
        // RAII handles cleanup automatically
    }
    
    // Initialize game (replaces scattered init code)
    bool initialize() {
        LOG_INFO("=== Initializing Game ===");
        
        // Load configuration
        if (!load_config()) {
            return false;
        }
        
        // Initialize logger with config settings
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
    
    // Main game loop
    void run() {
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
    
private:
    // Load configuration from file
    bool load_config() {
        config.load_from_file("assets/config.json");
        // Always succeeds - uses defaults if file not found
        return true;
    }
    
    // Initialize SDL resources (RAII)
    bool init_resources() {
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
        } catch (const std::exception& e) {
            LOG_ERROR("Resource initialization failed: " + std::string(e.what()));
            return false;
        }
    }
    
    // Initialize game managers
    bool init_managers() {
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
    
    // Initialize UI layout
    void init_ui_layout() {
        ui_layout = std::make_unique<UILayout>(
            config.display.screen_width,
            config.display.screen_height
        );
        
        LOG_INFO("UI Layout: Game viewport " + 
                std::to_string(ui_layout->game_viewport.w) + "x" +
                std::to_string(ui_layout->game_viewport.h));
    }
    
    // Initialize world and generate first level
    bool init_world() {
        world = std::make_unique<World>();
        enemy_spawner.reset(new EnemySpawner(
            world.get(),
            sprite_manager.get(),
            enemy_data.get()
        ));
        
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
        
        // Initialize turn manager
        message_log = std::make_unique<MessageLog>(
            resources->renderer.get(),
            ui_layout->message_log.x,
            ui_layout->message_log.y,
            ui_layout->message_log.w,
            ui_layout->message_log.h
        );
        
        GameInitializer::init_message_log(*message_log, config);
        
        turn_manager = new TurnManager(message_log.get());
        
        // Add all systems
        GameInitializer::init_world_systems(
            *world,
            current_map,
            sprite_manager.get(),
            message_log.get(),
            enemy_data.get(),
            camera.get(),
            ui_layout.get(),
            tile_vis,
            dungeon_manager.get()
        );
        
        // Get system pointers
        magic_system = dynamic_cast<MagicSystem*>(world->get_component_manager().get_array<Mana>());
        // Note: Better to add getter methods to World for system access
        
        return true;
    }
    
    // Initialize UI components
    void init_ui_components() {
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
    
    // Create player entity
    void init_player() {
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
    
    // Event handling (next part...)
    void handle_event(const SDL_Event& event);
    void update();
    void render();
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    try {
        Game game;
        
        if (!game.initialize()) {
            LOG_ERROR("Failed to initialize game");
            return 1;
        }
        
        game.run();
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

// Helper function for rendering HP text
void render_hp_display(SDL_Renderer* renderer, TTF_Font* font,
                      int current_hp, int max_hp, int x, int y) {
    if (!font) return;
    
    std::string hp_text = "HP: " + std::to_string(current_hp) + "/" + 
                         std::to_string(max_hp);
    
    SDL_Color color;
    float hp_percent = static_cast<float>(current_hp) / max_hp;
    
    if (hp_percent < 0.25f) {
        color = {255, 50, 50, 255};
    } else if (hp_percent < 0.50f) {
        color = {255, 200, 50, 255};
    } else {
        color = {100, 255, 100, 255};
    }
    
    SDL_Surface* surface = TTF_RenderText_Blended(font, hp_text.c_str(), 0, color);
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