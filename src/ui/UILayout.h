// ============================================================================
// UILayout.h - NEW FILE (src/ui/UILayout.h)
// ============================================================================
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstring>
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

    // Add this method to UILayout.h (inside the struct):

// Debug: Render colored borders around all UI regions
    void render_debug_borders(SDL_Renderer* renderer) const {
        // Helper lambda to draw a thick border
        auto draw_thick_border = [renderer](const SDL_Rect& rect, SDL_Color color, int thickness = 3) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

            for (int i = 0; i < thickness; i++) {
                SDL_FRect border = {
                    static_cast<float>(rect.x + i),
                    static_cast<float>(rect.y + i),
                    static_cast<float>(rect.w - i * 2),
                    static_cast<float>(rect.h - i * 2)
                };
                SDL_RenderRect(renderer, &border);
            }
            };

        // Game viewport - BRIGHT GREEN
        draw_thick_border(game_viewport, { 0, 255, 0, 255 }, 4);

        // Top bar (player info) - CYAN
        draw_thick_border(top_bar, { 0, 255, 255, 255 }, 3);

        // Minimap - MAGENTA
        draw_thick_border(minimap, { 255, 0, 255, 255 }, 3);

        // Message log - YELLOW
        draw_thick_border(message_log, { 255, 255, 0, 255 }, 3);

        // Hotbar - ORANGE
        draw_thick_border(hotbar, { 255, 128, 0, 255 }, 3);

        // Optional: Draw crosshairs at viewport origin
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        // Horizontal line at y=80 (viewport top)
        SDL_RenderLine(renderer,
            game_viewport.x,
            game_viewport.y,
            game_viewport.x + game_viewport.w,
            game_viewport.y);

        // Vertical line at x=0 (viewport left)
        SDL_RenderLine(renderer,
            game_viewport.x,
            game_viewport.y,
            game_viewport.x,
            game_viewport.y + game_viewport.h);
    }

    // Optional: Render labels for each section
    void render_debug_labels(SDL_Renderer* renderer, TTF_Font* font) const {
        if (!font) return;

        auto render_label = [renderer, font](const char* text, int x, int y, SDL_Color color) {
            SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    SDL_FRect dest = {
                        static_cast<float>(x),
                        static_cast<float>(y),
                        static_cast<float>(surface->w),
                        static_cast<float>(surface->h)
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &dest);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
            };

        // Labels
        render_label("GAME VIEWPORT", game_viewport.x + 10, game_viewport.y + 10, { 0, 255, 0, 255 });
        render_label("PLAYER INFO", top_bar.x + 10, top_bar.y + 10, { 0, 255, 255, 255 });
        render_label("MINIMAP", minimap.x + 10, minimap.y + 10, { 255, 0, 255, 255 });
        render_label("MESSAGE LOG", message_log.x + 10, message_log.y + 10, { 255, 255, 0, 255 });
        render_label("HOTBAR", hotbar.x + 10, hotbar.y + 10, { 255, 128, 0, 255 });

        // Coordinate labels
        char coord_text[64];
        snprintf(coord_text, sizeof(coord_text), "Origin: (%d, %d)", game_viewport.x, game_viewport.y);
        render_label(coord_text, game_viewport.x + 10, game_viewport.y + 30, { 255, 0, 0, 255 });

        snprintf(coord_text, sizeof(coord_text), "Size: %dx%d", game_viewport.w, game_viewport.h);
        render_label(coord_text, game_viewport.x + 10, game_viewport.y + 50, { 255, 0, 0, 255 });
    }
    // ========================================================================
// PLACEHOLDER CONTENT RENDERING
// ========================================================================

// Render placeholder content for all UI sections
    void render_placeholders(SDL_Renderer* renderer, TTF_Font* font) const {
        if (!font) return;

        //render_top_bar_placeholder(renderer, font);
        //render_minimap_placeholder(renderer, font);
        render_hotbar_placeholder(renderer, font);
    }

