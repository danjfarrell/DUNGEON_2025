#pragma once

#include <vector>
#include <random>

// Forward declaration
class Map;

// Generation strategy interface
class MapGenerator {
public:
    virtual ~MapGenerator() = default;
    virtual void generate(Map& map, std::mt19937& rng) = 0;
    virtual std::string get_name() const = 0;
};

// Different types of tiles
enum class TileType {
    FLOOR,
    WALL,
    DOOR_CLOSED,
    DOOR_OPEN,
    STAIRS_DOWN,
    STAIRS_UP
};

// Represents a rectangular room
struct Room {
    int x, y;
    int width, height;

    Room(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), width(w), height(h) {
    }

    int center_x() const { return x + width / 2; }
    int center_y() const { return y + height / 2; }

    bool intersects(const Room& other) const {
        return x < other.x + other.width &&
            x + width > other.x &&
            y < other.y + other.height &&
            y + height > other.y;
    }
};

class Map {
private:
    int width;
    int height;
    std::vector<std::vector<TileType>> tiles;
    std::vector<Room> rooms;
    std::mt19937 rng;

public:
    Map(int w, int h, unsigned int seed = 0);

    // Query functions
    int get_width() const { return width; }
    int get_height() const { return height; }

    TileType get_tile(int x, int y) const;
    void set_tile(int x, int y, TileType type);

    bool is_walkable(int x, int y) const;
    bool is_transparent(int x, int y) const;

    const std::vector<Room>& get_rooms() const { return rooms; }
    const Room* get_random_room();

    // Use a generation strategy
    void generate(MapGenerator& generator);

    // Helper functions that generators can use
    void fill_all(TileType type);
    void add_room(const Room& room);
    std::mt19937& get_rng() { return rng; }

private:
    void create_room(const Room& room);
    void create_horizontal_corridor(int x1, int x2, int y);
    void create_vertical_corridor(int y1, int y2, int x);
};