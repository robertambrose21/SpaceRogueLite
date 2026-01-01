#include <grid.h>

#include <algorithm>

namespace SpaceRogueLite {

Grid::Grid(int width, int height) : width(width), height(height) {
    tiles.resize(width * height, TILE_DEFAULT);
    markAllDirty();
}

void Grid::setTile(int x, int y, const GridTile& tile) {
    if (!isValidPosition(x, y)) {
        return;
    }

    size_t index = y * width + x;
    if (tiles[index] != tile) {
        tiles[index] = tile;
        expandDirtyRegion(x, y);
    }
}

void Grid::setTiles(const std::vector<GridTile>& newTiles, int newWidth, int newHeight) {
    if (newTiles.size() != static_cast<size_t>(newWidth * newHeight)) {
        return;
    }

    width = newWidth;
    height = newHeight;
    tiles = newTiles;
    markAllDirty();
}

GridTile Grid::getTile(int x, int y) const {
    if (!isValidPosition(x, y)) {
        return TILE_DEFAULT;
    }
    return tiles[y * width + x];
}

int Grid::getWidth() const { return width; }

int Grid::getHeight() const { return height; }

void Grid::resize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) {
        return;
    }

    std::vector<GridTile> newTiles(newWidth * newHeight, TILE_DEFAULT);

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

void Grid::forEachTile(std::function<void(int x, int y, const GridTile&)> callback) const {
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

std::vector<glm::ivec2> Grid::getIntersections(const glm::vec2& p1, const glm::vec2& p2) {
    std::vector<glm::ivec2> intersections;

    // Offset so we get the center of the tile
    auto op1 = p1 + glm::vec2(.5f, .5f);
    auto op2 = p2 + glm::vec2(.5f, .5f);

    int xMin = std::max(0, (int) std::floor(std::min(op1.x, op2.x)));
    int xMax = std::min(getWidth(), (int) std::ceil(std::max(op1.x, op2.x)));
    int yMin = std::max(0, (int) std::floor(std::min(op1.y, op2.y)));
    int yMax = std::min(getHeight(), (int) std::ceil(std::max(op1.y, op2.y)));

    for (int x = xMin; x < xMax; x++) {
        for (int y = yMin; y < yMax; y++) {
            if (hasTileIntersection(op1, op2, x, y)) {
                intersections.push_back(glm::ivec2(x, y));
            }
        }
    }

    return intersections;
}

bool Grid::hasTileIntersection(const glm::vec2& p1, const glm::vec2& p2, int x, int y) {
    std::vector<glm::vec2> corners = {glm::vec2(x, y), glm::vec2(x, y + 1), glm::vec2(x + 1, y),
                                      glm::vec2(x + 1, y + 1)};

    if (!hasPointsOnDifferentSides(p1, p2, corners)) {
        return false;
    } else if (p1.x > corners[3].x && p2.x > corners[3].x) {
        return false;
    } else if (p1.x < corners[0].x && p2.x < corners[0].x) {
        return false;
    } else if (p1.y > corners[3].y && p2.y > corners[3].y) {
        return false;
    } else if (p1.y < corners[0].y && p2.y < corners[0].y) {
        return false;
    }

    return true;
}

bool Grid::hasPointsOnDifferentSides(const glm::vec2& p1, const glm::vec2& p2,
                                     const std::vector<glm::vec2>& corners) {
    float p = pointOnLineSide(p1, p2, corners[0]);

    for (int i = 1; i < 4; i++) {
        p *= pointOnLineSide(p1, p2, corners[i]);
        if (p < 0) {
            return true;
        }
    }

    return false;
}

float Grid::pointOnLineSide(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& point) {
    return ((p2.y - p1.y) * point.x) + ((p1.x - p2.x) * point.y) + (p2.x * p1.y - p1.x * p2.y);
}

}  // namespace SpaceRogueLite
