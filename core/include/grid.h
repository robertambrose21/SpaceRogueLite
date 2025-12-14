#pragma once

#include <cstdint>
#include <fastwfc/wfc.hpp>
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

struct GridTile {
    enum Walkability : uint8_t {
        WALKABLE,
        BLOCKED,
    };

    TileId id = TILE_EMPTY;
    Walkability walkable = BLOCKED;

    // Rotation: 0, 1, 2, 3 -> 0, 90, 180, 270 degrees
    uint8_t orientation = 0;

    bool operator==(const GridTile& other) const {
        return id == other.id && walkable == other.walkable && orientation == other.orientation;
    }

    bool operator!=(const GridTile& other) const { return !(*this == other); }
};

constexpr GridTile TILE_DEFAULT = {TILE_EMPTY, GridTile::BLOCKED, 0};

class Grid {
public:
    Grid(int width, int height);

    void setTile(int x, int y, const GridTile& tile);
    GridTile getTile(int x, int y) const;

    int getWidth() const;
    int getHeight() const;
    void resize(int newWidth, int newHeight);

    bool isDirty() const;
    GridRegion getDirtyRegion() const;
    void clearDirty();
    void markAllDirty();

    void forEachTile(std::function<void(int x, int y, const GridTile&)> callback) const;

private:
    int width;
    int height;
    std::vector<GridTile> tiles;  // Row-major: tiles[y * width + x]

    bool dirty = false;
    GridRegion dirtyRegion{0, 0, 0, 0};

    void expandDirtyRegion(int x, int y);
    bool isValidPosition(int x, int y) const;
};

}  // namespace SpaceRogueLite
