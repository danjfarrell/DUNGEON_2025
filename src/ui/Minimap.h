// ============================================================================
// Minimap.h - NEW FILE (src/ui/Minimap.h)
// ============================================================================
#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include "../world/Map.h"
#include "../ecs/World.h"
#include "../components/Components.h"

class Minimap {
private:
    SDL_Renderer* renderer;
    Map* game_map;
    World* world;

    // Position and size on screen
    int x, y;           // Top-left corner
    int width, height;  // Size in pixels (250x250)

    // Map state tracking
    std::vector<std::vector<bool>> explored;  // Has player seen this tile?
    std::vector<std::vector<bool>> visible;   // Can player see it now? (FOV)

    // Visual settings
    int tile_size;      // Pixels per map tile (4x4 for now)
    bool show_fog;      // Show fog of war?
    bool show_entities; // Show enemies/items on minimap?

    // Colors for different tile types
    SDL_Color color_unexplored;
    SDL_Color color_wall_explored;
    SDL_Color color_floor_explored;
    SDL_Color color_wall_visible;
    SDL_Color color_floor_visible;
    SDL_Color color_player;
    SDL_Color color_enemy;
    SDL_Color color_item;
    SDL_Color color_stairs;
    SDL_Color color_door;
    SDL_Color color_border;

    // Camera tracking
    int camera_center_x, camera_center_y;  // Center position in map tiles

public:
    Minimap(SDL_Renderer* rend, Map* map, World* w,
        int pos_x, int pos_y, int w_pixels, int h_pixels)
        : renderer(rend), game_map(map), world(w),
        x(pos_x), y(pos_y), width(w_pixels), height(h_pixels),
        tile_size(4), show_fog(true), show_entities(true),
        camera_center_x(0), camera_center_y(0) {

        // Initialize explored/visible arrays
        explored.resize(map->get_height());
        visible.resize(map->get_height());
        for (int i = 0; i < map->get_height(); i++) {
            explored[i].resize(map->get_width(), false);
            visible[i].resize(map->get_width(), false);
        }

        // Set colors (4x4 pixels will show these clearly)
        color_unexplored = { 0, 0, 0, 255 };         // Black
        color_wall_explored = { 80, 80, 80, 255 };      // Dark gray
        color_floor_explored = { 40, 40, 40, 255 };     // Very dark gray
        color_wall_visible = { 180, 180, 180, 255 };   // Light gray
        color_floor_visible = { 120, 120, 100, 255 };   // Tan
        color_player = { 0, 255, 0, 255 };       // Bright green
        color_enemy = { 255, 0, 0, 255 };       // Bright red
        color_item = { 255, 255, 0, 255 };     // Bright yellow
        color_stairs = { 255, 128, 0, 255 };     // Orange
        color_door = { 150, 100, 50, 255 };    // Brown
        color_border = { 100, 100, 100, 255 };   // Gray
    }

    // Update visibility (call this when FOV system is added)
    void update_visibility(const std::vector<std::vector<bool>>& fov_data) {
        visible = fov_data;

        // Mark visible tiles as explored
        for (int y = 0; y < game_map->get_height(); y++) {
            for (int x = 0; x < game_map->get_width(); x++) {
                if (visible[y][x]) {
                    explored[y][x] = true;
                }
            }
        }
    }

    // Temporary: Mark everything as explored (until FOV is implemented)
    void reveal_all() {
        for (int y = 0; y < game_map->get_height(); y++) {
            for (int x = 0; x < game_map->get_width(); x++) {
                explored[y][x] = true;
                visible[y][x] = true;  // Everything visible for now
            }
        }
    }

    // Center minimap on position (call with player position)
    void center_on(int tile_x, int tile_y) {
        camera_center_x = tile_x;
        camera_center_y = tile_y;
    }

    // Get color for a tile
    SDL_Color get_tile_color(int map_x, int map_y, bool is_visible, bool is_explored) {
        // Check for player
        auto* positions = world->get_component_manager().get_array<Position>();
        auto* players = world->get_component_manager().get_array<PlayerControlled>();

        if (positions && players) {
            auto& player_entities = players->get_entities();
            for (Entity e : player_entities) {
                Position* pos = positions->get(e);
                if (pos && pos->x == map_x && pos->y == map_y) {
                    return color_player;
                }
            }
        }

        // Check for visible entities (enemies, items)
        if (is_visible && show_entities) {
            // Check for enemies
            auto* ais = world->get_component_manager().get_array<BlocksMovement>();
            if (ais && positions) {
                auto& ai_entities = ais->get_entities();
                for (Entity e : ai_entities) {
                    Position* pos = positions->get(e);
                    if (pos && pos->x == map_x && pos->y == map_y) {
                        return color_enemy;
                    }
                }
            }

            // Check for items (if you have DroppedItem component)
            // auto* items = world->get_component_manager().get_array<DroppedItem>();
            // if (items && positions) { ... }
        }

        // Get tile type
        TileType tile = game_map->get_tile(map_x, map_y);

        // Special tiles
        if (tile == TileType::STAIRS_DOWN || tile == TileType::STAIRS_UP) {
            return color_stairs;
        }

        if (tile == TileType::DOOR_CLOSED || tile == TileType::DOOR_OPEN) {
            return color_door;
        }

        // Walls and floors
        if (tile == TileType::WALL) {
            return is_visible ? color_wall_visible : color_wall_explored;
        }

        if (tile == TileType::FLOOR) {
            return is_visible ? color_floor_visible : color_floor_explored;
        }

        // Unexplored (VOID)
        return color_unexplored;
    }

