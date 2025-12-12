#include <grid.h>

#include <algorithm>

namespace SpaceRogueLite {

Grid::Grid(int width, int height) : width(width), height(height) {
    tiles.resize(width * height, TILE_EMPTY);
    markAllDirty();
}

void Grid::setTile(int x, int y, TileId tile) {
    if (!isValidPosition(x, y)) {
        return;
    }

    size_t index = y * width + x;
    if (tiles[index] != tile) {
        tiles[index] = tile;
        expandDirtyRegion(x, y);
    }
}

TileId Grid::getTile(int x, int y) const {
    if (!isValidPosition(x, y)) {
        return TILE_EMPTY;
    }
    return tiles[y * width + x];
}

int Grid::getWidth() const { return width; }

int Grid::getHeight() const { return height; }

void Grid::resize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) {
        return;
    }

    std::vector<TileId> newTiles(newWidth * newHeight, TILE_EMPTY);

    // Copy existing tiles that fit in new dimensions
    int copyWidth = std::min(width, newWidth);
    int copyHeight = std::min(height, newHeight);

    for (int y = 0; y < copyHeight; ++y) {
        for (int x = 0; x < copyWidth; ++x) {
            newTiles[y * newWidth + x] = tiles[y * width + x];
        }
    }

    tiles = std::move(newTiles);
    width = newWidth;
    height = newHeight;
    markAllDirty();
}

bool Grid::isDirty() const { return dirty; }

GridRegion Grid::getDirtyRegion() const { return dirtyRegion; }

void Grid::clearDirty() {
    dirty = false;
    dirtyRegion = {0, 0, 0, 0};
}

void Grid::markAllDirty() {
    dirty = true;
    dirtyRegion = {0, 0, width, height};
}

void Grid::forEachTile(std::function<void(int x, int y, TileId)> callback) const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            callback(x, y, tiles[y * width + x]);
        }
    }
}

void Grid::expandDirtyRegion(int x, int y) {
    if (!dirty) {
        dirty = true;
        dirtyRegion = {x, y, 1, 1};
    } else {
        int minX = std::min(dirtyRegion.x, x);
        int minY = std::min(dirtyRegion.y, y);
        int maxX = std::max(dirtyRegion.x + dirtyRegion.width, x + 1);
        int maxY = std::max(dirtyRegion.y + dirtyRegion.height, y + 1);

        dirtyRegion.x = minX;
        dirtyRegion.y = minY;
        dirtyRegion.width = maxX - minX;
        dirtyRegion.height = maxY - minY;
    }
}

bool Grid::isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

}  // namespace SpaceRogueLite
