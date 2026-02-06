// src/core/GameInitializer.h
#pragma once

#include "../ecs/World.h"
#include "../config/GameConfig.h"
#include "../graphics/SpriteManager.h"
#include "../data/EnemyData.h"
#include "../world/DungeonManager.h"
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../systems/TurnManager.h"
#include "../utils/Logger.h"

// Encapsulates all game initialization logic
class GameInitializer {
public:
    struct InitResult {
        bool success;
        std::string error_message;
    };
    
    // Initialize sprite manager
    static InitResult init_sprites(SpriteManager& sprite_manager, const GameConfig& config) {
        LOG_INFO("=== Initializing Sprite System ===");
        
        if (!sprite_manager.load_config(config.assets.sprite_config)) {
            return {false, "Failed to load sprite configuration: " + config.assets.sprite_config};
        }
        
        sprite_manager.dump_config_to_log();
        LOG_INFO("Sprite system initialized successfully");
        return {true, ""};
    }
    
    // Initialize enemy data
    static InitResult init_enemy_data(EnemyDataManager& enemy_data, const GameConfig& config) {
        LOG_INFO("=== Loading Enemy Data ===");
        
        if (!enemy_data.load_from_file(config.assets.enemy_data)) {
            LOG_WARN("Failed to load enemy data from: " + config.assets.enemy_data);
            return {false, "Enemy data loading failed (continuing anyway)"};
        }
        
        LOG_INFO("Enemy data loaded successfully");
        return {true, ""};
    }
    
    // Initialize message log with font
    static InitResult init_message_log(MessageLog& message_log, const GameConfig& config) {
        LOG_INFO("=== Initializing Message Log ===");
        
        // Try primary font
        if (message_log.init_font(config.assets.font_path, config.assets.font_size)) {
            LOG_INFO("Loaded font: " + config.assets.font_path);
            return {true, ""};
        }
        
        // Try fallback font
        if (!config.assets.font_fallback.empty()) {
            if (message_log.init_font(config.assets.font_fallback, config.assets.font_size)) {
                LOG_INFO("Loaded fallback font: " + config.assets.font_fallback);
                return {true, ""};
            }
        }
        
        LOG_WARN("Could not load any font - message log will not render");
        return {false, "Font loading failed (continuing without text)"};
    }
    
    // Initialize world and add all systems
    static void init_world_systems(
        World& world,
        Map* game_map,
        SpriteManager* sprite_manager,
        MessageLog* message_log,
        EnemyDataManager* enemy_data,
        Camera* camera,
        UILayout* ui_layout,
        TileVisibility* tile_vis,
        DungeonManager* dungeon_manager
    ) {
        LOG_INFO("=== Creating ECS Systems ===");
        
        // Create systems in dependency order
        auto* xp_system = world.add_system<ExperienceSystem>(message_log);
        auto* combat_system = world.add_system<CombatSystem>(
            message_log, &world, sprite_manager, enemy_data, xp_system
        );
        auto* magic_system = world.add_system<MagicSystem>(message_log, game_map);
        auto* consumable_system = world.add_system<ConsumableSystem>(message_log);
        
        // AI and gameplay systems
        world.add_system<AISystem>(game_map, combat_system);
        world.add_system<ItemPickupSystem>(message_log);
        world.add_system<DeathSystem>();
        auto* stair_system = world.add_system<StairSystem>(
            game_map, dungeon_manager, message_log
        );
        
        // Rendering systems
        world.add_system<SpriteUpdateSystem>(sprite_manager);
        auto* map_render_system = world.add_system<MapRenderSystem>(
            sprite_manager, game_map, 2, camera, ui_layout, tile_vis
        );
        auto* render_system = world.add_system<RenderSystem>(
            sprite_manager, 2, camera, ui_layout, tile_vis
        );
        
        LOG_INFO("All systems created successfully");
    }
    
    // Create and configure player entity
    static Entity init_player(
        World& world,
        SpriteManager& sprite_manager,
        const Position& spawn_pos,
        const GameConfig& config
    ) {
        LOG_INFO("=== Creating Player Entity ===");
        
        Entity player = world.create_entity();
        
        // Core components
        world.add_component(player, spawn_pos);
        world.add_component(player, SpriteBase{"player", "south"});
        world.add_component(player, Facing{Facing::SOUTH});
        world.add_component(player, sprite_manager.create_renderable("player.south"));
        world.add_component(player, PlayerControlled{});
        world.add_component(player, BlocksMovement{});
        
        // Stats
        world.add_component(player, Name{"Player"});
        world.add_component(player, CombatStats{
            5,  // attack
            1,  // defense
            config.gameplay.player_starting_hp
        });
        world.add_component(player, Health{
            config.gameplay.player_starting_hp,
            config.gameplay.player_starting_hp
        });
        
        // Progression
        world.add_component(player, Energy{100});
        world.add_component(player, Experience{1, 0});
        
        // Magic
        world.add_component(player, Intelligence{10});
        world.add_component(player, Mana{config.gameplay.player_starting_mp, 5});
        world.add_component(player, SpellBook{});
        
        // Inventory
        world.add_component(player, Inventory{});
        world.add_component(player, Equipment{});
        
        LOG_INFO("Player entity created with ID: " + std::to_string(player));
        return player;
    }
    
    // Give player starting spells
    static void init_player_spells(
        Entity player,
        World& world,
        MagicSystem* magic_system
    ) {
        LOG_INFO("=== Teaching Starting Spells ===");
        
        auto& components = world.get_component_manager();
        
        magic_system->learn_spell(components, player, "magic_missile");
        magic_system->learn_spell(components, player, "minor_heal");
        
        SpellBook* spellbook = world.get_component<SpellBook>(player);
        if (spellbook) {
            spellbook->equip_to_slot(0, 0);  // Magic Missile in slot 1
            spellbook->equip_to_slot(1, 1);  // Minor Heal in slot 2
            LOG_INFO("Starting spells equipped");
        }
    }
    
    // Send initial messages
    static void send_welcome_messages(MessageLog& message_log, int depth) {
        message_log.add_success("Welcome to the dungeon!");
        message_log.add_info("Dungeon Level " + std::to_string(depth));
        message_log.add_info("Use arrow keys to move. Press ESC to quit.");
    }
};