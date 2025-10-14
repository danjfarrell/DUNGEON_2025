#include "RenderSystem.h"

RenderSystem::RenderSystem(SpriteManager* sm, int scale)
    : sprite_manager(sm), tile_scale(scale) {
}

void RenderSystem::update(ComponentManager& components, float dt) {
    // Get component arrays
    ComponentArray<Position>* positions = components.get_array<Position>();
    ComponentArray<Renderable>* renderables = components.get_array<Renderable>();

    if (!positions || !renderables) {
        return;
    }

    // Collect all entities that have both Position and Renderable
    struct RenderInfo {
        Position pos;
        Renderable rend;
    };
    std::vector<RenderInfo> to_render;

    auto& rend_entities = renderables->get_entities();
    auto& rend_data = renderables->get_components();

    for (size_t i = 0; i < rend_data.size(); i++) {
        Entity entity = rend_entities[i];
        Position* pos = positions->get(entity);

        if (pos) {
            to_render.push_back({ *pos, rend_data[i] });
        }
    }

    // Sort by layer (lower layers drawn first, so higher layers appear on top)
    std::sort(to_render.begin(), to_render.end(),
        [](const RenderInfo& a, const RenderInfo& b) {
            return a.rend.layer < b.rend.layer;
        });

    // Render all entities
    int tile_size = sprite_manager->get_tile_width() * tile_scale;

    for (const auto& info : to_render) {
        int screen_x = info.pos.x * tile_size;
        int screen_y = info.pos.y * tile_size;

        sprite_manager->render_tile(
            info.rend.sheet_id,
            info.rend.tile_x,
            info.rend.tile_y,
            screen_x,
            screen_y,
            tile_scale
        );
    }
}