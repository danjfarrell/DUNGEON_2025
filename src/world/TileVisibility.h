// ============================================================================
// TileVisibility.h - NEW FILE (src/world/TileVisibility.h)
// ============================================================================
#pragma once

#include <vector>

enum class VisibilityState {
    UNEXPLORED,  // Never seen (render as black or don't render)
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

    bool TileVisibility::is_unexplored(int x, int y) const {
        return get(x, y) == VisibilityState::UNEXPLORED;
    }

    // Check if tile is currently visible
    bool is_visible(int x, int y) const {
        return get(x, y) == VisibilityState::VISIBLE;
    }

    // Mark tile as visible (in current FOV)
    void set_visible(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y][x] = VisibilityState::VISIBLE;
        }
    }

    // Mark tile as explored but not visible (was in FOV, now isn't)
    void set_explored(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (tiles[y][x] != VisibilityState::UNEXPLORED) {
                tiles[y][x] = VisibilityState::EXPLORED;
            }
        }
    }

    // Clear all visible tiles to explored (call at start of each turn)
    void clear_visible() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (tiles[y][x] == VisibilityState::VISIBLE) {
                    tiles[y][x] = VisibilityState::EXPLORED;
                }
            }
        }
    }

    // Temporary: Reveal entire map (for testing without FOV)
    void reveal_all() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                tiles[y][x] = VisibilityState::VISIBLE;
            }
        }
    }

    // Update FOV (when you implement FOV system later)
    void update_fov(int player_x, int player_y, int view_radius) {
        // First, mark all currently visible tiles as just explored
        clear_visible();

        // Simple circle FOV (replace with shadowcasting later)
        for (int dy = -view_radius; dy <= view_radius; dy++) {
            for (int dx = -view_radius; dx <= view_radius; dx++) {
                int dist_squared = dx * dx + dy * dy;
                if (dist_squared <= view_radius * view_radius) {
                    int tile_x = player_x + dx;
                    int tile_y = player_y + dy;
                    set_visible(tile_x, tile_y);
                }
            }
        }
    }

    int get_width() const { return width; }
    int get_height() const { return height; }
};
