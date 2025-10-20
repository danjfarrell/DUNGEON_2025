#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "../utils/Logger.h"


using json = nlohmann::json;

// Info about a sprite sheet file
struct SpriteSheetInfo {
    int id;
    std::string name;
    std::string filepath;

    SpriteSheetInfo(int i = 0, const std::string& n = "", const std::string& f = "")
        : id(i), name(n), filepath(f) {
    }
};

// Defines a sprite's location in a sprite sheet
struct SpriteDefinition {
    int sheet_id;
    int tile_x;
    int tile_y;
    int layer;

    SpriteDefinition(int sheet = 0, int tx = 0, int ty = 0, int l = 0)
        : sheet_id(sheet), tile_x(tx), tile_y(ty), layer(l) {
    }
};

class SpriteConfig {
private:
    std::vector<SpriteSheetInfo> sprite_sheets;
    std::unordered_map<std::string, SpriteDefinition> sprite_definitions;

public:
    // Load configuration from a JSON file
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);

        if (!file.is_open()) {
            std::cout << "Failed to open sprite config: " << filepath << std::endl;
            return false;
        }

        try {
            json config = json::parse(file);

            // Parse sprite sheet definitions
            if (config.contains("sprite_sheets")) {
                for (const auto& sheet : config["sprite_sheets"]) {
                    SpriteSheetInfo info;
                    info.id = sheet["id"];
                    info.name = sheet["name"];
                    info.filepath = sheet["file"];

                    sprite_sheets.push_back(info);

                    std::cout << "Found sprite sheet: " << info.name
                        << " (id:" << info.id << " file:" << info.filepath << ")" << std::endl;
                }
            }

            // Parse sprite definitions
            if (config.contains("sprites")) {
                for (const auto& item : config["sprites"].items()) {
                    std::string name = item.key();
                    const json& sprite_data = item.value();

                    SpriteDefinition def;
                    def.sheet_id = sprite_data["sheet_id"];
                    def.tile_x = sprite_data["tile_x"];
                    def.tile_y = sprite_data["tile_y"];
                    def.layer = sprite_data.value("layer", 0);

                    sprite_definitions[name] = def;

                    std::cout << "Loaded sprite: " << name
                        << " (sheet:" << def.sheet_id
                        << " pos:" << def.tile_x << "," << def.tile_y
                        << " layer:" << def.layer << ")" << std::endl;
                }
            }

            std::cout << "Successfully loaded " << sprite_sheets.size() << " sprite sheets and "
                << sprite_definitions.size() << " sprite definitions" << std::endl;
            return true;

        }
        catch (const json::exception& e) {
            std::cout << "JSON parsing error: " << e.what() << std::endl;
            return false;
        }
    }

    // Get sprite definition by name
    const SpriteDefinition* get_sprite(const std::string& name) const {
        auto it = sprite_definitions.find(name);
        if (it != sprite_definitions.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Check if a sprite exists
    bool has_sprite(const std::string& name) const {
        return sprite_definitions.find(name) != sprite_definitions.end();
    }

    // Get all sprite sheet info
    const std::vector<SpriteSheetInfo>& get_sprite_sheets() const {
        return sprite_sheets;
    }

    // Get all sprite names (useful for debugging)
    std::vector<std::string> get_all_sprite_names() const {
        std::vector<std::string> names;
        for (const auto& pair : sprite_definitions) {
            names.push_back(pair.first);
        }
        return names;
    }

    void dump_to_log() const {
        LOG_INFO("=== SPRITE CONFIG DUMP ===");
        LOG_INFO("Number of sprite sheets: " + std::to_string(sprite_sheets.size()));

        for (const auto& sheet : sprite_sheets) {
            LOG_INFO("Sheet " + std::to_string(sheet.id) + ": " +
                sheet.name + " (" + sheet.filepath + ")");
        }

        LOG_INFO("Number of sprite definitions: " + std::to_string(sprite_definitions.size()));

        for (const auto& pair : sprite_definitions) {
            const std::string& name = pair.first;
            const SpriteDefinition& def = pair.second;

            LOG_DEBUG("  '" + name + "' -> sheet:" + std::to_string(def.sheet_id) +
                " tile:(" + std::to_string(def.tile_x) + "," + std::to_string(def.tile_y) + ")" +
                " layer:" + std::to_string(def.layer));
        }

        LOG_INFO("=== END SPRITE CONFIG DUMP ===");
    }


};