#include "MapRenderSystem.h"

//MapRenderSystem::MapRenderSystem(SpriteManager* sm, Map* map, int scale, Camera* cam)
//    : sprite_manager(sm), game_map(map), tile_scale(scale), camera(cam) {
//}

void MapRenderSystem::update(ComponentManager& components, float dt) {
    if (!game_map) return;

    // If no camera, render everything (fallback)
    if (!camera) {
        for (int y = 0; y < game_map->get_height(); y++) {
            for (int x = 0; x < game_map->get_width(); x++) {
                render_map_tile(x, y);
            }
        }
        return;
    }

    // With camera: only render visible tiles
    int tile_size = sprite_manager->get_tile_width() * tile_scale;

    // Calculate visible tile range
    int start_x = camera->get_x() / tile_size;
    int start_y = camera->get_y() / tile_size;
    int end_x = (camera->get_x() + 800) / tile_size + 1;  // Screen width
    int end_y = (camera->get_y() + 600) / tile_size + 1;  // Screen height

    // Clamp to map bounds
    start_x = std::max(0, start_x);
    start_y = std::max(0, start_y);
    end_x = std::min(game_map->get_width(), end_x);
    end_y = std::min(game_map->get_height(), end_y);

    // Render only visible tiles
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            render_map_tile(x, y);
        }
    }

}
/*
void MapRenderSystem::render_map_tile(int x, int y) {
    TileType tile = game_map->get_tile(x, y);
    std::string sprite_name;

    switch (tile) {
    case TileType::FLOOR:
        sprite_name = "floor.stone";
        break;

    case TileType::WALL:
        sprite_name = get_wall_sprite_name(x, y);
        break;

    case TileType::DOOR_CLOSED:
        sprite_name = "door.closed";
        break;

    case TileType::DOOR_OPEN:
        sprite_name = "door.open";
        break;

    case TileType::STAIRS_DOWN:
        sprite_name = "stairs.down";
        break;

    case TileType::STAIRS_UP:
        sprite_name = "stairs.up";
        break;

    default:
        return;
    }

    int tile_size = sprite_manager->get_tile_width() * tile_scale;
    int screen_x = x * tile_size;
    int screen_y = y * tile_size;

    sprite_manager->render_sprite(sprite_name, screen_x, screen_y, tile_scale);
}
*/

void MapRenderSystem::render_map_tile(int x, int y) {
    TileType tile = game_map->get_tile(x, y);
    std::string sprite_name;

    switch (tile) {
    case TileType::VOID:
        // Don't render void tiles (or use a black/empty sprite)
        return;

    case TileType::FLOOR:
        sprite_name = "floor.stone";
        break;

    case TileType::WALL:
        sprite_name = get_wall_sprite_name(x, y);
        break;

    case TileType::DOOR_CLOSED:
        sprite_name = "door.closed";
        break;

    case TileType::DOOR_OPEN:
        sprite_name = "door.open";
        break;

    case TileType::STAIRS_DOWN:
        sprite_name = "stairs.down";
        break;

    case TileType::STAIRS_UP:
        sprite_name = "stairs.up";
        break;

    default:
        return;
    }

    int tile_size = sprite_manager->get_tile_width() * tile_scale;
    //int screen_x = x * tile_size;
    //int screen_y = y * tile_size;
    int world_x = x * tile_size;
    int world_y = y * tile_size;

    // IMPORTANT: Apply camera offset to map too
    int screen_x = world_x;
    int screen_y = world_y;

    if (camera) {
        screen_x = camera->world_to_screen_x(world_x);
        screen_y = camera->world_to_screen_y(world_y);
    }

    //  NEW: Add viewport offset
    if (ui_layout) {
        screen_x += ui_layout->game_viewport.x;
        screen_y += ui_layout->game_viewport.y;
    }

    sprite_manager->render_sprite(sprite_name, screen_x, screen_y, tile_scale);
}

bool MapRenderSystem::is_wall(int x, int y) {
    //TileType tile = game_map->get_tile(x, y);
    //return tile == TileType::WALL;
    if (x < 0 || x >= game_map->get_width() ||
        y < 0 || y >= game_map->get_height()) {
        return false;  // Out-of-bounds = no wall connection
    }

    TileType tile = game_map->get_tile(x, y);
    return tile == TileType::WALL;
}

std::string MapRenderSystem::get_wall_sprite_name(int x, int y) {
    bool n = is_wall(x, y - 1);
    bool s = is_wall(x, y + 1);
    bool e = is_wall(x + 1, y);
    bool w = is_wall(x - 1, y);

    // Cross intersection
    if (n && s && e && w) {
        return "wall.cross";
    }

    // T-junctions
    if (n && e && w && !s) return "wall.t.north";
    if (s && e && w && !n) return "wall.t.south";
    if (n && s && e && !w) return "wall.t.east";
    if (n && s && w && !e) return "wall.t.west";

    // Corners
    if (s && e && !n && !w) return "wall.corner.se";
    if (s && w && !n && !e) return "wall.corner.sw";
    if (n && e && !s && !w) return "wall.corner.ne";
    if (n && w && !s && !e) return "wall.corner.nw";

    // Straight segments
    if (n && s && !e && !w) return "wall.vertical";
    if (e && w && !n && !s) return "wall.horizontal";

    // Dead ends
    if (n && !s && !e && !w) return "wall.end.north";
    if (s && !n && !e && !w) return "wall.end.south";
    if (e && !n && !s && !w) return "wall.end.east";
    if (w && !n && !s && !e) return "wall.end.west";

    // Isolated
    return "wall.single";
}

std::string MapRenderSystem::get_sprite_name_for_tile(int x, int y, TileType tile) const {
    switch (tile) {
    case TileType::FLOOR:
        return "floor.stone";

    case TileType::WALL:
        return const_cast<MapRenderSystem*>(this)->get_wall_sprite_name(x, y);

    case TileType::DOOR_CLOSED:
        return "door.closed";

    case TileType::DOOR_OPEN:
        return "door.open";

    case TileType::STAIRS_DOWN:
        return "stairs.down";

    case TileType::STAIRS_UP:
        return "stairs.up";

    default:
        return "UNKNOWN";
    }
}