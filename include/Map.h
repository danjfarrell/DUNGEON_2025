// CHANGES since last full view:
// - render(...) now accepts originX/originY to draw the map at a UI offset (for new layout).
// - no other API changes here.

#pragma once
#include <vector>
#include <string>
#include "TileSheet.h"
#include "TileAtlas.h"
#include "Camera.h"
#include "WorldItem.h"

class Map {
public:
    bool loadFromFile(const std::string& path);

    void render(SDL_Renderer* r,
        TileSheet& tiles,
        const TileAtlas& atlas,
        const Camera& cam,
        int originX,
        int originY);

    const std::vector<std::string>& data() const { return grid_; }
    int width()  const { return grid_.empty() ? 0 : (int)grid_[0].size(); }
    int height() const { return (int)grid_.size(); }

    void addItem(const Item& it, int x, int y) { items_.push_back(WorldItem{ it,x,y }); }
    int  itemIndexAt(int x, int y) const;
    void removeItemAt(int idx);
    const std::vector<WorldItem>& items() const { return items_; }

private:
    std::vector<std::string> grid_;
    std::vector<WorldItem> items_;

    void drawTile(char c,
        int screenX,
        int screenY,
        SDL_Renderer* r,
        TileSheet& tiles,
        const TileAtlas& atlas);
};
