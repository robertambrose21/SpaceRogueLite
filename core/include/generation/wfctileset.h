#pragma once

#include <grid.h>
#include <fastwfc/tiling_wfc.hpp>
#include <fastwfc/utils/array2D.hpp>
#include <fastwfc/wfc.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace SpaceRogueLite {

class WFCTileSet {
public:
    typedef struct _wfcTile {
        TileId tileId;
        Symmetry symmetry;
        std::string name;
        double weight;
        int textureId;
        uint8_t orientation;
    } WFCTile;

    WFCTileSet(const std::string& rulesFile);

    const std::vector<Tile<WFCTile>>& getTiles(void) const;
    const std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>>& getNeighbours(
        void) const;
    const std::map<std::string, WFCTile>& getTileMapping(void) const;

    const std::map<unsigned, bool>& getWalkableTiles(void) const;
    bool isTileWalkable(unsigned id);

    unsigned getEdgeTile(void) const;
    unsigned getRoomTile(void) const;

    void load(void);
    void reset(void);

private:
    bool validate(void) const;
    Symmetry getSymmetry(char symmetry);

    bool isError;
    bool isLoaded;
    std::string rulesFile;

    std::vector<Tile<WFCTile>> tiles;
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> neighbours;
    std::map<unsigned, bool> walkableTiles;
    std::map<std::string, WFCTile> tileMapping;
    unsigned edgeTile;
    unsigned roomTile;
};

}  // namespace SpaceRogueLite