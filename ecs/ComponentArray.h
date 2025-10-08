#pragma once
#include <cstddef>
#include <vector>
#include "Entity.h"

template<typename T>
class ComponentArray {
private:
    std::vector<T> components;
    std::vector<Entity> dense_to_entity;
    std::vector<size_t> entity_to_dense;
public:
    ComponentArray() {
        entity_to_dense.resize(1000, SIZE_MAX);
    }
    void insert(Entity entity, T component) {
        // Make sure our sparse array is big enough
        if (entity >= entity_to_dense.size()) {
            entity_to_dense.resize(entity + 1, SIZE_MAX);
        }
        // Add component to the dense array
        size_t new_index = components.size();
        components.push_back(component);
        dense_to_entity.push_back(entity);
        // Update the mapping
        entity_to_dense[entity] = new_index;
    }
    
    T* get(Entity entity) {
        
        // Check if entity ID is within range
        if (entity >= entity_to_dense.size()) {
            return nullptr;
        }

        // Get the index in the dense array
        size_t index = entity_to_dense[entity];

        // Check if entity has this component
        if (index == SIZE_MAX) {
            return nullptr;
        }

        // Return pointer to the component
        return &components[index];
    }

    bool has(Entity entity) const {
        return entity < entity_to_dense.size() &&
            entity_to_dense[entity] != SIZE_MAX;
    }

    void remove(Entity entity) {
        if (entity >= entity_to_dense.size()) {
            return;
        }

        size_t removed_index = entity_to_dense[entity];
        if (removed_index == SIZE_MAX) {
            return;
        }

        size_t last_index = components.size() - 1;
        Entity last_entity = dense_to_entity[last_index];

        components[removed_index] = components[last_index];
        dense_to_entity[removed_index] = last_entity;
        entity_to_dense[last_entity] = removed_index;
        entity_to_dense[entity] = SIZE_MAX;

        components.pop_back();
        dense_to_entity.pop_back();
    }


    // For systems to iterate efficiently
    std::vector<T>& get_components() {
        return components;
    }

    std::vector<Entity>& get_entities() {
        return dense_to_entity;
    }

    size_t size() const {
        return components.size();
    }
};