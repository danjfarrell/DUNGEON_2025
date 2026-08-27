// src/ui/HudRenderer.h
// Phase 3: Extract HUD rendering from Game class
// Responsibilities: Render all UI backgrounds and overlay elements

#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "../ecs/Entity.h"

// Forward declarations
class World;
struct UILayout;
class Minimap;
class UnifiedHotbar;
class InventoryPanel;
class HealthBar;
class MessageLog;

// ============================================================================
// HudRenderer - Renders all HUD / UI overlay elements
// ============================================================================
// Single Responsibility: Draw UI panels, backgrounds, and overlay widgets
// Extracted from Game class to reduce God-object complexity
//
// Does NOT own any of the UI components � it only borrows pointers.
// Ownership remains with Game (via unique_ptr members).
// ============================================================================

class HudRenderer {
public:
    HudRenderer(
        SDL_Renderer* renderer,
        UILayout* ui_layout,
        World* world,
        Minimap* minimap,
        UnifiedHotbar* hotbar,
        InventoryPanel* inventory_panel,
        HealthBar* health_bar,
        MessageLog* message_log,
        Entity player,
        TTF_Font* title_font,
        TTF_Font* ui_font
    );

    // ----------------------------------------
    // Main render entry points
    // ----------------------------------------

    // Draw panel backgrounds (call before world renders)
    void render_backgrounds();

    // Draw all HUD elements on top of the game world
    void render_elements();

    // Full-screen game-over / victory overlay, drawn on top of everything
    // else once the run has ended (see Game::GameState).
    void render_end_screen(bool victory);

    // ----------------------------------------
    // Update player reference (e.g. after level transition)
    // ----------------------------------------
    void set_player(Entity new_player) { player = new_player; }

private:
    // Dependencies (non-owning)
    SDL_Renderer* renderer;
    UILayout* ui_layout;
    World* world;
    Minimap* minimap;
    UnifiedHotbar* hotbar;
    InventoryPanel* inventory_panel;
    HealthBar* health_bar;
    MessageLog* message_log;
    Entity player;
    TTF_Font* title_font;
    TTF_Font* ui_font;

    // Internal helpers
    void render_top_bar_background();
    void render_minimap_background();
    void render_message_log_background();
    void render_hotbar_background();
    void render_text_centered(TTF_Font* font, const std::string& text, int center_x, int y, SDL_Color color);
};
