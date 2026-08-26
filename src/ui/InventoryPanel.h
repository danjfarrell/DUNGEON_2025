// src/ui/InventoryPanel.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include "../ecs/World.h"
#include "../components/Components.h"
#include "../components/Equipment.h"  // We'll create this next

class InventoryPanel {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* title_font;
    World* world;
    
    int x, y;
    int width, height;
    bool visible;
    
    // Layout sections
    SDL_Rect equipment_section;    // Top: Equipped items
    SDL_Rect inventory_section;    // Bottom: Item grid
    SDL_Rect stats_section;        // Right: Character stats
    
    int equipment_slot_size;
    int inventory_slot_size;
    int inventory_columns;
    int inventory_rows;
    
    // Colors
    SDL_Color bg_color;
    SDL_Color panel_color;
    SDL_Color border_color;
    SDL_Color text_color;
    SDL_Color highlight_color;
    SDL_Color equipment_color;
    SDL_Color consumable_color;
    SDL_Color gem_color;
    SDL_Color gold_color;
    
    // Selection
    int selected_slot;
    bool selecting_equipment;
    
public:
    InventoryPanel(SDL_Renderer* rend, TTF_Font* f, TTF_Font* title_f, World* w,
                   int pos_x, int pos_y, int w_pixels, int h_pixels)
        : renderer(rend), font(f), title_font(title_f), world(w),
          x(pos_x), y(pos_y), width(w_pixels), height(h_pixels),
          visible(false),
          equipment_slot_size(60), inventory_slot_size(50),
          inventory_columns(6), inventory_rows(5),
          selected_slot(-1), selecting_equipment(false) {
        
        // Colors
        bg_color = { 10, 10, 15, 230 };        // Dark semi-transparent
        panel_color = { 30, 30, 40, 255 };     // Panel sections
        border_color = { 100, 100, 120, 255 }; 
        text_color = { 255, 255, 255, 255 };
        highlight_color = { 255, 215, 0, 255 };    // Gold highlight
        equipment_color = { 60, 50, 80, 255 };     // Purple-ish
        consumable_color = { 50, 80, 50, 255 };    // Green-ish
        gem_color = { 80, 50, 50, 255 };           // Red-ish
        gold_color = { 255, 215, 0, 255 };
        
        calculate_layout();
    }
    
    void toggle() { visible = !visible; }
    void show() { visible = true; }
    void hide() { visible = false; }
    bool is_visible() const { return visible; }

    // Keyboard navigation of the item grid (the panel used to render a
    // selection highlight for `selected_slot` but nothing ever moved it —
    // InputController swallowed all input except ESC/I while the panel was
    // open, so "Arrow keys to select | Enter to use/equip" did nothing).
    int get_selected_slot() const { return selected_slot; }

    void move_selection(int dx, int dy) {
        int col = (selected_slot >= 0) ? (selected_slot % inventory_columns) : 0;
        int row = (selected_slot >= 0) ? (selected_slot / inventory_columns) : 0;
        col = std::clamp(col + dx, 0, inventory_columns - 1);
        row = std::clamp(row + dy, 0, inventory_rows - 1);
        selected_slot = row * inventory_columns + col;
        selecting_equipment = false;
    }
    
    void render(Entity player) {
        if (!visible || !font) return;
        
        // Draw background overlay (dim game)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_FRect overlay = { 0, 0, 1280, 720 };  // Full screen
        SDL_RenderFillRect(renderer, &overlay);
        
        // Draw main panel background
        SDL_FRect panel_bg = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)
        };
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(renderer, &panel_bg);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, 
                               border_color.b, border_color.a);
        SDL_RenderRect(renderer, &panel_bg);
        
        // Title
        if (title_font) {
            render_text_large("INVENTORY", x + width/2 - 80, y + 10, highlight_color);
        }
        
        // Render sections
        render_equipment_section(player);
        render_inventory_grid(player);
        render_stats_section(player);
        
        // Instructions
        render_text("Press 'I' to close | Arrow keys to select | Enter to use/equip",
                   x + 10, y + height - 25, {150, 150, 150, 255});
    }
    
