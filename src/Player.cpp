#include "Player.h"

void Player::move(int dx, int dy, const Map& map) {
    const auto& g = map.data();
    int nx = x + dx, ny = y + dy;
    if (ny < 0 || ny >= (int)g.size()) return;
    if (nx < 0 || nx >= (int)g[ny].size()) return;
    if (g[ny][nx] == '#') return;  // wall blocks
    x = nx; y = ny;
}

void Player::render(SDL_Renderer* r, TileSheet& tiles, const TileAtlas& atlas) const {
    tiles.renderTile(atlas.playerX, atlas.playerY, x * TILE_WIDTH, y * TILE_HEIGHT, r);
}
