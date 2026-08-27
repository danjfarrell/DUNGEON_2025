// src/core/Game.cpp
// Phase 4 Final: Game is now a pure coordinator
// All responsibilities delegated to subsystems

#include "Game.h"
#include "GameBootstrap.h"
#include "../utils/Logger.h"

#include "../ecs/World.h"
#include "../graphics/SpriteManager.h"
#include "../systems/Camera.h"
#include "../world/Map.h"
#include "../world/DungeonManager.h"
#include "../world/TileVisibility.h"
#include "../systems/TurnManager.h"
#include "../systems/CombatSystem.h"
#include "../systems/MagicSystem.h"
#include "../systems/ExperienceSystem.h"
#include "../systems/StairSystem.h"
#include "../systems/ConsumableSystem.h"
#include "../systems/InputController.h"
#include "../systems/LevelTransitionSystem.h"   // Phase 4
#include "../ui/MessageLog.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../ui/HealthBar.h"
#include "../ui/HudRenderer.h"
#include "../components/Components.h"

// ============================================================================
// Constructor & Destructor
// ============================================================================

Game::Game()
    : running(false), player(0), current_map(nullptr),
    combat_system(nullptr), magic_system(nullptr), xp_system(nullptr),
    stair_system(nullptr), consumable_system(nullptr),
    map_render_system(nullptr), render_system(nullptr), tile_vis(nullptr)
{
}

Game::~Game() {
    LOG_INFO("=== Game shutting down ===");
    // tile_vis is a non-owning alias into World's std::unique_ptr<TileVisibility>
    // (see World::tile_visibility). World owns the object and frees it in its own
    // destructor, so do NOT delete it here — doing so caused a double free on every
    // clean exit (and after every level transition, since rebuild_tile_visibility()
    // used to replace it via raw new/delete behind World's back too).
}

// ============================================================================
// Initialization
// ============================================================================

bool Game::initialize(unsigned int seed) {
    auto result = GameBootstrap::initialize(seed);
    if (!result.success) {
        LOG_ERROR("Game initialization failed: " + result.error_message);
        return false;
    }

    // Absorb bootstrap result
    config = std::move(result.config);
    resources = std::move(result.resources);
    sprite_manager = std::move(result.sprite_manager);
    enemy_data = std::move(result.enemy_data);
    dungeon_manager = std::move(result.dungeon_manager);
    enemy_spawner = std::move(result.enemy_spawner);
    world = std::move(result.world);
    player = result.player;
    current_map = result.current_map;
    turn_manager = std::move(result.turn_manager);
    combat_system = result.combat_system;
    magic_system = result.magic_system;
    xp_system = result.xp_system;
    stair_system = result.stair_system;
    consumable_system = result.consumable_system;
    map_render_system = result.map_render_system;
    render_system = result.render_system;
    camera = std::move(result.camera);
    ui_layout = std::move(result.ui_layout);
    message_log = std::move(result.message_log);
    minimap = std::move(result.minimap);
    hotbar = std::move(result.hotbar);
    inventory_panel = std::move(result.inventory_panel);
    health_bar = std::move(result.health_bar);
    tile_vis = result.tile_vis;

    // Phase 2: InputController
    input_controller = std::make_unique<InputController>(
        world.get(), current_map, turn_manager.get(),
        combat_system, magic_system, stair_system, consumable_system,
        minimap.get(), inventory_panel.get(), message_log.get(),
        camera.get(), tile_vis, &config, player
    );

    // Phase 3: HudRenderer
    hud_renderer = std::make_unique<HudRenderer>(
        resources->renderer.get(), ui_layout.get(), world.get(),
        minimap.get(), hotbar.get(), inventory_panel.get(),
        health_bar.get(), message_log.get(), player,
        resources->title_font.get(), resources->ui_font.get()
    );

    // Phase 4: LevelTransitionSystem
    level_transition = std::make_unique<LevelTransitionSystem>(
        world.get(), dungeon_manager.get(), enemy_spawner.get(),
        stair_system, map_render_system, render_system,
        camera.get(), minimap.get(), message_log.get(),
        input_controller.get(), &config, player
    );
    level_transition->set_hud_renderer(hud_renderer.get());

    LOG_INFO("=== Game initialization complete ===");
    return true;
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

        // Input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { running = false; break; }

            if (state != GameState::PLAYING) {
                // The run has ended — only restart/quit mean anything now.
                // Route around InputController entirely so movement, spells,
                // and the inventory panel can't act on a finished run.
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) { running = false; break; }
                    if (event.key.key == SDLK_R) { wants_restart = true; running = false; break; }
                }
                continue;
            }

            auto result = input_controller->handle_event(event);
            if (result.quit_requested) { running = false; break; }
            if (result.turn_ended) { turn_manager->end_player_turn(); }
        }

        update();
        render();

        Uint64 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < frame_time) {
            SDL_Delay(static_cast<Uint32>(frame_time - elapsed));
        }
    }

    LOG_INFO("=== Game loop ended ===");
}

// ============================================================================
// Update
// ============================================================================

void Game::update() {
    if (state != GameState::PLAYING) {
        return;  // Frozen on the game-over/victory screen until restart or quit.
    }

    // Level transitions (checks stair trigger internally)
    if (level_transition->check_and_execute(current_map, tile_vis)) {
        // Pointers updated in-place � nothing else to do here
    }

    // Enemy turns
    if (turn_manager->is_enemy_turn()) {
        turn_manager->process_turn(*world);
        world->update(0.016f);
        if (magic_system) {
            // Regenerate mana exactly once per completed turn (see MagicSystem::update).
            magic_system->regenerate_mana(world->get_component_manager());
        }
        turn_manager->end_enemy_turn();
    }

    // Player death: CombatStats.current_hp is the single source of truth for
    // HP (see GameInitializer.h) — combat, potions, and spells all mutate it.
    // CombatSystem::handle_death() tags the player Dead without stripping
    // their other components, so the death screen can still show where they
    // fell.
    CombatStats* stats = world->get_component<CombatStats>(player);
    if (stats && !stats->is_alive()) {
        state = GameState::GAME_OVER;
        message_log->add_combat("You have died!");
        message_log->add_info("Press R to play again, or ESC to quit.");
        return;
    }

    // Victory: survive to the configured depth.
    if (dungeon_manager && dungeon_manager->get_current_depth() >= config.gameplay.victory_depth) {
        state = GameState::VICTORY;
        message_log->add_success("You reached depth " +
            std::to_string(config.gameplay.victory_depth) + " and won!");
        message_log->add_info("Press R to play again, or ESC to quit.");
    }
}

// ============================================================================
// Render
// ============================================================================

void Game::render() {
    auto* sdl_renderer = resources->renderer.get();

    SDL_SetRenderDrawColor(sdl_renderer, 20, 20, 30, 255);
    SDL_RenderClear(sdl_renderer);

    hud_renderer->render_backgrounds();
    world->update(0.016f);          // Always render � map-disappear bug is fixed
    hud_renderer->render_elements();

    if (state != GameState::PLAYING) {
        hud_renderer->render_end_screen(state == GameState::VICTORY);
    }

    SDL_RenderPresent(sdl_renderer);
}