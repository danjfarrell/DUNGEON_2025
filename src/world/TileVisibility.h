// src/world/TileVisibility.h
#pragma once

#include <vector>
#include <cmath>

enum class VisibilityState {
    UNEXPLORED,  // Never seen (don't render)
    EXPLORED,    // Seen before but not visible now (render dimmed)
    VISIBLE      // Currently visible (render bright)
};

class TileVisibility {
private:
    int width, height;
    std::vector<std::vector<VisibilityState>> tiles;

public:
    TileVisibility(int w, int h) : width(w), height(h) {
        tiles.resize(height);
        for (int y = 0; y < height; y++) {
            tiles[y].resize(width, VisibilityState::UNEXPLORED);
        }
    }

    // Get visibility state of a tile
    VisibilityState get(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return VisibilityState::UNEXPLORED;
        }
        return tiles[y][x];
    }

    // Check if tile has ever been explored
    bool is_explored(int x, int y) const {
        VisibilityState state = get(x, y);
        return state == VisibilityState::EXPLORED || state == VisibilityState::VISIBLE;
    }

    // Check if tile is currently visible
    bool is_visible(int x, int y) const {
        return get(x, y) == VisibilityState::VISIBLE;
    }

    // Check if tile is unexplored
    bool is_unexplored(int x, int y) const {
        return get(x, y) == VisibilityState::UNEXPLORED;
    }

    // Mark tile as visible (in current FOV)
    void set_visible(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y][x] = VisibilityState::VISIBLE;
        }
    }

    // Mark tile as explored but not visible
    void set_explored(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (tiles[y][x] != VisibilityState::UNEXPLORED) {
                tiles[y][x] = VisibilityState::EXPLORED;
            }
        }
    }

    // Clear all visible tiles to explored (call at start of each FOV update)
    void clear_visible() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (tiles[y][x] == VisibilityState::VISIBLE) {
                    tiles[y][x] = VisibilityState::EXPLORED;
                }
            }
        }
    }

    // Reveal entire map (for testing)
    void reveal_all() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                tiles[y][x] = VisibilityState::VISIBLE;
            }
        }
    }

    // Update FOV using simple circular algorithm
    // You can replace this with shadowcasting later for line-of-sight
    void update_fov(int player_x, int player_y, int view_radius) {
        // First, mark all currently visible tiles as just explored
        clear_visible();

        // Simple circle FOV (sees through walls - good for start)
        for (int dy = -view_radius; dy <= view_radius; dy++) {
            for (int dx = -view_radius; dx <= view_radius; dx++) {
                // Check if within circular radius
                int dist_squared = dx * dx + dy * dy;
                if (dist_squared <= view_radius * view_radius) {
                    int tile_x = player_x + dx;
                    int tile_y = player_y + dy;
                    set_visible(tile_x, tile_y);
                }
            }
        }
    }

    // Advanced: Shadowcasting FOV (line-of-sight, blocked by walls)
    // This is more complex but gives proper roguelike visibility
    void update_fov_shadowcast(int player_x, int player_y, int view_radius,
        bool (*is_blocking)(int, int)) {
        clear_visible();

        // Player can always see their own tile
        set_visible(player_x, player_y);

        // Cast shadows in 8 octants
        for (int i = 0; i < 8; i++) {
            cast_light(player_x, player_y, 1, 1.0, 0.0, view_radius,
                i, is_blocking);
        }
    }

    int get_width() const { return width; }
    int get_height() const { return height; }

private:
    // Shadowcasting helper (recursive octant scanning)
    void cast_light(int cx, int cy, int row, float start, float end,
        int radius, int octant, bool (*is_blocking)(int, int)) {
        if (start < end) return;

        float new_start = 0.0f;

        for (int i = row; i <= radius; i++) {
            int dx = -i - 1;
            int dy = -i;
            bool blocked = false;

            while (dx <= 0) {
                dx++;

                // Transform coordinates based on octant
                int mx, my;
                transform_octant(octant, dx, dy, &mx, &my);
                int map_x = cx + mx;
                int map_y = cy + my;

                // Calculate slopes
                float l_slope = (dx - 0.5f) / (dy + 0.5f);
                float r_slope = (dx + 0.5f) / (dy - 0.5f);

                if (start < r_slope) {
                    continue;
                }
                else if (end > l_slope) {
                    break;
                }

                // Check if within radius
                if (dx * dx + dy * dy < radius * radius) {
                    set_visible(map_x, map_y);
                }

                if (blocked) {
                    if (is_blocking(map_x, map_y)) {
                        new_start = r_slope;
                        continue;
                    }
                    else {
                        blocked = false;
                        start = new_start;
                    }
                }
                else {
                    if (is_blocking(map_x, map_y) && i < radius) {
                        blocked = true;
                        cast_light(cx, cy, i + 1, start, l_slope, radius,
                            octant, is_blocking);
                        new_start = r_slope;
                    }
                }
            }

            if (blocked) break;
        }
    }

    // Transform dx, dy based on which octant we're scanning
    void transform_octant(int octant, int dx, int dy, int* mx, int* my) {
        switch (octant) {
        case 0: *mx = dx; *my = dy; break;
        case 1: *mx = dy; *my = dx; break;
        case 2: *mx = -dy; *my = dx; break;
        case 3: *mx = -dx; *my = dy; break;
        case 4: *mx = -dx; *my = -dy; break;
        case 5: *mx = -dy; *my = -dx; break;
        case 6: *mx = dy; *my = -dx; break;
        case 7: *mx = dx; *my = -dy; break;
        }
    }
};