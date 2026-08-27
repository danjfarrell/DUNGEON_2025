// src/config/GameConfig.h
#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"

using json = nlohmann::json;

// Centralized game configuration loaded from JSON
struct GameConfig {
    // Display settings
    struct Display {
        int screen_width = 1280;
        int screen_height = 720;
        int tile_size = 16;
        int tile_scale = 2;
        bool fullscreen = false;
        bool vsync = true;
        
        int get_scaled_tile_size() const { return tile_size * tile_scale; }
    } display;
    
    // Asset paths
    struct Assets {
        std::string sprite_config = "assets/sprites.json";
        std::string enemy_data = "assets/enemies.json";
        std::string font_path = "assets/fonts/DejaVuSansMono.ttf";
        std::string font_fallback = "C:/Windows/Fonts/consola.ttf";
        int font_size = 14;
        int title_font_size = 24;
    } assets;
    
    // Gameplay settings
    struct Gameplay {
        int starting_depth = 1;
        int map_width = 80;
        int map_height = 50;
        int player_starting_hp = 30;
        int player_starting_mp = 50;
        int player_vision_range = 10;
        float target_fps = 60.0f;
        unsigned int seed = 0;
        int victory_depth = 10;  // reach this depth to win
    } gameplay;
    
    // UI Layout percentages (relative to screen size)
    struct UILayout {
        int top_bar_height = 80;
        int minimap_size = 250;
        int message_log_height = 180;
        int hotbar_height = 60;
    } ui_layout;
    
    // Logging settings
    struct Logging {
        std::string log_file = "game_log.txt";
        std::string log_level = "DEBUG";  // DEBUG, INFO, WARNING, ERROR
    } logging;
    
    // Default constructor with sensible defaults
    GameConfig() = default;
    
    // Load from JSON file
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_WARN("Config file not found: " + filepath + ", using defaults");
            return false;
        }
        
        try {
            json config = json::parse(file);
            
            // Parse display settings
            if (config.contains("display")) {
                auto& d = config["display"];
                display.screen_width = d.value("screen_width", display.screen_width);
                display.screen_height = d.value("screen_height", display.screen_height);
                display.tile_size = d.value("tile_size", display.tile_size);
                display.tile_scale = d.value("tile_scale", display.tile_scale);
                display.fullscreen = d.value("fullscreen", display.fullscreen);
                display.vsync = d.value("vsync", display.vsync);
            }
            
            // Parse asset paths
            if (config.contains("assets")) {
                auto& a = config["assets"];
                assets.sprite_config = a.value("sprite_config", assets.sprite_config);
                assets.enemy_data = a.value("enemy_data", assets.enemy_data);
                assets.font_path = a.value("font_path", assets.font_path);
                assets.font_fallback = a.value("font_fallback", assets.font_fallback);
                assets.font_size = a.value("font_size", assets.font_size);
                assets.title_font_size = a.value("title_font_size", assets.title_font_size);
            }
            
            // Parse gameplay settings
            if (config.contains("gameplay")) {
                auto& g = config["gameplay"];
                gameplay.starting_depth = g.value("starting_depth", gameplay.starting_depth);
                gameplay.map_width = g.value("map_width", gameplay.map_width);
                gameplay.map_height = g.value("map_height", gameplay.map_height);
                gameplay.player_starting_hp = g.value("player_starting_hp", gameplay.player_starting_hp);
                gameplay.player_starting_mp = g.value("player_starting_mp", gameplay.player_starting_mp);
                gameplay.player_vision_range = g.value("player_vision_range", gameplay.player_vision_range);
                gameplay.target_fps = g.value("target_fps", gameplay.target_fps);
                gameplay.seed = g.value("seed", gameplay.seed);
                gameplay.victory_depth = g.value("victory_depth", gameplay.victory_depth);
            }
            
            // Parse UI layout
            if (config.contains("ui_layout")) {
                auto& u = config["ui_layout"];
                ui_layout.top_bar_height = u.value("top_bar_height", ui_layout.top_bar_height);
                ui_layout.minimap_size = u.value("minimap_size", ui_layout.minimap_size);
                ui_layout.message_log_height = u.value("message_log_height", ui_layout.message_log_height);
                ui_layout.hotbar_height = u.value("hotbar_height", ui_layout.hotbar_height);
            }
            
            // Parse logging
            if (config.contains("logging")) {
                auto& l = config["logging"];
                logging.log_file = l.value("log_file", logging.log_file);
                logging.log_level = l.value("log_level", logging.log_level);
            }
            
            LOG_INFO("Successfully loaded config from: " + filepath);
            return true;
            
        } catch (const json::exception& e) {
            LOG_ERROR("Failed to parse config: " + std::string(e.what()));
            return false;
        }
    }
    
    // Save current config to file
    bool save_to_file(const std::string& filepath) const {
        json config;
        
        // Display
        config["display"]["screen_width"] = display.screen_width;
        config["display"]["screen_height"] = display.screen_height;
        config["display"]["tile_size"] = display.tile_size;
        config["display"]["tile_scale"] = display.tile_scale;
        config["display"]["fullscreen"] = display.fullscreen;
        config["display"]["vsync"] = display.vsync;
        
        // Assets
        config["assets"]["sprite_config"] = assets.sprite_config;
        config["assets"]["enemy_data"] = assets.enemy_data;
        config["assets"]["font_path"] = assets.font_path;
        config["assets"]["font_fallback"] = assets.font_fallback;
        config["assets"]["font_size"] = assets.font_size;
        config["assets"]["title_font_size"] = assets.title_font_size;
        
        // Gameplay
        config["gameplay"]["starting_depth"] = gameplay.starting_depth;
        config["gameplay"]["map_width"] = gameplay.map_width;
        config["gameplay"]["map_height"] = gameplay.map_height;
        config["gameplay"]["player_starting_hp"] = gameplay.player_starting_hp;
        config["gameplay"]["player_starting_mp"] = gameplay.player_starting_mp;
        config["gameplay"]["player_vision_range"] = gameplay.player_vision_range;
        config["gameplay"]["target_fps"] = gameplay.target_fps;
        config["gameplay"]["victory_depth"] = gameplay.victory_depth;

        // UI Layout
        config["ui_layout"]["top_bar_height"] = ui_layout.top_bar_height;
        config["ui_layout"]["minimap_size"] = ui_layout.minimap_size;
        config["ui_layout"]["message_log_height"] = ui_layout.message_log_height;
        config["ui_layout"]["hotbar_height"] = ui_layout.hotbar_height;
        
        // Logging
        config["logging"]["log_file"] = logging.log_file;
        config["logging"]["log_level"] = logging.log_level;
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to save config to: " + filepath);
            return false;
        }
        
        file << config.dump(4);  // Pretty print with 4-space indent
        LOG_INFO("Saved config to: " + filepath);
        return true;
    }
    
    // Get log level enum from string
    LogLevel get_log_level() const {
        if (logging.log_level == "ERROR") return LogLevel::ERROR;
        if (logging.log_level == "WARNING") return LogLevel::WARNING;
        if (logging.log_level == "INFO") return LogLevel::INFO;
        return LogLevel::DEBUG;
    }
};