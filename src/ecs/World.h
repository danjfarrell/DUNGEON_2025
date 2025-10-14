#pragma once

#include "Entity.h"
#include "ComponentManager.h"
#include <vector>
#include <memory>

// Forward declaration
class System;

class World {
private:
    EntityManager entity_manager;
    ComponentManager component_manager;
    std::vector<std::unique_ptr<System>> systems;

public:
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