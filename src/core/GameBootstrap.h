// src/core/GameBootstrap.h
// Encapsulates game initialization logic
// Extracted from Game class to reduce God-object complexity

#pragma once

#include <memory>
#include <string>
#include "../ecs/Entity.h"
#include "../config/GameConfig.h"
#include "ResourceManager.h"

// Forward declarations
class World;
class SpriteManager;
class EnemyDataManager;
class DungeonManager;
class EnemySpawner;
class Map;
class TileVisibility;
class MessageLog;
struct UILayout;
class Minimap;
class UnifiedHotbar;
class InventoryPanel;
class HealthBar;
class Camera;
class TurnManager;
class CombatSystem;
class MagicSystem;
class ExperienceSystem;
class StairSystem;
class ConsumableSystem;
class MapRenderSystem;
class RenderSystem;

// ============================================================================
// GameBootstrap - Handles all game initialization
// ============================================================================
// Extracts initialization responsibility from Game class
// Single Responsibility: Setup and configure game components
// ============================================================================

class GameBootstrap {
public:
    // Result of bootstrap operation with all initialized components
    struct BootstrapResult {
        bool success;
        std::string error_message;

        // Configuration
        GameConfig config;

        // Resources (RAII)
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
        std::unique_ptr<TurnManager> turn_manager;
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
        TileVisibility* tile_vis = nullptr;
    };

    // Main initialization entry point
    // Returns fully configured game components or error
    static BootstrapResult initialize(unsigned int seed = 0);

private:
    // Phase 1: Configuration
    static bool load_config(GameConfig& config);

    // Phase 2: SDL and rendering resources
    static bool init_resources(
        std::unique_ptr<ResourceManager>& resources,
        const GameConfig& config
    );

    // Phase 3: Game managers (non-rendering)
    static bool init_managers(
        std::unique_ptr<SpriteManager>& sprite_manager,
        std::unique_ptr<EnemyDataManager>& enemy_data,
        std::unique_ptr<DungeonManager>& dungeon_manager,
        std::unique_ptr<EnemySpawner>& enemy_spawner,
        ResourceManager* resources,
        const GameConfig& config
    );

    // Phase 4: UI layout
    static void init_ui_layout(
        std::unique_ptr<UILayout>& ui_layout,
        const GameConfig& config
    );

    // Phase 5: World, map, and visibility
    static bool init_world(
        std::unique_ptr<World>& world,
        std::unique_ptr<Camera>& camera,
        std::unique_ptr<MessageLog>& message_log,
        std::unique_ptr<TurnManager>& turn_manager,
        Map*& current_map,
        TileVisibility*& tile_vis,
        CombatSystem*& combat_system,
        MagicSystem*& magic_system,
        ExperienceSystem*& xp_system,
        StairSystem*& stair_system,
        ConsumableSystem*& consumable_system,
        MapRenderSystem*& map_render_system,
        RenderSystem*& render_system,
        DungeonManager* dungeon_manager,
        SpriteManager* sprite_manager,
        EnemyDataManager* enemy_data,
        EnemySpawner* enemy_spawner,
        ResourceManager* resources,
        UILayout* ui_layout,
        const GameConfig& config
    );

    // Phase 6: UI components
    static void init_ui_components(
        std::unique_ptr<Minimap>& minimap,
        std::unique_ptr<UnifiedHotbar>& hotbar,
        std::unique_ptr<InventoryPanel>& inventory_panel,
        std::unique_ptr<HealthBar>& health_bar,
        World* world,
        Map* current_map,
        MagicSystem* magic_system,
        MessageLog* message_log,
        ResourceManager* resources,
        UILayout* ui_layout
    );

    // Phase 7: Player creation
    static void init_player(
        Entity& player,
        World* world,
        SpriteManager* sprite_manager,
        DungeonManager* dungeon_manager,
        MagicSystem* magic_system,
        Camera* camera,
        TileVisibility* tile_vis,
        Minimap* minimap,
        const GameConfig& config
    );

    // Phase 8: Enemy spawning
    static void spawn_initial_enemies(
        DungeonManager* dungeon_manager,
        EnemySpawner* enemy_spawner,
        World* world
    );

    // Phase 9: Welcome messages
    static void send_welcome_messages(
        MessageLog* message_log,
        int starting_depth
    );
};