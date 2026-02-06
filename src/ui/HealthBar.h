// ============================================================================
// HealthBar.h - NEW FILE (src/ui/HealthBar.h)
// ============================================================================
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class HealthBar {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    int x, y, width, height;

    SDL_Color bg_color;
    SDL_Color health_color;
    SDL_Color low_health_color;
    SDL_Color border_color;

    float flash_timer;
    bool flashing;

public:
    HealthBar(SDL_Renderer* rend, TTF_Font* f, int pos_x, int pos_y, int w, int h)
        : renderer(rend), font(f), x(pos_x), y(pos_y), width(w), height(h),
        flash_timer(0.0f), flashing(false) {

        bg_color = { 50, 0, 0, 255 };           // Dark red background
        health_color = { 0, 200, 0, 255 };      // Green
        low_health_color = { 200, 0, 0, 255 };  // Red (when low HP)
        border_color = { 100, 100, 100, 255 };  // Gray
    }

    void update(float dt) {
        // Update flash animation
        if (flashing) {
            flash_timer -= dt;
            if (flash_timer <= 0.0f) {
                flashing = false;
            }
        }
    }

    void trigger_flash() {
        flashing = true;
        flash_timer = 0.3f;  // Flash for 0.3 seconds
    }

    void render(int current_hp, int max_hp) {
        // Calculate health percentage
        float health_percent = static_cast<float>(current_hp) / max_hp;

        // Clamp between 0 and 1
        health_percent = std::max(0.0f, std::min(1.0f, health_percent));

        // Background
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g,
            bg_color.b, bg_color.a);
        SDL_FRect bg = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)
        };
        SDL_RenderFillRect(renderer, &bg);

        // Health fill
        int fill_width = static_cast<int>(width * health_percent);

        // Choose color based on health percentage
        SDL_Color fill_color;
        if (health_percent < 0.3f) {
            fill_color = low_health_color;  // Red when below 30%
        }
        else {
            fill_color = health_color;      // Green when healthy
        }

        // Flash white when taking damage
        if (flashing) {
            fill_color = { 255, 255, 255, 255 };  // White flash
        }

        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g,
            fill_color.b, fill_color.a);
        SDL_FRect fill = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(fill_width),
            static_cast<float>(height)
        };
        SDL_RenderFillRect(renderer, &fill);

        // Border
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g,
            border_color.b, border_color.a);
        SDL_RenderRect(renderer, &bg);

        // Text (HP numbers)
        if (font) {
            std::string text = std::to_string(current_hp) + " / " +
                std::to_string(max_hp);
            //TTF_SetFontColor(font, SDL_Color{ 255, 255, 255, 255 });
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(),0,
				SDL_Color{ 255, 255, 255, 255 });
            //    { 255, 255, 255, 255 });

            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    // Center text in bar
                    SDL_FRect text_rect = {
                        static_cast<float>(x + width / 2 - surface->w / 2),
                        static_cast<float>(y + height / 2 - surface->h / 2),
                        static_cast<float>(surface->w),
                        static_cast<float>(surface->h)
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &text_rect);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
        }
    }
};
