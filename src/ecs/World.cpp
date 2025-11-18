#include "World.h"
#include "System.h"


World::World() {
    // Initialize tile visibility as nullptr
    // Will be created when initialize_tile_visibility() is called
}

World::~World() {
    // Unique_ptr automatically cleans up
}




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

// ========================================
// NEW: Tile Visibility Methods
// ========================================

void World::initialize_tile_visibility(int width, int height) {
    tile_visibility = std::make_unique<TileVisibility>(width, height);
}

TileVisibility* World::get_tile_visibility() {
    return tile_visibility.get();
}