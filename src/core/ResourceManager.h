// src/core/ResourceManager.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include "../utils/Logger.h"

// RAII wrapper for SDL_Window
class WindowHandle {
private:
    SDL_Window* window;
    
public:
    WindowHandle(const std::string& title, int width, int height, Uint32 flags = 0) {
        window = SDL_CreateWindow(title.c_str(), width, height, flags);
        if (!window) {
            LOG_ERROR("Failed to create window: " + std::string(SDL_GetError()));
            throw std::runtime_error("Window creation failed");
        }
        LOG_INFO("Created window: " + std::to_string(width) + "x" + std::to_string(height));
    }
    
    ~WindowHandle() {
        if (window) {
            SDL_DestroyWindow(window);
            LOG_INFO("Destroyed window");
        }
    }
    
    // Delete copy constructor and assignment
    WindowHandle(const WindowHandle&) = delete;
    WindowHandle& operator=(const WindowHandle&) = delete;
    
    // Allow move
    WindowHandle(WindowHandle&& other) noexcept : window(other.window) {
        other.window = nullptr;
    }
    
    WindowHandle& operator=(WindowHandle&& other) noexcept {
        if (this != &other) {
            if (window) SDL_DestroyWindow(window);
            window = other.window;
            other.window = nullptr;
        }
        return *this;
    }
    
    SDL_Window* get() { return window; }
    operator SDL_Window*() { return window; }
};

// RAII wrapper for SDL_Renderer
class RendererHandle {
private:
    SDL_Renderer* renderer;
    
public:
    explicit RendererHandle(SDL_Window* window, const char* driver = nullptr) {
        renderer = SDL_CreateRenderer(window, driver);
        if (!renderer) {
            LOG_ERROR("Failed to create renderer: " + std::string(SDL_GetError()));
            throw std::runtime_error("Renderer creation failed");
        }
        LOG_INFO("Created renderer");
    }
    
    ~RendererHandle() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            LOG_INFO("Destroyed renderer");
        }
    }
    
    RendererHandle(const RendererHandle&) = delete;
    RendererHandle& operator=(const RendererHandle&) = delete;
    
    RendererHandle(RendererHandle&& other) noexcept : renderer(other.renderer) {
        other.renderer = nullptr;
    }
    
    RendererHandle& operator=(RendererHandle&& other) noexcept {
        if (this != &other) {
            if (renderer) SDL_DestroyRenderer(renderer);
            renderer = other.renderer;
            other.renderer = nullptr;
        }
        return *this;
    }
    
    SDL_Renderer* get() { return renderer; }
    operator SDL_Renderer*() { return renderer; }
};

// RAII wrapper for TTF_Font
class FontHandle {
private:
    TTF_Font* font;
    std::string path;
    int size;
    
public:
    FontHandle(const std::string& font_path, int font_size) 
        : font(nullptr), path(font_path), size(font_size) {
        font = TTF_OpenFont(font_path.c_str(), font_size);
        if (!font) {
            LOG_WARN("Failed to load font: " + font_path + " - " + std::string(SDL_GetError()));
            // Don't throw - font might be optional
        } else {
            LOG_INFO("Loaded font: " + font_path + " (size " + std::to_string(font_size) + ")");
        }
    }
    
    ~FontHandle() {
        if (font) {
            TTF_CloseFont(font);
            LOG_DEBUG("Closed font: " + path);
        }
    }
    
    FontHandle(const FontHandle&) = delete;
    FontHandle& operator=(const FontHandle&) = delete;
    
    FontHandle(FontHandle&& other) noexcept 
        : font(other.font), path(std::move(other.path)), size(other.size) {
        other.font = nullptr;
    }
    
    FontHandle& operator=(FontHandle&& other) noexcept {
        if (this != &other) {
            if (font) TTF_CloseFont(font);
            font = other.font;
            path = std::move(other.path);
            size = other.size;
            other.font = nullptr;
        }
        return *this;
    }
    
    TTF_Font* get() { return font; }
    operator TTF_Font*() { return font; }
    bool is_valid() const { return font != nullptr; }
};

// RAII wrapper for SDL subsystem initialization
class SDLSubsystem {
private:
    bool initialized;
    
public:
    SDLSubsystem() : initialized(false) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            LOG_ERROR("SDL_Init failed: " + std::string(SDL_GetError()));
            throw std::runtime_error("SDL initialization failed");
        }
        LOG_INFO("SDL initialized successfully");
        initialized = true;
    }
    
    ~SDLSubsystem() {
        if (initialized) {
            SDL_Quit();
            LOG_INFO("SDL shut down");
        }
    }
    
    SDLSubsystem(const SDLSubsystem&) = delete;
    SDLSubsystem& operator=(const SDLSubsystem&) = delete;
};

// RAII wrapper for SDL_ttf subsystem
class TTFSubsystem {
private:
    bool initialized;
    
public:
    TTFSubsystem() : initialized(false) {
        if (!TTF_Init()) {
            LOG_ERROR("TTF_Init failed: " + std::string(SDL_GetError()));
            throw std::runtime_error("TTF initialization failed");
        }
        LOG_INFO("TTF initialized successfully");
        initialized = true;
    }
    
    ~TTFSubsystem() {
        if (initialized) {
            TTF_Quit();
            LOG_INFO("TTF shut down");
        }
    }
    
    TTFSubsystem(const TTFSubsystem&) = delete;
    TTFSubsystem& operator=(const TTFSubsystem&) = delete;
};

// Aggregate resource manager - RAII-based cleanup
struct ResourceManager {
    SDLSubsystem sdl;           // Initialized first
    TTFSubsystem ttf;
    WindowHandle window;
    RendererHandle renderer;
    FontHandle ui_font;
    FontHandle title_font;
    
    ResourceManager(const std::string& title, int width, int height,
                   const std::string& font_path, const std::string& font_fallback,
                   int font_size, int title_size)
        : window(title, width, height, 0),
          renderer(window.get()),
          ui_font(try_load_font(font_path, font_fallback, font_size)),
          title_font(try_load_font(font_path, font_fallback, title_size)) {
        LOG_INFO("All resources initialized");
    }
    
    ~ResourceManager() {
        LOG_INFO("Cleaning up resources (RAII)");
        // Resources automatically cleaned up in reverse order:
        // title_font, ui_font, renderer, window, ttf, sdl
    }
    
private:
    static FontHandle try_load_font(const std::string& primary, 
                                   const std::string& fallback, 
                                   int size) {
        FontHandle font(primary, size);
        if (!font.is_valid() && !fallback.empty()) {
            LOG_INFO("Trying fallback font: " + fallback);
            return FontHandle(fallback, size);
        }
        return font;
    }
};