#include "MapGenerators.h"
#include <algorithm>
#include <iostream>

// ============================================================================
// ROOM AND CORRIDOR GENERATOR
// ============================================================================

void RoomCorridorGenerator::generate(Map& map, std::mt19937& rng) {
    map.fill_all(TileType::WALL);

    std::vector<Room> rooms;
    std::uniform_int_distribution<int> room_size_dist(min_room_size, max_room_size);

    for (int attempt = 0; attempt < num_rooms * 3; attempt++) {
        int room_width = room_size_dist(rng);
        int room_height = room_size_dist(rng);

        std::uniform_int_distribution<int> x_dist(1, map.get_width() - room_width - 1);
        std::uniform_int_distribution<int> y_dist(1, map.get_height() - room_height - 1);

        int x = x_dist(rng);
        int y = y_dist(rng);

        Room new_room(x, y, room_width, room_height);

        bool intersects = false;
        for (const auto& room : rooms) {
            if (new_room.intersects(room)) {
                intersects = true;
                break;
            }
        }

        if (intersects) continue;

        create_room_on_map(map, new_room);

        if (!rooms.empty()) {
            const Room& prev = rooms.back();

            if (rng() % 2 == 0) {
                create_h_corridor(map, prev.center_x(), new_room.center_x(), prev.center_y());
                create_v_corridor(map, prev.center_y(), new_room.center_y(), new_room.center_x());
            }
            else {
                create_v_corridor(map, prev.center_y(), new_room.center_y(), prev.center_x());
                create_h_corridor(map, prev.center_x(), new_room.center_x(), new_room.center_y());
            }
        }

        rooms.push_back(new_room);
        map.add_room(new_room);

        if (rooms.size() >= static_cast<size_t>(num_rooms)) break;
    }

    std::cout << "  Created " << rooms.size() << " rooms" << std::endl;
}

void RoomCorridorGenerator::create_room_on_map(Map& map, const Room& room) {
    for (int y = room.y; y < room.y + room.height; y++) {
        for (int x = room.x; x < room.x + room.width; x++) {
            map.set_tile(x, y, TileType::FLOOR);
        }
    }
}

void RoomCorridorGenerator::create_h_corridor(Map& map, int x1, int x2, int y) {
    int start = std::min(x1, x2);
    int end = std::max(x1, x2);
    for (int x = start; x <= end; x++) {
        map.set_tile(x, y, TileType::FLOOR);
    }
}

void RoomCorridorGenerator::create_v_corridor(Map& map, int y1, int y2, int x) {
    int start = std::min(y1, y2);
    int end = std::max(y1, y2);
    for (int y = start; y <= end; y++) {
        map.set_tile(x, y, TileType::FLOOR);
    }
}

// ============================================================================
// LARN MAZE GENERATOR
// ============================================================================

void LarnMazeGenerator::generate(Map& map, std::mt19937& rng) {
    map.fill_all(TileType::WALL);

    map.set_tile(1, 1, TileType::FLOOR);
    carve_maze(map, 1, 1, rng);

    if (create_open_spaces) {
        for (int i = 0; i < num_open_areas; i++) {
            create_open_space(map, rng);
        }
    }

    std::cout << "  Carved maze with " << (create_open_spaces ? "open spaces" : "tight corridors") << std::endl;
}

void LarnMazeGenerator::carve_maze(Map& map, int x, int y, std::mt19937& rng) {
    int directions[4][2] = { {0, -2}, {0, 2}, {2, 0}, {-2, 0} };

    for (int i = 3; i > 0; i--) {
        int j = rng() % (i + 1);
        std::swap(directions[i], directions[j]);
    }

    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        if (nx < 1 || nx >= map.get_width() - 1 ||
            ny < 1 || ny >= map.get_height() - 1) {
            continue;
        }

        if (map.get_tile(nx, ny) == TileType::WALL) {
            int mx = x + directions[i][0] / 2;
            int my = y + directions[i][1] / 2;

            map.set_tile(mx, my, TileType::FLOOR);
            map.set_tile(nx, ny, TileType::FLOOR);

            carve_maze(map, nx, ny, rng);
        }
    }
}

void LarnMazeGenerator::create_open_space(Map& map, std::mt19937& rng) {
    std::uniform_int_distribution<int> width_dist(5, 12);
    std::uniform_int_distribution<int> height_dist(3, 6);
    std::uniform_int_distribution<int> x_dist(3, map.get_width() - 15);
    std::uniform_int_distribution<int> y_dist(2, map.get_height() - 8);

    int width = width_dist(rng);
    int height = height_dist(rng);
    int x = x_dist(rng);
    int y = y_dist(rng);

    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int px = x + dx;
            int py = y + dy;

            if (px > 0 && px < map.get_width() - 1 &&
                py > 0 && py < map.get_height() - 1) {
                map.set_tile(px, py, TileType::FLOOR);
            }
        }
    }

    Room open_area(x, y, width, height);
    map.add_room(open_area);
}

// ============================================================================
// CELLULAR AUTOMATA GENERATOR
// ============================================================================

void CellularAutomataGenerator::generate(Map& map, std::mt19937& rng) {
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);

    for (int y = 0; y < map.get_height(); y++) {
        for (int x = 0; x < map.get_width(); x++) {
            if (x == 0 || x == map.get_width() - 1 ||
                y == 0 || y == map.get_height() - 1) {
                map.set_tile(x, y, TileType::WALL);
            }
            else {
                if (prob_dist(rng) < initial_wall_probability) {
                    map.set_tile(x, y, TileType::WALL);
                }
                else {
                    map.set_tile(x, y, TileType::FLOOR);
                }
            }
        }
    }

    for (int i = 0; i < smoothing_iterations; i++) {
        smooth_map(map);
    }

    Room cave_area(1, 1, map.get_width() - 2, map.get_height() - 2);
    map.add_room(cave_area);

    std::cout << "  Created cave with " << smoothing_iterations << " smoothing iterations" << std::endl;
}

int CellularAutomataGenerator::count_wall_neighbors(Map& map, int x, int y) {
    int count = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (map.get_tile(nx, ny) == TileType::WALL) {
                count++;
            }
        }
    }

    return count;
}

void CellularAutomataGenerator::smooth_map(Map& map) {
    std::vector<std::vector<TileType>> new_tiles(map.get_height());

    for (int y = 0; y < map.get_height(); y++) {
        new_tiles[y].resize(map.get_width());
        for (int x = 0; x < map.get_width(); x++) {
            new_tiles[y][x] = map.get_tile(x, y);
        }
    }

    for (int y = 1; y < map.get_height() - 1; y++) {
        for (int x = 1; x < map.get_width() - 1; x++) {
            int wall_count = count_wall_neighbors(map, x, y);

            if (wall_count >= 5) {
                new_tiles[y][x] = TileType::WALL;
            }
            else if (wall_count <= 3) {
                new_tiles[y][x] = TileType::FLOOR;
            }
        }
    }

    for (int y = 0; y < map.get_height(); y++) {
        for (int x = 0; x < map.get_width(); x++) {
            map.set_tile(x, y, new_tiles[y][x]);
        }
    }
}