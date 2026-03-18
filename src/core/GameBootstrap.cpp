// src/core/GameBootstrap.cpp
// Implementation of GameBootstrap initialization phases

#include "GameBootstrap.h"
#include "GameInitializer.h"
#include "../utils/Logger.h"

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
// Main Bootstrap Entry Point
// ============================================================================

GameBootstrap::BootstrapResult GameBootstrap::initialize(unsigned int seed) {
    BootstrapResult result;
    result.success = false;

    // CRITICAL: Initialize logger FIRST before any LOG_* calls
    Logger::get_instance("game_log.txt", LogLevel::DEBUG);

    LOG_INFO("=== GameBootstrap: Starting initialization ===");

    // Phase 1: Load configuration
    if (!load_config(result.config)) {
        result.error_message = "Failed to load configuration";
        return result;
    }

    // Re-initialize logger with config settings
    Logger::get_instance(
        result.config.logging.log_file,
        result.config.get_log_level()
    );

    // Phase 2: Initialize SDL and resources
    if (!init_resources(result.resources, result.config)) {
        result.error_message = "Failed to initialize SDL resources";
        return result;
    }

    // Phase 3: Initialize game managers
    if (!init_managers(
        result.sprite_manager,
        result.enemy_data,
        result.dungeon_manager,
        result.enemy_spawner,
        result.resources.get(),
        result.config
    )) {
        result.error_message = "Failed to initialize game managers";
        return result;
    }

    // Phase 4: Create UI layout
    init_ui_layout(result.ui_layout, result.config);

    // Phase 5: Initialize world, systems, and map
    if (!init_world(
        result.world,
        result.camera,
        result.message_log,
        result.turn_manager,
        result.current_map,
        result.tile_vis,
        result.combat_system,
        result.magic_system,
        result.xp_system,
        result.stair_system,
        result.consumable_system,
        result.map_render_system,
        result.render_system,
        result.dungeon_manager.get(),
        result.sprite_manager.get(),
        result.enemy_data.get(),
        result.enemy_spawner.get(),
        result.resources.get(),
        result.ui_layout.get(),
        result.config
    )) {
        result.error_message = "Failed to initialize world";
        return result;
    }

    // Phase 6: Initialize UI components
    init_ui_components(
        result.minimap,
        result.hotbar,
        result.inventory_panel,
        result.health_bar,
        result.world.get(),
        result.current_map,
        result.magic_system,
        result.message_log.get(),
        result.resources.get(),
        result.ui_layout.get()
    );

    // Phase 7: Create player
    init_player(
        result.player,
        result.world.get(),
        result.sprite_manager.get(),
        result.dungeon_manager.get(),
        result.magic_system,
        result.camera.get(),
        result.tile_vis,
        result.minimap.get(),
        result.config
    );

    // Phase 8: Spawn enemies
    spawn_initial_enemies(
        result.dungeon_manager.get(),
        result.enemy_spawner.get(),
        result.world.get()
    );

    // Phase 9: Send welcome messages
    send_welcome_messages(
        result.message_log.get(),
        result.config.gameplay.starting_depth
    );

    LOG_INFO("=== GameBootstrap: Initialization complete ===");
    result.success = true;
    return result;
}

// ============================================================================
// Phase 1: Configuration
// ============================================================================

bool GameBootstrap::load_config(GameConfig& config) {
    LOG_INFO("Phase 1: Loading configuration");
    config.load_from_file("assets/config.json");
    // Always succeeds - uses defaults if file not found
    return true;
}

// ============================================================================
// Phase 2: SDL and Resources
// ============================================================================

