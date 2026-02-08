// src/core/Game.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>

// Forward declarations
#include "../ecs/World.h"
#include "../ecs/Entity.h"
#include "../config/GameConfig.h"
#include "ResourceManager.h"
#include "../graphics/SpriteManager.h"
#include "../data/EnemyData.h"
#include "../world/DungeonManager.h"
#include "../world/Map.h"
#include "../world/TileVisibility.h"
#include "../spawning/EnemySpawner.h"
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../ui/HealthBar.h"
#include "../systems/Camera.h"
#include "../systems/TurnManager.h"
#include "../systems/CombatSystem.h"
#include "../systems/MagicSystem.h"
#include "../systems/ExperienceSystem.h"
#include "../systems/StairSystem.h"
#include "../systems/ConsumableSystem.h"
#include "../systems/MapRenderSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/DeathSystem.h"
#include "../systems/ItemPickupSystem.h"  
#include "../systems/AISystem.h"
#include "../systems/SpriteUpdateSystem.h"

// ============================================================================
// Game Class - Encapsulates entire game state and logic
// ============================================================================

class Game {
public:
    Game();
    ~Game();

    // Initialization
    bool initialize();

    // Main game loop
    void run();

private:
    // ========================================
    // Member Variables
    // ========================================

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
    Entity player = 0;
    Map* current_map = nullptr;

    // Systems (pointers to systems owned by World)
    TurnManager* turn_manager = nullptr;
    CombatSystem* combat_system = nullptr;
    MagicSystem* magic_system = nullptr;
    ExperienceSystem* xp_system = nullptr;
    StairSystem* stair_system = nullptr;
    ConsumableSystem* consumable_system = nullptr;
    MapRenderSystem* map_render_system = nullptr;
    RenderSystem* render_system = nullptr;

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
    bool running = false;
    TileVisibility* tile_vis = nullptr;

    // ========================================
    // Initialization Methods
    // ========================================

    bool load_config();
    bool init_resources();
    bool init_managers();
    void init_ui_layout();
    bool init_world();
    void init_ui_components();
    void init_player();

    // ========================================
    // Event Handling
    // ========================================

    void handle_event(const SDL_Event& event);
    void handle_spell_and_item_hotkeys(const SDL_Event& event);
    void handle_stair_navigation(const SDL_Event& event);
    void handle_minimap_toggle(const SDL_Event& event);
    void handle_player_movement(const SDL_Event& event);

    // Movement helpers
    void handle_wall_collision(int x, int y);
    bool check_entity_collision(int new_x, int new_y);
    void move_player(int new_x, int new_y);
    void add_flavor_text();
    bool use_consumable(int slot_index);
    void end_player_turn();

    // ========================================
    // Update Logic
    // ========================================

    void update();
    void handle_level_transition();
    void clear_non_player_entities();
    void update_systems_for_new_level();
    void restore_or_create_exploration(int depth);
    void recreate_camera();
    void check_player_death();

    // ========================================
    // Rendering
    // ========================================

    void render();
    void render_ui_backgrounds();
    void render_ui_elements();
    void render_player_stats();
    void render_stat_text(SDL_Renderer* renderer, TTF_Font* font,
        const std::string& text, int x, int y,
        SDL_Color color);
    void render_hp_display(int current_hp, int max_hp, int x, int y);
};