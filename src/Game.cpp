#include "Game.h"
#include <iostream>

Game::Game(SDL_Window* win, SDL_Renderer* rend)
    : window(win), renderer(rend) {
}

Game::~Game() {
    cleanup();
}

void Game::init() {
    running = true;

    //SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    //cam.init(screenW, screenH, 2.0f);  // keep camera in sync
    cam.init(screenW, screenH, 1.0f);  // keep camera in sync

    if (!tiles.load(renderer, "assets/tiles/tileset.png")) {
        std::cerr << "[tileset load failed] ensure assets/tiles/tileset.png exists\n";
        running = false; return;
    }
    if (!map.loadFromFile("assets/maps/level1.txt")) {
        std::cerr << "[map load failed] assets/maps/level1.txt\n";
        running = false; return;
    }
    if (!hud.loadFont("assets/fonts/OpenSans-Regular.ttf", 16)) {
        SDL_Log("[font missing] assets/fonts/OpenSans-Regular.ttf");
        // continue without text
    }

    player.setPosition(2, 2);

    cam.centerOn(player.getX(), player.getY(), map.width(), map.height());

    // place some items
    map.addItem(Item{ "Potion", ItemType::CONSUMABLE, 25 }, 4, 4);
    map.addItem(Item{ "Iron Key", ItemType::KEY, 0 }, 5, 5);
    map.addItem(Item{ "Fire Scroll", ItemType::SCROLL, 40 }, 6, 6);
}



void Game::run() {
    init();
    if (!running) return;

    while (running) {
        handleEvents();
        update();
        render();
        //SDL_Delay(16);  // ~60 FPS (basic timing)
    }
}

void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        //input.processEvent(event);



        if (e.type == SDL_EVENT_QUIT) {
            running = false;
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
        }

        // after movement: check pickup
        int idx = map.itemIndexAt(player.getX(), player.getY());
        if (idx != -1) {
            const auto& wi = map.items()[idx];
            player.inventory().add(wi.item);
            map.removeItemAt(idx);
        }

        // Optional quick-use: press '1' to consume first item if it's a potion.
        if (e.key.scancode == SDL_SCANCODE_1) {
            auto& inv = player.inventory();
            const auto& list = inv.items();
            if (!list.empty() && list[0].type == ItemType::CONSUMABLE) {
                player.setHealth(std::min(100, player.getHealth() + list[0].value));
                inv.remove(0);
            }
        }

    }
}

void Game::update() {
    cam.centerOn(player.getX(), player.getY(), map.width(), map.height());
}

void Game::render() {
   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
   SDL_RenderClear(renderer);

   map.render(renderer, tiles, atlas,cam);
   player.render(renderer, tiles, atlas,cam);
   hud.render(renderer, player);

//
//    map.render(renderer, tiles, atlas);
//    for (const auto& e : enemies) {
//        tiles.renderTile(atlas.getTileX("enemy"), atlas.getTileY("enemy"),
//            e.getX() * TILE_WIDTH, e.getY() * TILE_HEIGHT, renderer);
//    }

    SDL_RenderPresent(renderer);
}

void Game::cleanup() {
    tiles.cleanup();
//    // No SDL_Quit or TTF_Quit here — done in main
}
