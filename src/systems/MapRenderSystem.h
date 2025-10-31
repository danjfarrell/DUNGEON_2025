#pragma once

#include "../ecs/System.h"
#include "../graphics/SpriteManager.h"
#include "../world/Map.h"
#include "../systems/Camera.h"  // NEW
#include <string>

class MapRenderSystem : public System {
private:
    SpriteManager* sprite_manager;
    Map* game_map;
    int tile_scale;
    Camera* camera;  // NEW

public:
    MapRenderSystem(SpriteManager* sm, Map* map, int scale = 2, Camera* cam = nullptr)
        : sprite_manager(sm), game_map(map), tile_scale(scale), camera(cam) {
    }
    //MapRenderSystem(SpriteManager* sm, Map* map, int scale = 2, Camera* cam = nullptr);

    void update(ComponentManager& components, float dt) override;

    // NEW: Public access to sprite selection logic for debugging
    std::string get_sprite_name_for_tile(int x, int y, TileType tile) const;

    // NEW: Set camera reference
    void set_camera(Camera* cam) { camera = cam; }

private:
    std::string get_wall_sprite_name(int x, int y);
    bool is_wall(int x, int y);
    void render_map_tile(int x, int y);
};