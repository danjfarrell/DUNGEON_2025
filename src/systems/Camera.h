// ============================================================================
// Camera.h - NEW FILE
// ============================================================================
#pragma once

class Camera {
private:
    int x;              // Camera position in world coordinates (pixels)
    int y;
    //int screen_width;   // Viewport size
    //int screen_height;
    int viewport_width;   // Changed from screen_width
    int viewport_height;  // Changed from screen_height
    int map_width;      // Map boundaries (in pixels)
    int map_height;
    int tile_size;      // Tile size in pixels

public:
    Camera(int viewport_w, int viewport_h, int map_w, int map_h, int tile_sz)
        : x(0), y(0),
        viewport_width(viewport_w),
        viewport_height(viewport_h),
        map_width(map_w* tile_sz),
        map_height(map_h* tile_sz),
        tile_size(tile_sz) {
    }

    // Center camera on a world position (in tile coordinates)
    void center_on(int tile_x, int tile_y) {
        // Convert tile coordinates to pixel coordinates
        int pixel_x = tile_x * tile_size;
        int pixel_y = tile_y * tile_size;

        // Center the camera on this position
        x = pixel_x - viewport_width / 2;
        y = pixel_y - viewport_height / 2;

        // Clamp to map boundaries
        clamp_to_bounds();
    }

    // Smoothly move camera towards target (optional smooth following)
    void smooth_follow(int tile_x, int tile_y, float lerp_factor = 0.1f) {
        int target_x = tile_x * tile_size - viewport_width / 2;
        int target_y = tile_y * tile_size - viewport_height / 2;

        // Lerp towards target
        x += static_cast<int>((target_x - x) * lerp_factor);
        y += static_cast<int>((target_y - y) * lerp_factor);

        clamp_to_bounds();
    }

    // Get camera offset for rendering
    int get_x() const { return x; }
    int get_y() const { return y; }

    // Convert world coordinates to screen coordinates
    int world_to_screen_x(int world_x) const { return world_x - x; }
    int world_to_screen_y(int world_y) const { return world_y - y; }

    // Check if a tile is visible on screen
    bool is_tile_visible(int tile_x, int tile_y) const {
        int pixel_x = tile_x * tile_size;
        int pixel_y = tile_y * tile_size;

        return pixel_x + tile_size >= x &&
            pixel_x < x + viewport_width &&
            pixel_y + tile_size >= y &&
            pixel_y < y + viewport_height;
    }

private:
    void clamp_to_bounds() {
        // Don't scroll past left/top edges
        if (x < 0) x = 0;
        if (y < 0) y = 0;

        // Don't scroll past right/bottom edges
        if (x + viewport_width > map_width) {
            x = map_width - viewport_height;
            if (x < 0) x = 0;  // Handle small maps
        }
        if (y + viewport_height > map_height) {
            y = map_height - viewport_height;
            if (y < 0) y = 0;
        }
    }
};