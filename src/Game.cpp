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

    if (!tiles_.load(renderer, "assets/tiles/tileset.png")) {
        std::cerr << "[tileset load failed] ensure assets/tiles/tileset.png exists\n";
        running = false; return;
    }
    if (!map_.loadFromFile("assets/maps/level1.txt")) {
        std::cerr << "[map load failed] assets/maps/level1.txt\n";
        running = false; return;
    }

}

//    // Load tileset
//    if (!tiles.load(renderer, "assets/tiles/tileset.png")) {
//        std::cerr << "Failed to load tileset\n";
//        running = false;
//        return;
//    }
//
//    // Load tile atlas (JSON-based — coming soon)
//    if (!atlas.loadFromFile("assets/config/tilemap.json")) {
//        std::cerr << "Failed to load tile atlas\n";
//        running = false;
//        return;
//    }
//
//    // Load font
//    if (!hud.loadFont("assets/fonts/OpenSans-Regular.ttf", 16)) {
//        std::cerr << "Failed to load font\n";
//        running = false;
//        return;
//    }
//
//    // Load map
//    if (!map.loadFromFile("assets/maps/level1.txt")) {
//        std::cerr << "Failed to load map\n";
//        running = false;
//        return;
//    }
//
//    // Place initial items and enemies
//    map.addItem(Item("Potion", ItemType::CONSUMABLE, 25), 3, 3);
//    map.addItem(Item("Fire Scroll", ItemType::SCROLL, 30), 4, 4);
//    map.addItem(Item("Iron Key", ItemType::KEY), 5, 5);
//
//    enemies.emplace_back(Enemy(6, 6, 20));
//}
//
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
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        //input.processEvent(event);



        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }
        }

    }
}

void Game::update() {
//    input.update();
//
//    // Movement input (arrow keys)
//    const Uint8* keys = SDL_GetKeyboardState(nullptr);
//    if (keys[SDL_SCANCODE_UP])    player.move(0, -1, map, enemies);
//    if (keys[SDL_SCANCODE_DOWN])  player.move(0, 1, map, enemies);
//    if (keys[SDL_SCANCODE_LEFT])  player.move(-1, 0, map, enemies);
//    if (keys[SDL_SCANCODE_RIGHT]) player.move(1, 0, map, enemies);
//
//    // Select inventory slot
//    for (int i = 0; i < 5; ++i) {
//        if (keys[SDL_SCANCODE_1 + i]) {
//            player.setSelectedSlot(i);
//        }
//    }
//
//    // Use selected item
//    auto& inv = player.getInventory();
//    const auto& items = inv.getItems();
//    int sel = player.getSelectedSlot();
//    if (sel < items.size()) {
//        const Item& item = items[sel];
//        if (item.getType() == ItemType::CONSUMABLE) {
//            player.setHealth(player.getHealth() + item.getValue());
//            inv.removeItem(sel);
//        }
//        else if (item.getType() == ItemType::SCROLL) {
//            if (!enemies.empty()) enemies.erase(enemies.begin());  // simulate kill
//            inv.removeItem(sel);
//        }
//    }
}

void Game::render() {
   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
   SDL_RenderClear(renderer);

   map_.render(renderer, tiles_, atlas_);
//
//    map.render(renderer, tiles, atlas);
//    for (const auto& e : enemies) {
//        tiles.renderTile(atlas.getTileX("enemy"), atlas.getTileY("enemy"),
//            e.getX() * TILE_WIDTH, e.getY() * TILE_HEIGHT, renderer);
//    }
//
//    player.render(renderer, tiles, atlas);
//    hud.render(renderer, player);
//
    SDL_RenderPresent(renderer);
}

void Game::cleanup() {
    tiles_.cleanup();
//    // No SDL_Quit or TTF_Quit here — done in main
}
