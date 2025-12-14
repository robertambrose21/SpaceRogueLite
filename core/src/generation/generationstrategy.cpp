#include "generation/generationstrategy.h"

using namespace SpaceRogueLite;

GenerationStrategy::GenerationStrategy(const RoomConfiguration& roomConfiguration)
    : roomConfiguration(roomConfiguration) {
    auto& grid = entt::locator<Grid>::value();

    width = grid.getWidth();
    height = grid.getHeight();

    data.resize(height, std::vector<GridTile>(width, TILE_DEFAULT));
}

void GenerationStrategy::setTile(int x, int y, const GridTile& tile) { data[y][x] = tile; }

GridTile GenerationStrategy::getTile(int x, int y) const { return data[y][x]; }

int GenerationStrategy::getWidth(void) const { return width; }

int GenerationStrategy::getHeight(void) const { return height; }

GenerationStrategy::RoomConfiguration GenerationStrategy::getRoomConfiguration(void) const {
    return roomConfiguration;
}

void GenerationStrategy::addRoom(const Room& room) { rooms.push_back(room); }

bool GenerationStrategy::hasCollision(const Room& room, const std::vector<Room>& existingRooms) {
    for (auto existingRoom : existingRooms) {
        if (hasCollision(existingRoom, room)) {
            return true;
        }
    }

    return false;
}

bool GenerationStrategy::hasCollision(const Room& roomA, const Room& roomB) {
    return roomA.min.x < roomB.max.x && roomA.max.x > roomB.min.x && roomA.min.y < roomB.max.y &&
           roomA.max.y > roomB.min.y;
}

bool GenerationStrategy::isSparse(const Room& room, const std::vector<Room>& existingRooms) {
    int sparseness = getRoomConfiguration().sparseness;

    if (sparseness <= 0) {
        return true;
    }

    return shortestDistance(room, existingRooms) >= sparseness;
}

int GenerationStrategy::shortestDistance(const Room& room, const std::vector<Room>& existingRooms) {
    int shortestDistance = std::numeric_limits<int>::max();

    for (auto existingRoom : existingRooms) {
        int dist = distance(room, existingRoom);

        if (dist < shortestDistance) {
            shortestDistance = dist;
        }
    }

    return shortestDistance;
}

int GenerationStrategy::distance(const Room& roomA, const Room& roomB) {
    auto roomACenter = glm::vec2(roomA.max.x - roomA.min.x, roomA.max.y - roomA.min.y);
    auto roomBCenter = glm::vec2(roomB.max.x - roomB.min.x, roomB.max.y - roomB.min.y);

    return static_cast<int>(glm::distance(roomACenter, roomBCenter));
}

const std::vector<GenerationStrategy::Room>& GenerationStrategy::getRooms(void) const {
    return rooms;
}

void GenerationStrategy::clearRooms(void) { rooms.clear(); }

const std::vector<std::vector<GridTile>>& GenerationStrategy::getData(void) const { return data; }