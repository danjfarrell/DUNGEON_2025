// ============================================================================
// MessageLog.h - NEW FILE (src/ui/MessageLog.h)
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

struct Message {
    std::string text;
    SDL_Color color;
    int turn_number;  // When it was added (useful for filtering old messages)

    Message(const std::string& txt, SDL_Color col = { 255, 255, 255, 255 }, int turn = 0)
        : text(txt), color(col), turn_number(turn) {
    }
};

class MessageLog {
private:
    std::deque<Message> messages;
    size_t max_messages;
    int current_turn;

    // Rendering
    TTF_Font* font;
    SDL_Renderer* renderer;
    int x, y;           // Position on screen
    int width, height;  // Display area size
    int line_height;
    int max_visible_lines;

public:
    MessageLog(SDL_Renderer* rend, int pos_x, int pos_y, int w, int h, int max_msgs = 100)
        : max_messages(max_msgs), current_turn(0), font(nullptr), renderer(rend),
        x(pos_x), y(pos_y), width(w), height(h), line_height(16), max_visible_lines(h / 16) {
    }

    ~MessageLog() {
        if (font) {
            TTF_CloseFont(font);
        }
    }

    // Initialize with a font (call after TTF_Init)
    bool init_font(const std::string& font_path, int font_size = 14) {
        font = TTF_OpenFont(font_path.c_str(), font_size);
        if (!font) {
            std::cerr << "Failed to load font: " << SDL_GetError() << std::endl;
            return false;
        }
        line_height = TTF_GetFontLineSkip(font);
        max_visible_lines = height / line_height;
        return true;
    }

    TTF_Font* get_font() const { return font; }

    // Add a message
    void add_message(const std::string& text, SDL_Color color = { 255, 255, 255, 255 }) {
        messages.push_back(Message(text, color, current_turn));

        // Keep log size manageable
        if (messages.size() > max_messages) {
            messages.pop_front();
        }

        // Also log to console/file for debugging
        std::cout << "[Turn " << current_turn << "] " << text << std::endl;
    }

    // Convenience methods with preset colors
    void add_info(const std::string& text) {
        add_message(text, { 200, 200, 200, 255 });  // Light gray
    }

    void add_combat(const std::string& text) {
        add_message(text, { 255, 100, 100, 255 });  // Red
    }

    void add_success(const std::string& text) {
        add_message(text, { 100, 255, 100, 255 });  // Green
    }

    void add_warning(const std::string& text) {
        add_message(text, { 255, 255, 100, 255 });  // Yellow
    }

    void add_lore(const std::string& text) {
        add_message(text, { 150, 150, 255, 255 });  // Blue
    }

    // Advance the turn counter
    void next_turn() {
        current_turn++;
    }

    // Clear all messages
    void clear() {
        messages.clear();
    }

    // Render the message log
    void render() {
        if (!font || messages.empty()) return;

        // Draw semi-transparent background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_FRect bg_rect = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)
        };
        SDL_RenderFillRect(renderer, &bg_rect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderRect(renderer, &bg_rect);

        // Render messages (most recent at bottom)
        int lines_to_show = std::min(static_cast<int>(messages.size()), max_visible_lines);
        int start_index = messages.size() - lines_to_show;

        for (int i = 0; i < lines_to_show; i++) {
            const Message& msg = messages[start_index + i];

            SDL_Surface* surface = TTF_RenderText_Blended(
                font,
                msg.text.c_str(),
                0,
                msg.color
            );

            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                if (texture) {
                    SDL_FRect dest = {
                        static_cast<float>(x + 5),
                        static_cast<float>(y + 5 + i * line_height),
                        static_cast<float>(surface->w),
                        static_cast<float>(surface->h)
                    };

                    SDL_RenderTexture(renderer, texture, nullptr, &dest);
                    SDL_DestroyTexture(texture);
                }

                SDL_DestroySurface(surface);
            }
        }
    }

    // Get recent messages as strings (for saving/loading)
    std::vector<std::string> get_recent_messages(int count = 10) const {
        std::vector<std::string> result;
        int start = std::max(0, static_cast<int>(messages.size()) - count);

        for (size_t i = start; i < messages.size(); i++) {
            result.push_back(messages[i].text);
        }

        return result;
    }
};
