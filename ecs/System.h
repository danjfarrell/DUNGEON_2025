#pragma once

#include "ComponentManager.h"

class System {
public:
    virtual void update(ComponentManager& components, float dt) = 0;
    virtual ~System() = default;
};