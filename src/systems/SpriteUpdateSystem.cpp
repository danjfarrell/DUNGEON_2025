#include "SpriteUpdateSystem.h"

SpriteUpdateSystem::SpriteUpdateSystem(SpriteManager* sm)
    : sprite_manager(sm) {
}

void SpriteUpdateSystem::update(ComponentManager& components, float dt) {
    auto* sprite_bases = components.get_array<SpriteBase>();
    auto* facings = components.get_array<Facing>();
    auto* renderables = components.get_array<Renderable>();

    if (!sprite_bases || !renderables) return;

    auto& sprite_entities = sprite_bases->get_entities();
    auto& sprite_data = sprite_bases->get_components();

    for (size_t i = 0; i < sprite_data.size(); i++) {
        Entity entity = sprite_entities[i];
        SpriteBase& sprite_base = sprite_data[i];

        // If entity has Facing, update variant based on direction
        Facing* facing = facings ? facings->get(entity) : nullptr;
        if (facing) {
            std::string direction_variant = facing->to_string();

            if (sprite_base.variant != direction_variant) {
                sprite_base.set_variant(direction_variant);
            }
        }

        // Update the Renderable component
        Renderable* rend = renderables->get(entity);
        if (rend) {
            std::string sprite_name = sprite_base.get_full_name();
            const SpriteDefinition* def = sprite_manager->get_sprite(sprite_name);

            if (def) {
                rend->sheet_id = def->sheet_id;
                rend->tile_x = def->tile_x;
                rend->tile_y = def->tile_y;
                rend->layer = def->layer;
            }
        }
    }
}