    // Render the minimap
    void render() {
        // Draw background
        SDL_SetRenderDrawColor(renderer,
            color_unexplored.r, color_unexplored.g,
            color_unexplored.b, 200);
        SDL_FRect bg = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)
        };
        SDL_RenderFillRect(renderer, &bg);

        // Draw border
        SDL_SetRenderDrawColor(renderer,
            color_border.r, color_border.g,
            color_border.b, color_border.a);
        SDL_RenderRect(renderer, &bg);

        // Calculate visible map range (what fits in 250x250 at 4x4 per tile)
        int tiles_wide = width / tile_size;   // 250 / 4 = 62 tiles wide
        int tiles_high = height / tile_size;  // 250 / 4 = 62 tiles high

        int start_x = camera_center_x - tiles_wide / 2;
        int start_y = camera_center_y - tiles_high / 2;
        int end_x = start_x + tiles_wide;
        int end_y = start_y + tiles_high;

        // Clamp to map bounds
        start_x = std::max(0, start_x);
        start_y = std::max(0, start_y);
        end_x = std::min(game_map->get_width(), end_x);
        end_y = std::min(game_map->get_height(), end_y);

        // Render each tile as a 4x4 pixel square
        for (int map_y = start_y; map_y < end_y; map_y++) {
            for (int map_x = start_x; map_x < end_x; map_x++) {
                bool is_explored = explored[map_y][map_x];
                bool is_visible = visible[map_y][map_x];

                // Skip unexplored tiles if fog is enabled
                if (show_fog && !is_explored) continue;

                // Get color for this tile
                SDL_Color color = get_tile_color(map_x, map_y, is_visible, is_explored);

                // Calculate screen position
                int screen_x = x + (map_x - start_x) * tile_size;
                int screen_y = y + (map_y - start_y) * tile_size;

                // Draw 4x4 pixel square
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                SDL_FRect tile_rect = {
                    static_cast<float>(screen_x),
                    static_cast<float>(screen_y),
                    static_cast<float>(tile_size),
                    static_cast<float>(tile_size)
                };
                SDL_RenderFillRect(renderer, &tile_rect);
            }
        }
    }

    // Convert screen coords to map coords (for clicking minimap)
    bool screen_to_map(int screen_x, int screen_y, int& map_x, int& map_y) const {
        // Check if click is inside minimap
        if (screen_x < x || screen_x >= x + width ||
            screen_y < y || screen_y >= y + height) {
            return false;
        }

        int tiles_wide = width / tile_size;
        int tiles_high = height / tile_size;
        int start_x = camera_center_x - tiles_wide / 2;
        int start_y = camera_center_y - tiles_high / 2;

        int tile_x_offset = (screen_x - x) / tile_size;
        int tile_y_offset = (screen_y - y) / tile_size;

        map_x = start_x + tile_x_offset;
        map_y = start_y + tile_y_offset;

        // Clamp to map bounds
        map_x = std::max(0, std::min(game_map->get_width() - 1, map_x));
        map_y = std::max(0, std::min(game_map->get_height() - 1, map_y));

        return true;
    }

    // Add to Minimap class in ui/Minimap.h
    void set_map(Map* new_map) {
        game_map = new_map;

        // Resize explored/visible arrays
        explored.clear();
        visible.clear();
        explored.resize(game_map->get_height());
        visible.resize(game_map->get_height());

        for (int i = 0; i < game_map->get_height(); i++) {
            explored[i].resize(game_map->get_width(), false);
            visible[i].resize(game_map->get_width(), false);
        }
    }



    // Settings
    void set_show_fog(bool enabled) { show_fog = enabled; }
    void set_show_entities(bool enabled) { show_entities = enabled; }
    void set_tile_size(int size) { tile_size = size; }  // Change to 3x3 or 5x5 if needed

    bool get_show_fog() const { return show_fog; }
    bool get_show_entities() const { return show_entities; }
};