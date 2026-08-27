// src/ui/HudRenderer.cpp
// Phase 3: HUD rendering extracted from Game class

#include "HudRenderer.h"
#include "../ui/UILayout.h"
#include "../ui/Minimap.h"
#include "../ui/UnifiedHotbar.h"
#include "../ui/InventoryPanel.h"
#include "../ui/HealthBar.h"
#include "../ui/MessageLog.h"
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../utils/Logger.h"

// ============================================================================
// Constructor
// ============================================================================

HudRenderer::HudRenderer(
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
)
    : renderer(renderer),
    ui_layout(ui_layout),
    world(world),
    minimap(minimap),
    hotbar(hotbar),
    inventory_panel(inventory_panel),
    health_bar(health_bar),
    message_log(message_log),
    player(player),
    title_font(title_font),
    ui_font(ui_font)
{
    LOG_INFO("HudRenderer initialized");
}

// ============================================================================
// render_backgrounds � call before world->update() so panels sit behind world
// ============================================================================

void HudRenderer::render_backgrounds() {
    render_top_bar_background();
    render_minimap_background();
    render_message_log_background();
    render_hotbar_background();
}

// ============================================================================
// render_elements � call after world->update() so HUD sits on top
// ============================================================================

void HudRenderer::render_elements() {
    // Minimap
    if (minimap) {
        minimap->render();
    }

    // Message log
    if (message_log) {
        message_log->render();
    }

    // Hotbar
    if (hotbar) {
        hotbar->render(player);
    }

    // Health bar � CombatStats is the single source of truth for HP (combat,
    // potions, and spells all mutate current_hp/max_hp directly; see
    // CLAUDE.md for why there's no separate Health component to read here).
    if (health_bar && world) {
        CombatStats* player_stats = world->get_component<CombatStats>(player);
        if (player_stats) {
            health_bar->render(player_stats->current_hp, player_stats->max_hp);
        }
    }

    // Inventory panel (only when visible)
    if (inventory_panel && inventory_panel->is_visible()) {
        inventory_panel->render(player);
    }
}

// ============================================================================
// render_end_screen � full-screen game-over / victory overlay
// ============================================================================

void HudRenderer::render_end_screen(bool victory) {
    if (!ui_layout) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_FRect overlay = {
        0.0f, 0.0f,
        static_cast<float>(ui_layout->screen_width),
        static_cast<float>(ui_layout->screen_height)
    };
    SDL_RenderFillRect(renderer, &overlay);

    int center_x = ui_layout->screen_width / 2;
    int center_y = ui_layout->screen_height / 2;

    SDL_Color headline_color = victory
        ? SDL_Color{ 255, 215, 0, 255 }
        : SDL_Color{ 210, 60, 60, 255 };
    render_text_centered(title_font, victory ? "VICTORY!" : "YOU DIED",
        center_x, center_y - 40, headline_color);
    render_text_centered(ui_font, "Press R to play again, or ESC to quit",
        center_x, center_y + 30, SDL_Color{ 220, 220, 220, 255 });
}

void HudRenderer::render_text_centered(TTF_Font* font, const std::string& text, int center_x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_FRect dest = {
            static_cast<float>(center_x - surface->w / 2),
            static_cast<float>(y),
            static_cast<float>(surface->w),
            static_cast<float>(surface->h)
        };
        SDL_RenderTexture(renderer, texture, nullptr, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

// ============================================================================
// Private helpers � panel backgrounds
// ============================================================================

void HudRenderer::render_top_bar_background() {
    // Fill
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_FRect rect = {
        static_cast<float>(ui_layout->top_bar.x),
        static_cast<float>(ui_layout->top_bar.y),
        static_cast<float>(ui_layout->top_bar.w),
        static_cast<float>(ui_layout->top_bar.h)
    };
    SDL_RenderFillRect(renderer, &rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderRect(renderer, &rect);
}

void HudRenderer::render_minimap_background() {
    // Fill
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
    SDL_FRect rect = {
        static_cast<float>(ui_layout->minimap.x),
        static_cast<float>(ui_layout->minimap.y),
        static_cast<float>(ui_layout->minimap.w),
        static_cast<float>(ui_layout->minimap.h)
    };
    SDL_RenderFillRect(renderer, &rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderRect(renderer, &rect);
}

void HudRenderer::render_message_log_background() {
    // Fill
    SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
    SDL_FRect rect = {
        static_cast<float>(ui_layout->message_log.x),
        static_cast<float>(ui_layout->message_log.y),
        static_cast<float>(ui_layout->message_log.w),
        static_cast<float>(ui_layout->message_log.h)
    };
    SDL_RenderFillRect(renderer, &rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 70, 70, 90, 255);
    SDL_RenderRect(renderer, &rect);
}

void HudRenderer::render_hotbar_background() {
    // Fill
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_FRect rect = {
        static_cast<float>(ui_layout->hotbar.x),
        static_cast<float>(ui_layout->hotbar.y),
        static_cast<float>(ui_layout->hotbar.w),
        static_cast<float>(ui_layout->hotbar.h)
    };
    SDL_RenderFillRect(renderer, &rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderRect(renderer, &rect);
}