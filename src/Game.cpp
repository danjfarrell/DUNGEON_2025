// CHANGES since last full view:
// - Computes UI regions and sets renderer scale (ui.scale).
// - Seeds items again (so pickups/inventory work).
// - Adds mouse hover/click handling for spell slots.
// - Adds drawMiniMap (real minimap from map data).
// - Adds drawSpellSidebar (icons from tilesheet + hover outline).
// - drawUIPlaceholders keeps sidebar bg + status strip.

#include "Game.h"
#include <iostream>

Game::Game(SDL_Window* w, SDL_Renderer* r)
    : window(w), renderer(r) {
}

Game::~Game() {
    cleanup();
}

static inline bool PointInFRect(int px, int py, const SDL_FRect& r)
{
    const float fx = static_cast<float>(px);
    const float fy = static_cast<float>(py);
    return (fx >= r.x) && (fx < r.x + r.w) &&
        (fy >= r.y) && (fy < r.y + r.h);
}



void Game::init() {
    running = true;

    SDL_GetRenderOutputSize(renderer, &ui.screenW, &ui.screenH);
    ui.compute();

    //SDL_SetRenderScale(renderer, ui.scale, ui.scale);

    if (!tiles.load(renderer, "assets/tiles/tileset.png")) {
        std::cerr << "[tileset load failed]\n";
        running = false;
        return;
    }

    if (!map.loadFromFile("assets/maps/level1.txt")) {
        std::cerr << "[map load failed]\n";
        running = false;
        return;
    }

    hud.loadFont("assets/fonts/OpenSans-Regular.ttf", 16);

    int visW = ui.mapW / int(TILE_WIDTH * ui.scale);
    int visH = ui.mapH / int(TILE_HEIGHT * ui.scale);
    if (visW < 1) visW = 1;
    if (visH < 1) visH = 1;

    cam.init(ui.mapW, ui.mapH, ui.scale);
    cam.wTiles = visW;
    cam.hTiles = visH;

    player.setPosition(5, 5);
    cam.centerOn(player.getX(), player.getY(), map.width(), map.height());

    // items to pick up (visual + HUD inventory)
    map.addItem(Item{ "Potion",      ItemType::CONSUMABLE, 25 }, 8, 6);
    map.addItem(Item{ "Iron Key",    ItemType::KEY,         0 }, 12, 7);
    map.addItem(Item{ "Fire Scroll", ItemType::SCROLL,     40 }, 14, 9);
}

void Game::run() {
    init();

    if (!running) {
        return;
    }

    while (running) {
        handleEvents();
        update();
        render();
    }
}

void Game::handleEvents() {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            running = false;
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            hoveredSlot = -1;
            for (int i = 0; i < (int)spellSlots.size(); ++i) {
                //SDL_Point p{ e.motion.x, e.motion.y };
                //if (SDL_PointInFRect(&p, &spellSlots[i])) {
                //    hoveredSlot = i;
                //    break;
                //}
                if (PointInFRect(e.motion.x, e.motion.y, spellSlots[i]))
                {
                    hoveredSlot = i;
                    break;
                }

            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (hoveredSlot != -1) {
                std::cout << "Clicked spell slot " << hoveredSlot << "\n";
            }
        }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }
            else if (e.key.scancode == SDL_SCANCODE_UP) {
                player.move(0, -1, map);
            }
            else if (e.key.scancode == SDL_SCANCODE_DOWN) {
                player.move(0, 1, map);
            }
            else if (e.key.scancode == SDL_SCANCODE_LEFT) {
                player.move(-1, 0, map);
            }
            else if (e.key.scancode == SDL_SCANCODE_RIGHT) {
                player.move(1, 0, map);
            }

            // pickup after move
            int idx = map.itemIndexAt(player.getX(), player.getY());
            if (idx != -1) {
                const auto& wi = map.items().at(idx);
                player.inventory().add(wi.item);
                map.removeItemAt(idx);
            }

            // quick-use slot 1: potion
            if (e.key.scancode == SDL_SCANCODE_1) {
                auto& inv = player.inventory();
                const auto& list = inv.items();

                if (!list.empty() && list[0].type == ItemType::CONSUMABLE) {
                    int healed = std::min(100, player.getHealth() + list[0].value);
                    player.setHealth(healed);
                    inv.remove(0);
                }
            }
        }
    }
}

