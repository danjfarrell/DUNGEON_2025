#pragma once

#include "../ecs/System.h"
#include "../components/Components.h"
#include "../graphics/SpriteManager.h"

class SpriteUpdateSystem : public System {
private:
    SpriteManager* sprite_manager;

public:
    SpriteUpdateSystem(SpriteManager* sm);

    void update(ComponentManager& components, float dt) override;
};