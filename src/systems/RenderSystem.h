#pragma once

#include "../ecs/System.h"
#include "Camera.h"
#include "../components/Components.h"
#include "../graphics/SpriteManager.h"
#include "../ui/UILayout.h"
#include "../world/TileVisibility.h" //new
#include <vector>
#include <algorithm>

class RenderSystem : public System {
private:
    SpriteManager* sprite_manager;
    int tile_scale;  // How much to scale tiles (2 = 32x32 for 16x16 tiles)
    Camera* camera;  // NEW
    UILayout* ui_layout;  //  ADD THIS
    TileVisibility* tile_visibility;

public:
    RenderSystem(SpriteManager* sm, int scale = 2, Camera* cam = nullptr, UILayout* layout = nullptr,
        TileVisibility* vis = nullptr)
        : sprite_manager(sm), tile_scale(scale), camera(cam), ui_layout(layout), tile_visibility(vis) {
    }
        

    void update(ComponentManager& components, float dt) override;

    // NEW: Set camera reference
    void set_camera(Camera* cam) { camera = cam; }
    void set_tile_visibility(TileVisibility* vis) { tile_visibility = vis; }  // ADD THIS
};