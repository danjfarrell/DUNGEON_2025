#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include "SpriteConfig.h"
#include "../components/Components.h"

class SpriteManager {
private:
    SDL_Renderer* renderer;
    std::vector<SDL_Texture*> sprite_sheets;
    int tile_width;
    int tile_height;
    SpriteConfig sprite_config;

public:
    SpriteManager(SDL_Renderer* r, int tile_w = 16, int tile_h = 16)
        : renderer(r), tile_width(tile_w), tile_height(tile_h) {
    }

    ~SpriteManager() {
        for (SDL_Texture* texture : sprite_sheets) {
            if (texture) {
                SDL_DestroyTexture(texture);
            }
        }
    }

    // Load sprite configuration and automatically load all sprite sheets
    bool load_config(const std::string& config_file) {
        // Load the JSON config
        if (!sprite_config.load_from_file(config_file)) {
            return false;
        }

        // Automatically load all sprite sheets defined in the config
        const auto& sheets = sprite_config.get_sprite_sheets();

        // Pre-size the vector to ensure IDs match indices
        sprite_sheets.resize(sheets.size(), nullptr);

        for (const auto& sheet_info : sheets) {
            std::cout << "Loading sprite sheet " << sheet_info.id << ": "
                << sheet_info.filepath << std::endl;

            int loaded_id = load_sprite_sheet_at_id(sheet_info.filepath, sheet_info.id);

            if (loaded_id != sheet_info.id) {
                std::cout << "ERROR: Failed to load sprite sheet at expected ID!" << std::endl;
                return false;
            }
        }

        std::cout << "All sprite sheets loaded successfully!" << std::endl;
        return true;
    }

    // Load a sprite sheet at a specific ID (used internally)
    int load_sprite_sheet_at_id(const std::string& filepath, int target_id) {
        //SDL_Surface* surface = SDL_LoadBMP(filepath.c_str());
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            std::cout << "Failed to load sprite sheet: " << filepath << std::endl;
            std::cout << "SDL Error: " << SDL_GetError() << std::endl;
            return -1;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture) {
            std::cout << "Failed to create texture from: " << filepath << std::endl;
            std::cout << "SDL Error: " << SDL_GetError() << std::endl;
            return -1;
        }

        // Ensure the vector is large enough
        if (target_id >= static_cast<int>(sprite_sheets.size())) {
            sprite_sheets.resize(target_id + 1, nullptr);
        }

        sprite_sheets[target_id] = texture;

        std::cout << "  -> Loaded at ID " << target_id << std::endl;

        return target_id;
    }

    // Legacy function for manual loading (now optional)
    int load_sprite_sheet(const std::string& filepath) {
        //SDL_Surface* surface = SDL_LoadBMP(filepath.c_str());
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            std::cout << "Failed to load sprite sheet: " << filepath << std::endl;
            std::cout << "SDL Error: " << SDL_GetError() << std::endl;
            return -1;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture) {
            std::cout << "Failed to create texture from: " << filepath << std::endl;
            std::cout << "SDL Error: " << SDL_GetError() << std::endl;
            return -1;
        }

        int sheet_id = static_cast<int>(sprite_sheets.size());
        sprite_sheets.push_back(texture);

        std::cout << "Loaded sprite sheet " << sheet_id << ": " << filepath << std::endl;

        return sheet_id;
    }

    // Get sprite definition by name
    const SpriteDefinition* get_sprite(const std::string& name) const {
        return sprite_config.get_sprite(name);
    }

    // Check if sprite exists
    bool has_sprite(const std::string& name) const {
        return sprite_config.has_sprite(name);
    }

    // Create a Renderable component from a sprite name
    Renderable create_renderable(const std::string& name) const {
        const SpriteDefinition* def = get_sprite(name);
        if (def) {
            return Renderable{ def->sheet_id, def->tile_x, def->tile_y, def->layer };
        }
        std::cout << "Warning: Sprite '" << name << "' not found, using default" << std::endl;
        return Renderable{ 0, 0, 0, 0 };
    }

    // Render a specific tile from a sprite sheet
    void render_tile(int sheet_id, int tile_x, int tile_y, int screen_x, int screen_y, int scale = 1) {
        if (sheet_id < 0 || sheet_id >= static_cast<int>(sprite_sheets.size())) {
            return;
        }

        if (!sprite_sheets[sheet_id]) {
            return;  // Texture not loaded
        }

        SDL_FRect src_rect = {
            static_cast<float>(tile_x * tile_width),
            static_cast<float>(tile_y * tile_height),
            static_cast<float>(tile_width),
            static_cast<float>(tile_height)
        };

        SDL_FRect dest_rect = {
            static_cast<float>(screen_x),
            static_cast<float>(screen_y),
            static_cast<float>(tile_width * scale),
            static_cast<float>(tile_height * scale)
        };

        SDL_RenderTexture(renderer, sprite_sheets[sheet_id], &src_rect, &dest_rect);
    }

    // Render a sprite by name
    void render_sprite(const std::string& name, int screen_x, int screen_y, int scale = 1) {
        const SpriteDefinition* def = get_sprite(name);
        if (def) {
            render_tile(def->sheet_id, def->tile_x, def->tile_y, screen_x, screen_y, scale);
        }
        else {
            std::cout << "Warning: Sprite '" << name << "' not found in config" << std::endl;
        }
    }

    int get_tile_width() const { return tile_width; }
    int get_tile_height() const { return tile_height; }

    // Get all sprite names for debugging
    std::vector<std::string> get_all_sprite_names() const {
        return sprite_config.get_all_sprite_names();
    }
};