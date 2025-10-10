#include "World.h"
#include "System.h"

Entity World::create_entity() {
    return entity_manager.create();
}

void World::destroy_entity(Entity entity) {
    // TODO: Should remove all components from this entity
    entity_manager.destroy(entity);
}

void World::update(float dt) {
    for (auto& system : systems) {
        system->update(component_manager, dt);
    }
}

ComponentManager& World::get_component_manager() {
    return component_manager;
}