private:
    // Helper to render centered text
    void render_text_centered(SDL_Renderer* renderer, TTF_Font* font,
        const char* text, int x, int y, int width,
        SDL_Color color) const {
        SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                // Center horizontally
                int text_x = x + (width - surface->w) / 2;

                SDL_FRect dest = {
                    static_cast<float>(text_x),
                    static_cast<float>(y),
                    static_cast<float>(surface->w),
                    static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }

    // Helper to render left-aligned text
    void render_text(SDL_Renderer* renderer, TTF_Font* font,
        const char* text, int x, int y, SDL_Color color) const {
        SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_FRect dest = {
                    static_cast<float>(x),
                    static_cast<float>(y),
                    static_cast<float>(surface->w),
                    static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }

public:
    // Top bar - Player stats and information
    void render_top_bar_placeholder(SDL_Renderer* renderer, TTF_Font* font) const {
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Color red = { 255, 100, 100, 255 };
        SDL_Color blue = { 100, 150, 255, 255 };
        SDL_Color green = { 100, 255, 100, 255 };
        SDL_Color gold = { 255, 215, 0, 255 };

        int x = top_bar.x + 15;
        int y = top_bar.y + 10;
        int line_height = 20;

        // Player name
        render_text(renderer, font, "Hero the Brave", x, y, white);
        y += line_height;

        // Health bar placeholder
        render_text(renderer, font, "HP: ", x, y, white);
        render_text(renderer, font, "85/100", x + 40, y, red);

        // Draw a simple health bar
        SDL_FRect health_bg = {
            static_cast<float>(x + 120),
            static_cast<float>(y + 5),
            150.0f,
            10.0f
        };
        SDL_SetRenderDrawColor(renderer, 60, 20, 20, 255);
        SDL_RenderFillRect(renderer, &health_bg);

        SDL_FRect health_bar = {
            static_cast<float>(x + 120),
            static_cast<float>(y + 5),
            150.0f * 0.85f,  // 85% health
            10.0f
        };
        SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
        SDL_RenderFillRect(renderer, &health_bar);

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderRect(renderer, &health_bg);

        y += line_height;

        // Mana bar placeholder
        render_text(renderer, font, "MP: ", x, y, white);
        render_text(renderer, font, "45/60", x + 40, y, blue);

        // Draw a simple mana bar
        SDL_FRect mana_bg = {
            static_cast<float>(x + 120),
            static_cast<float>(y + 5),
            150.0f,
            10.0f
        };
        SDL_SetRenderDrawColor(renderer, 20, 30, 60, 255);
        SDL_RenderFillRect(renderer, &mana_bg);

        SDL_FRect mana_bar = {
            static_cast<float>(x + 120),
            static_cast<float>(y + 5),
            150.0f * 0.75f,  // 75% mana
            10.0f
        };
        SDL_SetRenderDrawColor(renderer, 50, 100, 200, 255);
        SDL_RenderFillRect(renderer, &mana_bar);

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderRect(renderer, &mana_bg);

        // Additional stats in columns
        int col2_x = x + 350;
        int stats_y = top_bar.y + 10;

        render_text(renderer, font, "Level: 5", col2_x, stats_y, gold);
        stats_y += line_height;
        render_text(renderer, font, "XP: 1250/2000", col2_x, stats_y, green);
        stats_y += line_height;
        render_text(renderer, font, "Gold: 347", col2_x, stats_y, gold);

        int col3_x = x + 550;
        stats_y = top_bar.y + 10;
        render_text(renderer, font, "Floor: Dungeon 1", col3_x, stats_y, white);
        stats_y += line_height;
        render_text(renderer, font, "Turn: 127", col3_x, stats_y, white);
    }

    // Minimap - Show placeholder map
    void render_minimap_placeholder(SDL_Renderer* renderer, TTF_Font* font) const {
        SDL_Color white = { 200, 200, 200, 255 };

        // Title
        render_text_centered(renderer, font, "MINIMAP",
            minimap.x, minimap.y + 5, minimap.w, white);

        // Draw a simple grid pattern as placeholder
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);

        int grid_size = 10;
        int map_area_y = minimap.y + 30;
        int map_area_h = minimap.h - 35;

        // Vertical lines
        for (int i = 0; i <= minimap.w; i += grid_size) {
            SDL_RenderLine(renderer,
                minimap.x + i, map_area_y,
                minimap.x + i, map_area_y + map_area_h);
        }

        // Horizontal lines
        for (int i = 0; i <= map_area_h; i += grid_size) {
            SDL_RenderLine(renderer,
                minimap.x, map_area_y + i,
                minimap.x + minimap.w, map_area_y + i);
        }

        // Draw a "player" dot in center
        SDL_FRect player_dot = {
            static_cast<float>(minimap.x + minimap.w / 2 - 3),
            static_cast<float>(map_area_y + map_area_h / 2 - 3),
            6.0f,
            6.0f
        };
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player_dot);

        // Draw some "rooms" as placeholder
        SDL_SetRenderDrawColor(renderer, 150, 150, 100, 255);
        SDL_FRect room1 = {
            static_cast<float>(minimap.x + 30),
            static_cast<float>(map_area_y + 40),
            40.0f,
            30.0f
        };
        SDL_RenderRect(renderer, &room1);

        SDL_FRect room2 = {
            static_cast<float>(minimap.x + 150),
            static_cast<float>(map_area_y + 100),
            50.0f,
            40.0f
        };
        SDL_RenderRect(renderer, &room2);
    }

    // Hotbar - Quick access items/abilities
    void render_hotbar_placeholder(SDL_Renderer* renderer, TTF_Font* font) const {
        SDL_Color white = { 200, 200, 200, 255 };
        SDL_Color gray = { 120, 120, 120, 255 };
        SDL_Color yellow = { 255, 255, 100, 255 };

        // Draw 6 hotbar slots
        int slot_size = 50;
        int slot_spacing = 60;
        int start_x = hotbar.x + (hotbar.w - (6 * slot_spacing)) / 2;
        int slot_y = hotbar.y + 5;

        const char* slot_labels[] = { "1", "2", "3", "4", "5", "6" };
        const char* slot_items[] = {
            "Sword",
            "Potion",
            "Spell",
            "[Empty]",
            "Torch",
            "[Empty]"
        };

        for (int i = 0; i < 6; i++) {
            int slot_x = start_x + (i * slot_spacing);

            // Draw slot background
            SDL_FRect slot_bg = {
                static_cast<float>(slot_x),
                static_cast<float>(slot_y),
                static_cast<float>(slot_size),
                static_cast<float>(slot_size)
            };

            // Highlight slot 1 as "selected"
            if (i == 0) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 50, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
            }
            SDL_RenderFillRect(renderer, &slot_bg);

            // Draw slot border
            if (i == 0) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 100, 255);  // Yellow for selected
            }
            else {
                SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
            }
            SDL_RenderRect(renderer, &slot_bg);

            // Draw number key
            render_text(renderer, font, slot_labels[i],
                slot_x + 5, slot_y + 5,
                i == 0 ? yellow : white);

            // Draw item name
            SDL_Color item_color = (strcmp(slot_items[i], "[Empty]") == 0) ? gray : white;

            // Center the item text in slot
            SDL_Surface* surface = TTF_RenderText_Blended(font, slot_items[i], 0, item_color);
            if (surface) {
                int text_x = slot_x + (slot_size - surface->w) / 2;
                int text_y = slot_y + 25;

                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    SDL_FRect dest = {
                        static_cast<float>(text_x),
                        static_cast<float>(text_y),
                        static_cast<float>(surface->w),
                        static_cast<float>(surface->h)
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &dest);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
        }

        // Instruction text below hotbar
        render_text_centered(renderer, font, "Press 1-6 to use items",
            hotbar.x, hotbar.y + slot_size + 10, hotbar.w, gray);
    }

};