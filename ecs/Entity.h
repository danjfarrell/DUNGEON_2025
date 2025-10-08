#pragma once
#include <cstdint>
#include <vector>


// Entity is just an ID number
using Entity = std::uint32_t;

class EntityManager {
private:
    std::uint32_t next_id;
    std::vector<Entity> available_ids;

public:
    EntityManager() : next_id(0) {
        available_ids.reserve(100);
    }
    Entity create() {
        // If we have recycled IDs, use one
        if (!available_ids.empty()) {
            Entity id = available_ids.back();
            available_ids.pop_back();
            return id;
        }

        // Otherwise, give out a new ID
        return next_id++;
    }
    void destroy(Entity entity) {
        available_ids.push_back(entity);
    }
};