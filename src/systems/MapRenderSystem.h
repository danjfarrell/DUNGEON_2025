#pragma once

#include "../ecs/System.h"
#include "../graphics/SpriteManager.h"
#include "../world/Map.h"
#include <string>

class MapRenderSystem : public System {
private:
    SpriteManager* sprite_manager;
    Map* game_map;
    int tile_scale;

public:
    MapRenderSystem(SpriteManager* sm, Map* map, int scale = 2);

    void update(ComponentManager& components, float dt) override;

private:
    std::string get_wall_sprite_name(int x, int y);
    bool is_wall(int x, int y);
    void render_map_tile(int x, int y);
};