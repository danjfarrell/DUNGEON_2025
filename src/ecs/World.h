#pragma once

#include "Entity.h"
#include "ComponentManager.h"
#include <vector>
#include <memory>
#include "../world/TileVisibility.h"   //  NEW INCLUDE

// Forward declaration
class System;

class World {
private:
    EntityManager entity_manager;
    ComponentManager component_manager;
    std::vector<std::unique_ptr<System>> systems;

    std::unique_ptr<TileVisibility> tile_visibility;  // NEW


public:
    // Constructor/Destructor - IMPLEMENTATION IN .cpp
    World();
    ~World();
    // Initialize visibility system
        // NEW: Tile visibility access (non-template)
    TileVisibility* get_tile_visibility();
    void initialize_tile_visibility(int width, int height);


    // ========================================
    // NEW: Save and restore exploration state
    // ========================================
    std::unique_ptr<TileVisibility> take_tile_visibility() {
        return std::move(tile_visibility);
    }

    void set_tile_visibility(std::unique_ptr<TileVisibility> vis) {
        tile_visibility = std::move(vis);
    }



    // Entity management
    Entity create_entity();
    void destroy_entity(Entity entity);

    // Component management
    template<typename T>
    void add_component(Entity entity, T component) {
        component_manager.add_component(entity, component);
    }

    template<typename T>
    T* get_component(Entity entity) {
        return component_manager.get_component<T>(entity);
    }

    template<typename T>
    bool has_component(Entity entity) {
        return component_manager.has_component<T>(entity);
    }

    template<typename T>
    void remove_component(Entity entity) {
        component_manager.remove_component<T>(entity);
    }

    // System management
    template<typename T, typename... Args>
    T* add_system(Args&&... args);

    // Update all systems
    void update(float dt);

    // Access to component manager
    ComponentManager& get_component_manager();
};

// Template function implementations must stay in header
#include "System.h"  // Include AFTER the class declaration

template<typename T, typename... Args>
T* World::add_system(Args&&... args) {
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    systems.push_back(std::move(system));
    return ptr;
}