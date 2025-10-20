#include "Map.h"
#include <algorithm>
#include <iostream>

#include "../utils/Logger.h"

Map::Map(int w, int h, unsigned int seed)
    : width(w), height(h), rng(seed == 0 ? std::random_device{}() : seed) {

    tiles.resize(height);
    for (int y = 0; y < height; y++) {
        tiles[y].resize(width, TileType::WALL);
    }

    std::cout << "Created map: " << width << "x" << height << std::endl;
}

void Map::generate(MapGenerator& generator) {
    std::cout << "Generating map with: " << generator.get_name() << std::endl;
    generator.generate(*this, rng);
}

void Map::fill_all(TileType type) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            tiles[y][x] = type;
        }
    }
}

void Map::add_room(const Room& room) {
    rooms.push_back(room);
}

TileType Map::get_tile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return TileType::WALL;
    }
    return tiles[y][x];
}

void Map::set_tile(int x, int y, TileType type) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y][x] = type;
    }
}

bool Map::is_walkable(int x, int y) const {
    TileType tile = get_tile(x, y);
    return tile == TileType::FLOOR ||
        tile == TileType::DOOR_OPEN ||
        tile == TileType::STAIRS_DOWN ||
        tile == TileType::STAIRS_UP;
}

bool Map::is_transparent(int x, int y) const {
    TileType tile = get_tile(x, y);
    return tile != TileType::WALL && tile != TileType::DOOR_CLOSED;
}

const Room* Map::get_random_room() {
    if (rooms.empty()) return nullptr;
    std::uniform_int_distribution<int> dist(0, rooms.size() - 1);
    return &rooms[dist(rng)];
}

void Map::create_room(const Room& room) {
    for (int y = room.y; y < room.y + room.height; y++) {
        for (int x = room.x; x < room.x + room.width; x++) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                tiles[y][x] = TileType::FLOOR;
            }
        }
    }
}

void Map::create_horizontal_corridor(int x1, int x2, int y) {
    int start_x = std::min(x1, x2);
    int end_x = std::max(x1, x2);

    for (int x = start_x; x <= end_x; x++) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y][x] = TileType::FLOOR;
        }
    }
}

void Map::create_vertical_corridor(int y1, int y2, int x) {
    int start_y = std::min(y1, y2);
    int end_y = std::max(y1, y2);

    for (int y = start_y; y <= end_y; y++) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y][x] = TileType::FLOOR;
        }
    }
}

void Map::dump_to_log(int max_width, int max_height) const {
    LOG_INFO("=== MAP DUMP ===");
    LOG_INFO("Dimensions: " + std::to_string(width) + "x" + std::to_string(height));
    LOG_INFO("Number of rooms: " + std::to_string(rooms.size()));

    // Print room information
    for (size_t i = 0; i < rooms.size(); i++) {
        const Room& room = rooms[i];
        LOG_INFO("Room " + std::to_string(i) + ": x=" + std::to_string(room.x) +
            " y=" + std::to_string(room.y) +
            " w=" + std::to_string(room.width) +
            " h=" + std::to_string(room.height) +
            " center=(" + std::to_string(room.center_x()) + "," + std::to_string(room.center_y()) + ")");
    }

    // Limit display size for readability
    int display_width = std::min(width, max_width);
    int display_height = std::min(height, max_height);

    LOG_INFO("Map visual (showing first " + std::to_string(display_width) + "x" + std::to_string(display_height) + "):");
    LOG_INFO("Legend: # = WALL, . = FLOOR, + = DOOR_CLOSED, / = DOOR_OPEN, > = STAIRS_DOWN, < = STAIRS_UP");

    // Build the map string
    for (int y = 0; y < display_height; y++) {
        std::string line;
        for (int x = 0; x < display_width; x++) {
            TileType tile = get_tile(x, y);
            char symbol;

            switch (tile) {
            case TileType::WALL:         symbol = '#'; break;
            case TileType::FLOOR:        symbol = '.'; break;
            case TileType::DOOR_CLOSED:  symbol = '+'; break;
            case TileType::DOOR_OPEN:    symbol = '/'; break;
            case TileType::STAIRS_DOWN:  symbol = '>'; break;
            case TileType::STAIRS_UP:    symbol = '<'; break;
            default:                     symbol = '?'; break;
            }

            line += symbol;
        }
        Logger::get_instance().log(LogLevel::INFO, line);
    }

    if (width > max_width || height > max_height) {
        LOG_INFO("(Map truncated for display)");
    }

    LOG_INFO("=== END MAP DUMP ===");
}

