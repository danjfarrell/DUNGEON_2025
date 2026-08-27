// src/core/GameInitializer.h
#pragma once

#include "../ecs/World.h"
#include "../config/GameConfig.h"
#include "../graphics/SpriteManager.h"
#include "../data/EnemyData.h"
#include "../data/EquipmentDatabase.h"
#include "../world/DungeonManager.h"
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../systems/TurnManager.h"
#include "../systems/MagicSystem.h"
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
            return { false, "Failed to load sprite configuration: " + config.assets.sprite_config };
        }

        sprite_manager.dump_config_to_log();
        LOG_INFO("Sprite system initialized successfully");
        return { true, "" };
    }

    // Initialize enemy data
    static InitResult init_enemy_data(EnemyDataManager& enemy_data, const GameConfig& config) {
        LOG_INFO("=== Loading Enemy Data ===");

        if (!enemy_data.load_from_file(config.assets.enemy_data)) {
            LOG_WARN("Failed to load enemy data from: " + config.assets.enemy_data);
            return { false, "Enemy data loading failed (continuing anyway)" };
        }

        LOG_INFO("Enemy data loaded successfully");
        return { true, "" };
    }

    // Initialize message log with font
    static InitResult init_message_log(MessageLog& message_log, const GameConfig& config) {
        LOG_INFO("=== Initializing Message Log ===");

        // Try primary font
        if (message_log.init_font(config.assets.font_path, config.assets.font_size)) {
            LOG_INFO("Loaded font: " + config.assets.font_path);
            return { true, "" };
        }

        // Try fallback font
        if (!config.assets.font_fallback.empty()) {
            if (message_log.init_font(config.assets.font_fallback, config.assets.font_size)) {
                LOG_INFO("Loaded fallback font: " + config.assets.font_fallback);
                return { true, "" };
            }
        }

        LOG_WARN("Could not load any font - message log will not render");
        return { false, "Font loading failed (continuing without text)" };
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
        world.add_component(player, SpriteBase{ "player", "south" });
        world.add_component(player, Facing{ Facing::SOUTH });
        world.add_component(player, sprite_manager.create_renderable("player.south"));
        world.add_component(player, PlayerControlled{});
        world.add_component(player, BlocksMovement{});

        // Stats
        world.add_component(player, Name{ "Player" });
        world.add_component(player, CombatStats{
            5,  // attack
            1,  // defense
            config.gameplay.player_starting_hp
            });
        // Note: no separate Health component. CombatStats.current_hp/max_hp is
        // the single source of truth for HP (combat, potions, and spells all
        // mutate it) — a duplicate Health component used to exist and drift
        // out of sync with it; see CLAUDE.md.

        // Progression
        world.add_component(player, Energy{ 100 });
        world.add_component(player, Experience{ 1, 0 });

        // Magic
        world.add_component(player, Intelligence{ 10 });
        world.add_component(player, Mana{ config.gameplay.player_starting_mp, 5 });
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

    // Give the player a starting weapon, already equipped, so the equip
    // system (mechanism + UI) has something to show/verify without needing
    // a lucky loot drop first. See EquipmentDatabase.h for the catalog.
    static void init_player_equipment(Entity player, World& world) {
        LOG_INFO("=== Equipping Starting Gear ===");

        static const EquipmentDatabase equipment_db;
        const EquipmentDefinition* dagger = equipment_db.get_item("rusty_dagger");
        if (!dagger) return;

        Entity weapon = world.create_entity();
        world.add_component(weapon, Name{ dagger->display_name });
        world.add_component(weapon, Item{ "rusty_dagger", 1 });
        world.add_component(weapon, dagger->item);

        Equipment* equipment = world.get_component<Equipment>(player);
        if (equipment) {
            equipment->equip(dagger->item.slot, weapon);
            LOG_INFO("Equipped starting weapon: " + dagger->display_name);
        }
    }

    // Send initial messages
    static void send_welcome_messages(MessageLog& message_log, int depth) {
        message_log.add_success("Welcome to the dungeon!");
        message_log.add_info("Dungeon Level " + std::to_string(depth));
        message_log.add_info("Use arrow keys to move. Press ESC to quit.");
    }
};