void Game::update() {
    cam.centerOn(player.getX(), player.getY(), map.width(), map.height());
}

void Game::drawMiniMap() {
    int startX = ui.screenW - ui.sidebarW + ui.pad;
    int startY = ui.pad;

    // draw box background first (minimap frame)
    SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
    SDL_FRect frame{ (float)startX, (float)startY, (float)ui.minimapSize, (float)ui.minimapSize };
    SDL_RenderFillRect(renderer, &frame);

    // tile dots
    int tileSize = 3;
    int maxW = std::min(map.width(), ui.minimapSize / tileSize);
    int maxH = std::min(map.height(), ui.minimapSize / tileSize);

    for (int y = 0; y < maxH; ++y) {
        for (int x = 0; x < maxW; ++x) {
            char c = map.data()[y][x];

            if (c == '#') {
                SDL_SetRenderDrawColor(renderer, 105, 105, 105, 255);
            }
            else if (c == '.') {
                SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
            }
            else if (c == 'D') {
                SDL_SetRenderDrawColor(renderer, 190, 140, 60, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            }

            SDL_FRect rect{
                (float)(startX + x * tileSize),
                (float)(startY + y * tileSize),
                (float)tileSize,
                (float)tileSize
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // player marker
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect p{
        (float)(startX + player.getX() * tileSize),
        (float)(startY + player.getY() * tileSize),
        (float)tileSize,
        (float)tileSize
    };
    SDL_RenderFillRect(renderer, &p);
}

void Game::drawSpellSidebar() {
    spellSlots.clear();

    int slotSize = 48;
    int sx = ui.screenW - ui.sidebarW + ui.pad;
    int sy = ui.pad + ui.minimapSize + ui.pad;

    for (int i = 0; i < 6; ++i) {
        SDL_FRect rect{
            (float)sx,
            (float)(sy + i * (slotSize + 8)),
            (float)slotSize,
            (float)slotSize
        };
        spellSlots.push_back(rect);

        // icon from tilesheet – cycle across row (5..10, row 0)
        int tx = 5 + (i % 6);
        int ty = 0;
        tiles.renderTile(tx, ty, (int)rect.x, (int)rect.y, renderer);

        // hover highlight
        if (i == hoveredSlot) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderRect(renderer, &rect);
        }
    }
}

void Game::drawUIPlaceholders() {
    // sidebar background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 30, 30, 50, 220);

    SDL_FRect sidebar{
        (float)(ui.screenW - ui.sidebarW),
        0.0f,
        (float)ui.sidebarW,
        (float)(ui.screenH - ui.hudH)
    };
    SDL_RenderFillRect(renderer, &sidebar);

    // status strip (top-left) – small effect boxes
    SDL_SetRenderDrawColor(renderer, 120, 80, 120, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_FRect fx{
            (float)(ui.pad + i * 28),
            (float)ui.pad,
            20.f,
            20.f
        };
        SDL_RenderFillRect(renderer, &fx);
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // map & player in the main area
    map.render(renderer, tiles, atlas, cam, ui.mapX, ui.mapY);
    player.render(renderer, tiles, atlas, cam, ui.mapX, ui.mapY);

    // UI layers
    drawUIPlaceholders();
    drawMiniMap();
    drawSpellSidebar();

    // bottom overlay HUD
    hud.render(renderer, player);

    SDL_RenderPresent(renderer);
}

void Game::cleanup() {
    tiles.cleanup();
}
