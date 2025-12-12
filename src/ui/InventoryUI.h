// src/ui/InventoryUI.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../ecs/World.h"
#include "../components/Components.h"

class InventoryUI {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    World* world;

    int x, y;           // Position on screen
    int slot_size;      // Size of each slot
    int slot_spacing;   // Space between slots
    int max_visible_slots;

    // Colors
    SDL_Color bg_color;
    SDL_Color border_color;
    SDL_Color selected_color;
    SDL_Color text_color;
    SDL_Color quantity_color;

public:
    InventoryUI(SDL_Renderer* rend, TTF_Font* f, World* w,
        int pos_x, int pos_y, int slot_sz = 50)
        : renderer(rend), font(f), world(w),
        x(pos_x), y(pos_y), slot_size(slot_sz),
        slot_spacing(slot_sz + 10), max_visible_slots(10) {

        bg_color = { 40, 40, 50, 255 };
        border_color = { 100, 100, 120, 255 };
        selected_color = { 200, 200, 100, 255 };
        text_color = { 255, 255, 255, 255 };
        quantity_color = { 200, 200, 200, 255 };
    }

    void render(Entity player) {
        if (!font) return;

        Inventory* inv = world->get_component<Inventory>(player);
        if (!inv) return;

        // Draw each item slot
        for (int i = 0; i < max_visible_slots; i++) {
            int slot_x = x + (i * slot_spacing);
            int slot_y = y;

            // Draw slot background
            SDL_FRect slot_bg = {
                static_cast<float>(slot_x),
                static_cast<float>(slot_y),
                static_cast<float>(slot_size),
                static_cast<float>(slot_size)
            };

            SDL_SetRenderDrawColor(renderer,
                bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &slot_bg);

            // Draw border
            SDL_SetRenderDrawColor(renderer,
                border_color.r, border_color.g,
                border_color.b, border_color.a);
            SDL_RenderRect(renderer, &slot_bg);

            // Draw slot number
            std::string slot_num = std::to_string(i + 1);
            if (i == 9) slot_num = "0";  // 0 key for 10th slot

            render_text(slot_num.c_str(),
                slot_x + 5, slot_y + 5,
                text_color);

            // Draw item if slot has one
            if (i < static_cast<int>(inv->items.size())) {
                Entity item_entity = inv->items[i];
                Item* item = world->get_component<Item>(item_entity);
                Name* item_name = world->get_component<Name>(item_entity);

                if (item && item_name) {
                    // Draw item name (abbreviated)
                    std::string display_name = get_short_name(item_name->name);
                    render_text(display_name.c_str(),
                        slot_x + 5, slot_y + 25,
                        text_color);

                    // Draw quantity if stackable
                    if (item->quantity > 1) {
                        std::string qty = "x" + std::to_string(item->quantity);
                        render_text(qty.c_str(),
                            slot_x + slot_size - 25,
                            slot_y + slot_size - 20,
                            quantity_color);
                    }
                }
            }
        }

        // Draw instruction text
        render_text("Press 1-0 to use items",
            x, y + slot_size + 10,
            { 150, 150, 150, 255 });
    }

private:
    void render_text(const char* text, int px, int py, SDL_Color color) {
        if (!font) return;

        SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_FRect dest = {
                    static_cast<float>(px),
                    static_cast<float>(py),
                    static_cast<float>(surface->w),
                    static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }

    std::string get_short_name(const std::string& full_name) {
        // Abbreviate long names
        if (full_name.length() <= 8) return full_name;

        // Common abbreviations
        if (full_name.find("health_potion") != std::string::npos) return "HP Pot";
        if (full_name.find("mana_potion") != std::string::npos) return "MP Pot";
        if (full_name.find("greater") != std::string::npos) return "G.HP Pot";

        // Default: truncate
        return full_name.substr(0, 8);
    }
};