void Map::dump_with_sprites_to_log(
    std::function<std::string(int, int, TileType)> get_sprite_name,
    int max_width,
    int max_height
) const {
    LOG_INFO("=== DETAILED MAP DUMP (With Sprite Names) ===");
    LOG_INFO("Dimensions: " + std::to_string(width) + "x" + std::to_string(height));
    LOG_INFO("Number of rooms: " + std::to_string(rooms.size()));

    // Limit display size
    int display_width = std::min(width, max_width);
    int display_height = std::min(height, max_height);

    LOG_INFO("Showing first " + std::to_string(display_width) + "x" + std::to_string(display_height) + " tiles:");
    LOG_INFO("Format: [x,y] TileType -> SpriteName");
    LOG_INFO("");

    // Option 1: Detailed list (every tile)
    for (int y = 0; y < display_height; y++) {
        for (int x = 0; x < display_width; x++) {
            TileType tile = get_tile(x, y);

            // Skip floors to reduce spam (optional)
            if (tile == TileType::FLOOR) continue;

            std::string tile_type_str;
            switch (tile) {
            case TileType::WALL:         tile_type_str = "WALL"; break;
            case TileType::FLOOR:        tile_type_str = "FLOOR"; break;
            case TileType::DOOR_CLOSED:  tile_type_str = "DOOR_CLOSED"; break;
            case TileType::DOOR_OPEN:    tile_type_str = "DOOR_OPEN"; break;
            case TileType::STAIRS_DOWN:  tile_type_str = "STAIRS_DOWN"; break;
            case TileType::STAIRS_UP:    tile_type_str = "STAIRS_UP"; break;
            default:                     tile_type_str = "UNKNOWN"; break;
            }

            std::string sprite_name = get_sprite_name(x, y, tile);

            LOG_DEBUG("[" + std::to_string(x) + "," + std::to_string(y) + "] " +
                tile_type_str + " -> '" + sprite_name + "'");
        }
    }

    LOG_INFO("=== END DETAILED MAP DUMP ===");
}

void Map::dump_sprite_grid_to_log(
    std::function<std::string(int, int, TileType)> get_sprite_name,
    int start_x, int start_y,
    int width, int height
) const {
    LOG_INFO("=== SPRITE NAME GRID ===");
    LOG_INFO("Region: (" + std::to_string(start_x) + "," + std::to_string(start_y) + ") " +
        "to (" + std::to_string(start_x + width) + "," + std::to_string(start_y + height) + ")");
    LOG_INFO("");

    for (int y = start_y; y < start_y + height && y < this->height; y++) {
        std::string line = "Row " + std::to_string(y) + ": ";

        for (int x = start_x; x < start_x + width && x < this->width; x++) {
            TileType tile = get_tile(x, y);
            std::string sprite_name = get_sprite_name(x, y, tile);

            // Abbreviate for readability
            std::string abbrev;
            if (sprite_name.find("wall.corner.nw") != std::string::npos) abbrev = "A";
            else if (sprite_name.find("wall.corner.ne") != std::string::npos) abbrev = "B";
            else if (sprite_name.find("wall.corner.sw") != std::string::npos) abbrev = "C";
            else if (sprite_name.find("wall.corner.se") != std::string::npos) abbrev = "D";
            else if (sprite_name.find("wall.t.north") != std::string::npos) abbrev = "E";
            else if (sprite_name.find("wall.t.south") != std::string::npos) abbrev = "F";
            else if (sprite_name.find("wall.t.east") != std::string::npos) abbrev = "J";
            else if (sprite_name.find("wall.t.west") != std::string::npos) abbrev = "K";
            else if (sprite_name.find("wall.cross") != std::string::npos) abbrev = "L";
            else if (sprite_name.find("wall.horizontal") != std::string::npos) abbrev = "M";
            else if (sprite_name.find("wall.vertical") != std::string::npos) abbrev = "N";
            else if (sprite_name.find("wall.end") != std::string::npos) abbrev = "=";
            else if (sprite_name.find("wall.single") != std::string::npos) abbrev = "*";
            else if (sprite_name.find("floor") != std::string::npos) abbrev = ".";
            else if (sprite_name.find("door.closed") != std::string::npos) abbrev = "+";
            else if (sprite_name.find("door.open") != std::string::npos) abbrev = "/";
            else abbrev = "?";

            line += abbrev;
        }

        LOG_INFO(line);
    }

    LOG_INFO("");
    LOG_INFO("Legend:");
    LOG_INFO("  ABCD= corners (nw,ne,sw,se)");
    LOG_INFO("  EFJK = T-junctions (n,s,e,w)");
    LOG_INFO("  L = cross");
    LOG_INFO("  MN = straight (h,v)");
    LOG_INFO("  = = end");
    LOG_INFO("  * = single");
    LOG_INFO("  . = floor");
    LOG_INFO("  + = closed door");
    LOG_INFO("  / = open door");
    LOG_INFO("=== END SPRITE NAME GRID ===");
}