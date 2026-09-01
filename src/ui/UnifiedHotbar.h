// src/ui/UnifiedHotbar.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../components/MagicComponents.h"
#include "../magic/SpellDatabase.h"

class UnifiedHotbar {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    World* world;
    const SpellDatabase* spell_db;

    int x, y;
    int width, height;
    int slot_size;
    int slot_spacing;

    // Colors
    SDL_Color bg_color;
    SDL_Color spell_bg_color;
    SDL_Color item_bg_color;
    SDL_Color border_color;
    SDL_Color selected_color;
    SDL_Color text_color;
    SDL_Color spell_text_color;
    SDL_Color item_text_color;
    SDL_Color hotkey_color;

public:
    UnifiedHotbar(SDL_Renderer* rend, TTF_Font* f, World* w,
        const SpellDatabase* db,
        int pos_x, int pos_y, int w_pixels, int h_pixels)
        : renderer(rend), font(f), world(w), spell_db(db),
        x(pos_x), y(pos_y), width(w_pixels), height(h_pixels),
        slot_size(50), slot_spacing(60) {

        bg_color = { 20, 20, 30, 255 };
        spell_bg_color = { 40, 30, 60, 255 };      // Purple-ish for spells
        item_bg_color = { 30, 50, 40, 255 };       // Green-ish for items
        border_color = { 100, 100, 120, 255 };
        selected_color = { 200, 200, 100, 255 };
        text_color = { 255, 255, 255, 255 };
        spell_text_color = { 200, 150, 255, 255 }; // Light purple
        item_text_color = { 150, 255, 150, 255 };  // Light green
        hotkey_color = { 255, 255, 100, 255 };
    }

    void render(Entity player) {
        if (!font) return;

        SpellBook* spellbook = world->get_component<SpellBook>(player);
        Inventory* inventory = world->get_component<Inventory>(player);
        Mana* mana = world->get_component<Mana>(player);

        // Draw background panel
        SDL_FRect bg_rect = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)
        };
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(renderer, &bg_rect);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
            border_color.b, border_color.a);
        SDL_RenderRect(renderer, &bg_rect);

        // Calculate starting position (centered)
        int total_slots = 10;  // 5 spells + 5 items
        int total_width = total_slots * slot_spacing;
        int start_x = x + (width - total_width) / 2;

        // ========================================
        // LEFT SIDE: SPELL SLOTS (1-5)
        // ========================================
        for (int i = 0; i < 5; i++) {
            int slot_x = start_x + (i * slot_spacing);
            int slot_y = y + 10;

            render_spell_slot(player, spellbook, mana, i, slot_x, slot_y);
        }

        // ========================================
        // DIVIDER
        // ========================================
        int divider_x = start_x + (5 * slot_spacing) - 5;
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderLine(renderer, divider_x, y + 5, divider_x, y + height - 5);

        // ========================================
        // RIGHT SIDE: ITEM SLOTS (6-0)
        // ========================================
        for (int i = 0; i < 5; i++) {
            int slot_x = start_x + ((i + 5) * slot_spacing);
            int slot_y = y + 10;
            int item_index = i;  // Maps to inventory slots 0-4

            render_item_slot(player, inventory, item_index, slot_x, slot_y, i + 6);
        }

        // ========================================
        // INSTRUCTIONS
        // ========================================
        //render_text_centered("Spells (Shift+1-5)",
        //    start_x, y + height - 20,
        //    5 * slot_spacing,
        //    spell_text_color);

        //render_text_centered("Items (6-0)",
        //    start_x + (5 * slot_spacing),
        //    y + height - 20,
        //    5 * slot_spacing,
        //    item_text_color);
    }

