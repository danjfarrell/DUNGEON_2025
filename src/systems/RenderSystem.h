#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../graphics/SpriteManager.h"
#include <vector>
#include <algorithm>

class RenderSystem : public System {
private:
    SpriteManager* sprite_manager;
    int tile_scale;  // How much to scale tiles (2 = 32x32 for 16x16 tiles)

public:
    RenderSystem(SpriteManager* sm, int scale = 2);

    void update(ComponentManager& components, float dt) override;
};