#pragma once
#include <cstdint>
#include <functional>
#include <vector>

namespace SpaceRogueLite {

using TileId = uint16_t;  // Supports 65535 tile types
constexpr TileId TILE_EMPTY = 0;

struct GridRegion {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class Grid {
public:
    Grid(int width, int height);

    void setTile(int x, int y, TileId tile);
    TileId getTile(int x, int y) const;

    int getWidth() const;
    int getHeight() const;
    void resize(int newWidth, int newHeight);

    bool isDirty() const;
    GridRegion getDirtyRegion() const;
    void clearDirty();
    void markAllDirty();

    void forEachTile(std::function<void(int x, int y, TileId)> callback) const;

private:
    int width;
    int height;
    std::vector<TileId> tiles;  // Row-major: tiles[y * width + x]

    bool dirty = false;
    GridRegion dirtyRegion{0, 0, 0, 0};

    void expandDirtyRegion(int x, int y);
    bool isValidPosition(int x, int y) const;
};

}  // namespace SpaceRogueLite
