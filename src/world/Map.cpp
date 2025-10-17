#include "Map.h"
#include <algorithm>
#include <iostream>

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