private:
    void render_spell_slot(Entity player, SpellBook* spellbook, Mana* mana,
        int slot_index, int slot_x, int slot_y) {
        // Draw slot background
        SDL_FRect slot_bg = {
            static_cast<float>(slot_x),
            static_cast<float>(slot_y),
            static_cast<float>(slot_size),
            static_cast<float>(slot_size)
        };

        SDL_SetRenderDrawColor(renderer,
            spell_bg_color.r, spell_bg_color.g,
            spell_bg_color.b, spell_bg_color.a);
        SDL_RenderFillRect(renderer, &slot_bg);

        SDL_SetRenderDrawColor(renderer,
            border_color.r, border_color.g,
            border_color.b, border_color.a);
        SDL_RenderRect(renderer, &slot_bg);

        // Draw hotkey number
        std::string hotkey = std::to_string(slot_index + 1);
        render_text(hotkey.c_str(), slot_x + 5, slot_y + 5, hotkey_color);

        // Draw spell if equipped
        if (spellbook && spell_db) {
            std::string spell_id = spellbook->get_spell_in_slot(slot_index);
            if (!spell_id.empty()) {
                const Spell* spell = spell_db->get_spell(spell_id);
                if (spell) {
                    // Spell name (abbreviated)
                    std::string display_name = get_spell_abbrev(spell->name);
                    render_text_centered(display_name.c_str(),
                        slot_x, slot_y + 20,
                        slot_size, text_color);

                    // Mana cost
                    std::string cost = std::to_string(spell->mana_cost) + "MP";
                    SDL_Color cost_color = (mana && mana->current >= spell->mana_cost)
                        ? SDL_Color{ 100, 255, 255, 255 }  // Cyan = can cast
                    : SDL_Color{ 255, 100, 100, 255 }; // Red = not enough mana

                    render_text_centered(cost.c_str(),
                        slot_x, slot_y + 35,
                        slot_size, cost_color);
                }
            }
            else {
                render_text_centered("[Empty]", slot_x, slot_y + 25,
                    slot_size, { 100, 100, 100, 255 });
            }
        }
    }

    void render_item_slot(Entity player, Inventory* inventory,
        int item_index, int slot_x, int slot_y,
        int display_number) {
        // Draw slot background
        SDL_FRect slot_bg = {
            static_cast<float>(slot_x),
            static_cast<float>(slot_y),
            static_cast<float>(slot_size),
            static_cast<float>(slot_size)
        };

        SDL_SetRenderDrawColor(renderer,
            item_bg_color.r, item_bg_color.g,
            item_bg_color.b, item_bg_color.a);
        SDL_RenderFillRect(renderer, &slot_bg);

        SDL_SetRenderDrawColor(renderer,
            border_color.r, border_color.g,
            border_color.b, border_color.a);
        SDL_RenderRect(renderer, &slot_bg);

        // Draw hotkey number
        std::string hotkey = (display_number == 10) ? "0" : std::to_string(display_number);
        render_text(hotkey.c_str(), slot_x + 5, slot_y + 5, hotkey_color);

        // Draw item if present
        if (inventory && item_index < static_cast<int>(inventory->items.size())) {
            Entity item_entity = inventory->items[item_index];
            Item* item = world->get_component<Item>(item_entity);
            Name* item_name = world->get_component<Name>(item_entity);

            if (item && item_name) {
                // Item name (abbreviated)
                std::string display_name = get_item_abbrev(item->item_type);
                render_text_centered(display_name.c_str(),
                    slot_x, slot_y + 20,
                    slot_size, text_color);

                // Quantity
                if (item->quantity > 1) {
                    std::string qty = "x" + std::to_string(item->quantity);
                    render_text(qty.c_str(),
                        slot_x + slot_size - 20,
                        slot_y + slot_size - 15,
                        { 200, 200, 200, 255 });
                }
            }
        }
        else {
            render_text_centered("[Empty]", slot_x, slot_y + 25,
                slot_size, { 100, 100, 100, 255 });
        }
    }

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

    void render_text_centered(const char* text, int cx, int cy,
        int box_width, SDL_Color color) {
        if (!font) return;

        SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int text_x = cx + (box_width - surface->w) / 2;
                SDL_FRect dest = {
                    static_cast<float>(text_x),
                    static_cast<float>(cy),
                    static_cast<float>(surface->w),
                    static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }

    std::string get_spell_abbrev(const std::string& name) {
        if (name == "Magic Missile") return "M.Miss";
        if (name == "Minor Heal") return "Heal";
        if (name == "Lightning Bolt") return "L.Bolt";
        if (name == "Fireball") return "Fireball";
        if (name == "Chain Lightning") return "Chain";
        if (name == "Meteor Storm") return "Meteor";
        if (name == "Cure Wounds") return "Cure";
        if (name == "Serious Healing") return "S.Heal";
        if (name == "Complete Restoration") return "Full";

        // Default: first 6 chars
        return name.substr(0, std::min<size_t>(6, name.length()));
    }

    std::string get_item_abbrev(const std::string& item_type) {
        if (item_type == "health_potion") return "HP Pot";
        if (item_type == "mana_potion") return "MP Pot";
        if (item_type == "greater_health_potion") return "G.HP";
        if (item_type == "greater_mana_potion") return "G.MP";
        if (item_type == "superior_health_potion") return "S.HP";
        if (item_type == "cure_poison_potion") return "Antid";
        if (item_type == "haste_potion") return "Haste";

        // Default: first 6 chars
        return item_type.substr(0, std::min<size_t>(6, item_type.length()));
    }
};