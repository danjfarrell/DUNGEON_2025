#pragma once
#include <vector>
#include <string>
#include "TileSheet.h"
#include "TileAtlas.h"
#include "WorldItem.h"


class Map {
public:
    bool loadFromFile(const std::string& path);
    void render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas);
    const std::vector<std::string>& data() const { return grid_; }
    void addItem(const Item& it, int x, int y) { items_.push_back(WorldItem{ it,x,y }); }
    int  itemIndexAt(int x, int y) const;
    void removeItemAt(int idx);
    const std::vector<WorldItem>& items() const { return items_; }


private:
    std::vector<std::string> grid_;
    std::vector<WorldItem> items_;

    void drawTile(char c, int x, int y, SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas);
};