bool GameBootstrap::init_resources(
    std::unique_ptr<ResourceManager>& resources,
    const GameConfig& config
) {
    LOG_INFO("Phase 2: Initializing SDL and resources");

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

// ============================================================================
// Phase 3: Game Managers
// ============================================================================

bool GameBootstrap::init_managers(
    std::unique_ptr<SpriteManager>& sprite_manager,
    std::unique_ptr<EnemyDataManager>& enemy_data,
    std::unique_ptr<DungeonManager>& dungeon_manager,
    std::unique_ptr<EnemySpawner>& enemy_spawner,
    ResourceManager* resources,
    const GameConfig& config
) {
    LOG_INFO("Phase 3: Initializing game managers");

    // Create sprite manager
    sprite_manager = std::make_unique<SpriteManager>(
        resources->renderer.get(),
        config.display.tile_size,
        config.display.tile_size
    );

    // Initialize sprites
    auto sprite_result = GameInitializer::init_sprites(*sprite_manager, config);
    if (!sprite_result.success) {
        LOG_ERROR(sprite_result.error_message);
        return false;
    }

    // Create enemy data manager
    enemy_data = std::make_unique<EnemyDataManager>();

    // Load enemy data
    GameInitializer::init_enemy_data(*enemy_data, config);

    // Create dungeon manager
    //dungeon_manager = std::make_unique<DungeonManager>(
    //    config.gameplay.map_width,
    //    config.gameplay.map_height
    //);

    // AFTER:
    // Generate seed now if not set, so we can log it
    unsigned int seed = config.gameplay.seed;
    if (seed == 0) {
        seed = std::random_device{}();
    }
    // Log it — this is how players can replay a run
    LOG_INFO("=== GAME SEED: " + std::to_string(seed) + " ===");
    LOG_INFO("    Replay with: --seed " + std::to_string(seed));

    dungeon_manager = std::make_unique<DungeonManager>(
        config.gameplay.map_width,
        config.gameplay.map_height,
        seed
    );

    // Store back so everything downstream can see it
    // (optional but useful for display in UI later)
    // config.gameplay.seed = seed;  // config is const& here, so skip or change signature


    // Create enemy spawner (world pointer will be set later)
    enemy_spawner = std::make_unique<EnemySpawner>(
        nullptr,  // Will set world pointer in Phase 5
        sprite_manager.get(),
        enemy_data.get()
    );

    return true;
}

// ============================================================================
// Phase 4: UI Layout
// ============================================================================

void GameBootstrap::init_ui_layout(
    std::unique_ptr<UILayout>& ui_layout,
    const GameConfig& config
) {
    LOG_INFO("Phase 4: Creating UI layout");

    ui_layout = std::make_unique<UILayout>(
        config.display.screen_width,
        config.display.screen_height
    );

    LOG_INFO("UI Layout: Game viewport " +
        std::to_string(ui_layout->game_viewport.w) + "x" +
        std::to_string(ui_layout->game_viewport.h));
}

// ============================================================================
// Phase 5: World Initialization
// ============================================================================

bool GameBootstrap::init_world(
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
) {
    LOG_INFO("Phase 5: Initializing world and systems");

    LOG_INFO("Phase 5.1: Creating World");
    world = std::make_unique<World>();

    LOG_INFO("Phase 5.2: Setting enemy spawner world pointer");
    enemy_spawner->set_world(world.get());

    LOG_INFO("Phase 5.3: Generating first level (depth " + std::to_string(config.gameplay.starting_depth) + ")");
    current_map = dungeon_manager->generate_level(config.gameplay.starting_depth);
    LOG_INFO("Phase 5.3 complete: Level generated");
    LOG_INFO("Phase 5.3 complete: Level generated");

    LOG_INFO("Phase 5.4: Initializing tile visibility (" +
        std::to_string(current_map->get_width()) + "x" +
        std::to_string(current_map->get_height()) + ")");
    world->initialize_tile_visibility(
        current_map->get_width(),
        current_map->get_height()
    );
    tile_vis = world->get_tile_visibility();
    LOG_INFO("Phase 5.4 complete: Visibility initialized");

    LOG_INFO("Phase 5.5: Creating camera");
    camera = std::make_unique<Camera>(
        ui_layout->game_viewport.w,
        ui_layout->game_viewport.h,
        current_map->get_width(),
        current_map->get_height(),
        config.display.get_scaled_tile_size()
    );
    LOG_INFO("Phase 5.5 complete: Camera created");

    // Create message log
    message_log = std::make_unique<MessageLog>(
        resources->renderer.get(),
        ui_layout->message_log.x,
        ui_layout->message_log.y,
        ui_layout->message_log.w,
        ui_layout->message_log.h
    );

    GameInitializer::init_message_log(*message_log, config);

    // Create turn manager
    turn_manager = std::make_unique<TurnManager>(message_log.get());

    // Add all systems and capture pointers
    xp_system = world->add_system<ExperienceSystem>(message_log.get());
    combat_system = world->add_system<CombatSystem>(
        message_log.get(), world.get(), sprite_manager,
        enemy_data, xp_system
    );
    magic_system = world->add_system<MagicSystem>(message_log.get(), current_map);
    consumable_system = world->add_system<ConsumableSystem>(message_log.get());

    // AI and gameplay systems
    world->add_system<AISystem>(current_map, combat_system);
    world->add_system<ItemPickupSystem>(message_log.get());
    world->add_system<DeathSystem>();
    stair_system = world->add_system<StairSystem>(
        current_map, dungeon_manager, message_log.get()
    );

    // Rendering systems
    world->add_system<SpriteUpdateSystem>(sprite_manager);
    map_render_system = world->add_system<MapRenderSystem>(
        sprite_manager, current_map, 2, camera.get(),
        ui_layout, tile_vis
    );
    render_system = world->add_system<RenderSystem>(
        sprite_manager, 2, camera.get(),
        ui_layout, tile_vis
    );

    return true;
}

// ============================================================================
// Phase 6: UI Components
// ============================================================================

void GameBootstrap::init_ui_components(
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
) {
    LOG_INFO("Phase 6: Initializing UI components");

    minimap = std::make_unique<Minimap>(
        resources->renderer.get(),
        current_map,
        world,
        ui_layout->minimap.x,
        ui_layout->minimap.y,
        ui_layout->minimap.w,
        ui_layout->minimap.h
    );

    hotbar = std::make_unique<UnifiedHotbar>(
        resources->renderer.get(),
        message_log->get_font(),
        world,
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
        world,
        200, 50, 880, 600
    );

    health_bar = std::make_unique<HealthBar>(
        resources->renderer.get(),
        message_log->get_font(),
        20, 20, 250, 35
    );
}

// ============================================================================
// Phase 7: Player Creation
// ============================================================================

void GameBootstrap::init_player(
    Entity& player,
    World* world,
    SpriteManager* sprite_manager,
    DungeonManager* dungeon_manager,
    MagicSystem* magic_system,
    Camera* camera,
    TileVisibility* tile_vis,
    Minimap* minimap,
    const GameConfig& config
) {
    LOG_INFO("Phase 7: Creating player");

    Position spawn_pos = dungeon_manager->get_player_spawn_position();

    player = GameInitializer::init_player(
        *world,
        *sprite_manager,
        spawn_pos,
        config
    );

    // Initialize player spells
    GameInitializer::init_player_spells(player, *world, magic_system);

    // Set camera and visibility
    camera->center_on(spawn_pos.x, spawn_pos.y);
    tile_vis->update_fov(spawn_pos.x, spawn_pos.y, config.gameplay.player_vision_range);
    minimap->center_on(spawn_pos.x, spawn_pos.y);
    minimap->update_from_fov(tile_vis);
}

// ============================================================================
// Phase 8: Enemy Spawning
// ============================================================================

void GameBootstrap::spawn_initial_enemies(
    DungeonManager* dungeon_manager,
    EnemySpawner* enemy_spawner,
    World* world
) {
    LOG_INFO("Phase 8: Spawning initial enemies");
    dungeon_manager->spawn_enemies(*enemy_spawner, *world);
}

// ============================================================================
// Phase 9: Welcome Messages
// ============================================================================

void GameBootstrap::send_welcome_messages(
    MessageLog* message_log,
    int starting_depth
) {
    LOG_INFO("Phase 9: Sending welcome messages");
    GameInitializer::send_welcome_messages(*message_log, starting_depth);
}