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
    Entity player
)
    : renderer(renderer),
    ui_layout(ui_layout),
    world(world),
    minimap(minimap),
    hotbar(hotbar),
    inventory_panel(inventory_panel),
    health_bar(health_bar),
    message_log(message_log),
    player(player)
{
    LOG_INFO("HudRenderer initialized");
}

// ============================================================================
// render_backgrounds — call before world->update() so panels sit behind world
// ============================================================================

void HudRenderer::render_backgrounds() {
    render_top_bar_background();
    render_minimap_background();
    render_message_log_background();
    render_hotbar_background();
}

// ============================================================================
// render_elements — call after world->update() so HUD sits on top
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

    // Health bar — fetch live health data from ECS
    if (health_bar && world) {
        Health* player_health = world->get_component<Health>(player);
        if (player_health) {
            health_bar->render(player_health->current, player_health->maximum);
        }
    }

    // Inventory panel (only when visible)
    if (inventory_panel && inventory_panel->is_visible()) {
        inventory_panel->render(player);
    }
}

// ============================================================================
// Private helpers — panel backgrounds
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