private:
    void calculate_layout() {
        int padding = 20;
        int section_spacing = 15;
        
        // Equipment section (top, centered)
        int eq_height = 200;
        equipment_section = {
            x + padding,
            y + 60,
            width - 2 * padding,
            eq_height
        };
        
        // Stats section (right side)
        int stats_width = 200;
        stats_section = {
            x + width - stats_width - padding,
            equipment_section.y + equipment_section.h + section_spacing,
            stats_width,
            height - equipment_section.h - 120
        };
        
        // Inventory grid (bottom left)
        inventory_section = {
            x + padding,
            equipment_section.y + equipment_section.h + section_spacing,
            width - stats_width - 3 * padding,
            height - equipment_section.h - 120
        };
    }
    
    void render_equipment_section(Entity player) {
        // Draw section background
        SDL_FRect section_bg = {
            static_cast<float>(equipment_section.x),
            static_cast<float>(equipment_section.y),
            static_cast<float>(equipment_section.w),
            static_cast<float>(equipment_section.h)
        };
        SDL_SetRenderDrawColor(renderer, panel_color.r, panel_color.g, 
                               panel_color.b, panel_color.a);
        SDL_RenderFillRect(renderer, &section_bg);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
                               border_color.b, border_color.a);
        SDL_RenderRect(renderer, &section_bg);
        
        // Title
        render_text("EQUIPMENT", equipment_section.x + 10, 
                   equipment_section.y + 5, text_color);
        
        Equipment* equipment = world->get_component<Equipment>(player);
        
        // Equipment slot layout (paper doll style)
        // Row 1: Helmet, Amulet
        // Row 2: Weapon, Armor, Shield
        // Row 3: Ring1, Belt, Ring2
        
        struct SlotInfo {
            const char* name;
            EquipmentSlot slot;
            int grid_x, grid_y;
        };
        
        SlotInfo slots[] = {
            {"Helmet", EquipmentSlot::HEAD, 1, 0},
            {"Amulet", EquipmentSlot::NECK, 2, 0},
            {"Weapon", EquipmentSlot::MAIN_HAND, 0, 1},
            {"Armor", EquipmentSlot::CHEST, 1, 1},
            {"Shield", EquipmentSlot::OFF_HAND, 2, 1},
            {"Ring1", EquipmentSlot::RING1, 0, 2},
            {"Belt", EquipmentSlot::BELT, 1, 2},
            {"Ring2", EquipmentSlot::RING2, 2, 2}
        };
        
        int start_x = equipment_section.x + (equipment_section.w - 3 * (equipment_slot_size + 10)) / 2;
        int start_y = equipment_section.y + 40;
        
        for (const auto& slot_info : slots) {
            int slot_x = start_x + slot_info.grid_x * (equipment_slot_size + 10);
            int slot_y = start_y + slot_info.grid_y * (equipment_slot_size + 10);
            
            render_equipment_slot(player, equipment, slot_info.slot, 
                                 slot_info.name, slot_x, slot_y);
        }
    }
    
    void render_equipment_slot(Entity player, Equipment* equipment, 
                              EquipmentSlot slot, const char* slot_name,
                              int slot_x, int slot_y) {
        // Draw slot background
        SDL_FRect slot_bg = {
            static_cast<float>(slot_x),
            static_cast<float>(slot_y),
            static_cast<float>(equipment_slot_size),
            static_cast<float>(equipment_slot_size)
        };
        
        SDL_SetRenderDrawColor(renderer, equipment_color.r, equipment_color.g,
                               equipment_color.b, equipment_color.a);
        SDL_RenderFillRect(renderer, &slot_bg);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
                               border_color.b, border_color.a);
        SDL_RenderRect(renderer, &slot_bg);
        
        // Slot label
        render_text_small(slot_name, slot_x + 5, slot_y + 5, {200, 200, 200, 255});
        
        // Draw equipped item (if any)
        if (equipment) {
            Entity item_entity = equipment->get_equipped(slot);
            if (item_entity != 0) {
                Name* item_name = world->get_component<Name>(item_entity);
                if (item_name) {
                    // Item name (abbreviated)
                    std::string abbrev = abbreviate(item_name->name, 8);
                    render_text_centered(abbrev.c_str(), 
                                       slot_x, slot_y + 30,
                                       equipment_slot_size, text_color);
                    
                    // TODO: Add item sprite rendering here
                }
            } else {
                // Empty slot
                render_text_centered("[Empty]", slot_x, slot_y + 30,
                                   equipment_slot_size, {100, 100, 100, 255});
            }
        }
    }
    
    void render_inventory_grid(Entity player) {
        // Draw section background
        SDL_FRect section_bg = {
            static_cast<float>(inventory_section.x),
            static_cast<float>(inventory_section.y),
            static_cast<float>(inventory_section.w),
            static_cast<float>(inventory_section.h)
        };
        SDL_SetRenderDrawColor(renderer, panel_color.r, panel_color.g,
                               panel_color.b, panel_color.a);
        SDL_RenderFillRect(renderer, &section_bg);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
                               border_color.b, border_color.a);
        SDL_RenderRect(renderer, &section_bg);
        
        // Title
        render_text("ITEMS", inventory_section.x + 10,
                   inventory_section.y + 5, text_color);
        
        Inventory* inventory = world->get_component<Inventory>(player);
        if (!inventory) return;
        
        // Draw grid
        int start_x = inventory_section.x + 10;
        int start_y = inventory_section.y + 35;
        int slot_spacing = inventory_slot_size + 5;
        
        for (int row = 0; row < inventory_rows; row++) {
            for (int col = 0; col < inventory_columns; col++) {
                int slot_index = row * inventory_columns + col;
                int slot_x = start_x + col * slot_spacing;
                int slot_y = start_y + row * slot_spacing;
                
                render_inventory_slot(player, inventory, slot_index, slot_x, slot_y);
            }
        }
    }
    
    void render_inventory_slot(Entity player, Inventory* inventory,
                               int slot_index, int slot_x, int slot_y) {
        // Determine background color by item type
        SDL_Color slot_color = panel_color;
        
        if (slot_index < static_cast<int>(inventory->items.size())) {
            Entity item_entity = inventory->items[slot_index];
            Item* item = world->get_component<Item>(item_entity);
            
            if (item) {
                if (item->item_type.find("potion") != std::string::npos) {
                    slot_color = consumable_color;
                } else if (item->item_type.find("gem") != std::string::npos) {
                    slot_color = gem_color;
                }
            }
        }
        
        // Draw slot
        SDL_FRect slot_bg = {
            static_cast<float>(slot_x),
            static_cast<float>(slot_y),
            static_cast<float>(inventory_slot_size),
            static_cast<float>(inventory_slot_size)
        };
        
        SDL_SetRenderDrawColor(renderer, slot_color.r, slot_color.g,
                               slot_color.b, slot_color.a);
        SDL_RenderFillRect(renderer, &slot_bg);
        
        // Highlight if selected
        if (slot_index == selected_slot && !selecting_equipment) {
            SDL_SetRenderDrawColor(renderer, highlight_color.r, highlight_color.g,
                                   highlight_color.b, highlight_color.a);
        } else {
            SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
                                   border_color.b, border_color.a);
        }
        SDL_RenderRect(renderer, &slot_bg);
        
        // Draw item
        if (slot_index < static_cast<int>(inventory->items.size())) {
            Entity item_entity = inventory->items[slot_index];
            Item* item = world->get_component<Item>(item_entity);
            Name* item_name = world->get_component<Name>(item_entity);
            
            if (item && item_name) {
                // Item name (abbreviated)
                std::string abbrev = abbreviate(item_name->name, 6);
                render_text_small(abbrev.c_str(), slot_x + 3, slot_y + 3, text_color);
                
                // Quantity
                if (item->quantity > 1) {
                    std::string qty = "x" + std::to_string(item->quantity);
                    render_text_small(qty.c_str(), 
                                    slot_x + inventory_slot_size - 20,
                                    slot_y + inventory_slot_size - 15,
                                    {200, 200, 200, 255});
                }
            }
        }
    }
    
    void render_stats_section(Entity player) {
        // Draw section background
        SDL_FRect section_bg = {
            static_cast<float>(stats_section.x),
            static_cast<float>(stats_section.y),
            static_cast<float>(stats_section.w),
            static_cast<float>(stats_section.h)
        };
        SDL_SetRenderDrawColor(renderer, panel_color.r, panel_color.g,
                               panel_color.b, panel_color.a);
        SDL_RenderFillRect(renderer, &section_bg);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
                               border_color.b, border_color.a);
        SDL_RenderRect(renderer, &section_bg);
        
        // Title
        render_text("CHARACTER", stats_section.x + 10,
                   stats_section.y + 5, text_color);
        
        // Character stats
        CombatStats* stats = world->get_component<CombatStats>(player);
        Experience* xp = world->get_component<Experience>(player);
        Inventory* inv = world->get_component<Inventory>(player);
        
        int text_y = stats_section.y + 35;
        int line_height = 25;
        int text_x = stats_section.x + 15;
        
        if (stats) {
            render_stat_line("HP:", stats->current_hp, stats->max_hp, 
                           text_x, text_y, {255, 100, 100, 255});
            text_y += line_height;
            
            render_stat_line("Attack:", stats->attack, 
                           text_x, text_y, {255, 200, 100, 255});
            text_y += line_height;
            
            render_stat_line("Defense:", stats->defense,
                           text_x, text_y, {100, 200, 255, 255});
            text_y += line_height;
        }
        
        text_y += 10;  // Spacing
        
        if (xp) {
            render_stat_line("Level:", xp->level,
                           text_x, text_y, {200, 200, 255, 255});
            text_y += line_height;
            
            int current_level_xp = xp->get_current_level_xp();
            int xp_needed = xp->get_current_level_requirement();
            render_stat_line("XP:", current_level_xp, xp_needed,
                           text_x, text_y, {100, 255, 100, 255});
            text_y += line_height;
        }
        
        text_y += 10;  // Spacing
        
        if (inv) {
            render_stat_line("Gold:", inv->gold,
                           text_x, text_y, gold_color);
        }
    }
    
    void render_stat_line(const char* label, int value, 
                         int text_x, int text_y, SDL_Color color) {
        render_text(label, text_x, text_y, text_color);
        std::string val_str = std::to_string(value);
        render_text(val_str.c_str(), text_x + 100, text_y, color);
    }
    
    void render_stat_line(const char* label, int current, int max,
                         int text_x, int text_y, SDL_Color color) {
        render_text(label, text_x, text_y, text_color);
        std::string val_str = std::to_string(current) + "/" + std::to_string(max);
        render_text(val_str.c_str(), text_x + 80, text_y, color);
    }
    
    void render_text(const char* text, int px, int py, SDL_Color color) {
        if (!font) return;
        
        SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_FRect dest = {
                    static_cast<float>(px), static_cast<float>(py),
                    static_cast<float>(surface->w), static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }
    
    void render_text_small(const char* text, int px, int py, SDL_Color color) {
        render_text(text, px, py, color);  // Use same font for now
    }
    
    void render_text_large(const char* text, int px, int py, SDL_Color color) {
        if (!title_font) {
            render_text(text, px, py, color);
            return;
        }
        
        SDL_Surface* surface = TTF_RenderText_Blended(title_font, text, 0, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_FRect dest = {
                    static_cast<float>(px), static_cast<float>(py),
                    static_cast<float>(surface->w), static_cast<float>(surface->h)
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
                    static_cast<float>(text_x), static_cast<float>(cy),
                    static_cast<float>(surface->w), static_cast<float>(surface->h)
                };
                SDL_RenderTexture(renderer, texture, nullptr, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
        }
    }
    
    std::string abbreviate(const std::string& text, size_t max_len) {
        if (text.length() <= max_len) return text;
        return text.substr(0, max_len);
    }
};
