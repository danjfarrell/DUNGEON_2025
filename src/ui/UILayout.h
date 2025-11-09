// ============================================================================
// UILayout.h - NEW FILE (src/ui/UILayout.h)
// ============================================================================
#pragma once

#include <SDL3/SDL.h>

// Defines the different sections of the screen
struct UILayout {
    // Screen dimensions
    int screen_width;
    int screen_height;

    // Game viewport (where the actual game renders)
    SDL_Rect game_viewport;

    // UI panels
    SDL_Rect top_bar;         // Health, stats
    SDL_Rect minimap;         // Top-right minimap
    SDL_Rect message_log;     // Bottom message area
    SDL_Rect hotbar;          // Bottom-center hotbar
    SDL_Rect sidebar;         // Optional left/right sidebar

    // Initialize with screen size
    UILayout(int width, int height)
        : screen_width(width), screen_height(height) {
        calculate_layout();
    }

    void calculate_layout() {
        // Top bar (health, stats) - 80 pixels high for larger screen
        top_bar = {
            0,                  // x
            0,                  // y
            screen_width - 260, // width (leave room for minimap)
            80                  // height
        };

        // Minimap (top-right corner) - 250x250 for larger screen
        minimap = {
            screen_width - 260, // x (10px from right)
            10,                 // y (10px from top)
            250,                // width
            250                 // height
        };

        // Message log (bottom) - 180 pixels high
        message_log = {
            10,                      // x (10px from left)
            screen_height - 190,     // y (10px from bottom)
            screen_width - 20,       // width (20px margin)
            180                      // height
        };

        // Hotbar (bottom, above message log) - 60 pixels high
        hotbar = {
            screen_width / 2 - 180,  // x (centered, 6 slots * 60px)
            screen_height - 250,      // y (above message log)
            360,                      // width (6 slots * 60px)
            50                        // height
        };

        // Game viewport (the actual play area)
        // Left side of screen, everything not occupied by UI
        game_viewport = {
            0,                       // x
            top_bar.h,               // y (below top bar)
            screen_width - 260,      // width (leave room for minimap)
            screen_height - top_bar.h - 260  // height (above hotbar+messages)
        };
    }

    // Check if position is inside game viewport
    bool is_in_game_area(int x, int y) const {
        return x >= game_viewport.x &&
            x < game_viewport.x + game_viewport.w &&
            y >= game_viewport.y &&
            y < game_viewport.y + game_viewport.h;
    }

    // Convert screen coordinates to game viewport coordinates
    void screen_to_viewport(int screen_x, int screen_y,
        int& viewport_x, int& viewport_y) const {
        viewport_x = screen_x - game_viewport.x;
        viewport_y = screen_y - game_viewport.y;
    }
};