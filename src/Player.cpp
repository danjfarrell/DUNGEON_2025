// CHANGES since last full view:
// - render now applies origin offsets.

#include "Player.h"

void Player::move(int dx, int dy, const Map& map) {
    const auto& g = map.data();

    int nx = x + dx;
    int ny = y + dy;

    if (ny < 0 || ny >= (int)g.size()) {
        return;
    }
    if (nx < 0 || nx >= (int)g[ny].size()) {
        return;
    }
    if (g[ny][nx] == '#') {
        return;
    }

    x = nx;
    y = ny;
}

void Player::render(SDL_Renderer* r,
    TileSheet& tiles,
    const TileAtlas& a,
    const Camera& cam,
    int originX,
    int originY) const {
    int sx = originX + (x - cam.x) * int(TILE_WIDTH * cam.scale);
    int sy = originY + (y - cam.y) * int(TILE_HEIGHT * cam.scale);

    tiles.renderTile(a.playerX, a.playerY, sx, sy, r);
}
