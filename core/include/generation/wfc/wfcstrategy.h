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
#include "generation/generationstrategy.h"
#include "wfctileset.h"

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