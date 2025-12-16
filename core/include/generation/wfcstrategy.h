#pragma once

#include <grid.h>
#include <spdlog/spdlog.h>
#include <fastwfc/tiling_wfc.hpp>
#include <fastwfc/utils/array2D.hpp>
#include <fastwfc/wfc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "generationstrategy.h"
#include "wfctileset.h"

namespace glm {

// Fix using glm vec2s as keys
template <typename T, precision P>
bool operator<(const tvec2<T, P>& a, const tvec2<T, P>& b) {
    return (a.x < b.x || (a.x == b.x && a.y < b.y));
}

struct ivec2Hash {
    std::size_t operator()(const ivec2& v) const noexcept {
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);

        return h1 ^ (h2 << 1);
    }
};

}  // namespace glm

namespace SpaceRogueLite {

class WFCStrategy : public GenerationStrategy {
public:
    WFCStrategy(const RoomConfiguration& roomConfiguration, const WFCTileSet& tileSet);

    std::vector<GridTile> generate(void) override;

private:
    std::optional<Array2D<WFCTileSet::WFCTile>> run(int numAttempts, int& successfulAttempt,
                                                    int& seed);
    std::optional<Array2D<WFCTileSet::WFCTile>> runAttempt(int seed);

    void generateMapEdge(TilingWFC<WFCTileSet::WFCTile>& wfc);
    void generateRoomsAndPaths(TilingWFC<WFCTileSet::WFCTile>& wfc);
    Room generateRoom(TilingWFC<WFCTileSet::WFCTile>& wfc, const std::vector<Room>& existingRooms);
    Room createRandomRoom(void);

    WFCTileSet tileSet;
};

}  // namespace SpaceRogueLite