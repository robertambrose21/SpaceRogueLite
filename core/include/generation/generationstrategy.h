#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include "grid.h"

namespace SpaceRogueLite {

class GenerationStrategy {
public:
    typedef struct _roomConfiguration {
        int numRooms;
        glm::ivec2 minRoomSize;
        glm::ivec2 maxRoomSize;
        int sparseness = 0;
    } RoomConfiguration;

    typedef struct _room {
        glm::ivec2 min;
        glm::ivec2 max;
    } Room;

    GenerationStrategy(const RoomConfiguration& roomConfiguration);

    virtual std::vector<std::vector<GridTile>> generate(void) = 0;

    void setTile(int x, int y, const GridTile& tile);
    GridTile getTile(int x, int y) const;

    int getWidth(void) const;
    int getHeight(void) const;

    RoomConfiguration getRoomConfiguration(void) const;
    void addRoom(const Room& room);
    const std::vector<Room>& getRooms(void) const;
    void clearRooms(void);
    bool hasCollision(const Room& room, const std::vector<Room>& existingRooms);
    bool hasCollision(const Room& roomA, const Room& roomB);
    bool isSparse(const Room& room, const std::vector<Room>& existingRooms);
    int shortestDistance(const Room& room, const std::vector<Room>& existingRooms);
    int distance(const Room& roomA, const Room& roomB);

    const std::vector<std::vector<GridTile>>& getData(void) const;

private:
    std::vector<std::vector<GridTile>> data;
    int width;
    int height;
    RoomConfiguration roomConfiguration;
    std::vector<Room> rooms;
};

}  // namespace SpaceRogueLite