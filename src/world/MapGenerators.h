#pragma once

#include "Map.h"
#include <stack>

// Room and Corridor Generator
class RoomCorridorGenerator : public MapGenerator {
private:
    int num_rooms;
    int min_room_size;
    int max_room_size;

public:
    RoomCorridorGenerator(int num_rooms = 10, int min_size = 4, int max_size = 10)
        : num_rooms(num_rooms), min_room_size(min_size), max_room_size(max_size) {
    }

    std::string get_name() const override {
        return "Room and Corridor Generator";
    }

    void generate(Map& map, std::mt19937& rng) override;

private:
    void create_room_on_map(Map& map, const Room& room);
    void create_h_corridor(Map& map, int x1, int x2, int y);
    void create_v_corridor(Map& map, int y1, int y2, int x);
};

// LARN-Style Maze Generator
class LarnMazeGenerator : public MapGenerator {
private:
    bool create_open_spaces;
    int num_open_areas;

public:
    LarnMazeGenerator(bool open_spaces = true, int open_areas = 3)
        : create_open_spaces(open_spaces), num_open_areas(open_areas) {
    }

    std::string get_name() const override {
        return "LARN Maze Generator (Recursive Backtracking)";
    }

    void generate(Map& map, std::mt19937& rng) override;

private:
    void carve_maze(Map& map, int x, int y, std::mt19937& rng);
    void create_open_space(Map& map, std::mt19937& rng);
};

// Cellular Automata Generator
class CellularAutomataGenerator : public MapGenerator {
private:
    float initial_wall_probability;
    int smoothing_iterations;

public:
    CellularAutomataGenerator(float wall_prob = 0.45f, int iterations = 4)
        : initial_wall_probability(wall_prob), smoothing_iterations(iterations) {
    }

    std::string get_name() const override {
        return "Cellular Automata Generator (Cave)";
    }

    void generate(Map& map, std::mt19937& rng) override;

private:
    int count_wall_neighbors(Map& map, int x, int y);
    void smooth_map(Map& map);
};