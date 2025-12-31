#pragma once

#include <generation/tileset.h>
#include <grid.h>
#include <tilevariant.h>
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

class WFCTileSet : public TileSet {
public:
    typedef struct _wfcTile {
        TileId tileId;
        Symmetry symmetry;
        std::string name;
        double weight;
        uint16_t textureId;
        uint8_t orientation;
    } WFCTile;

    WFCTileSet(const std::string& rulesFile);

    const std::vector<Tile<WFCTile>>& getTiles(void) const;
    const std::set<TileVariant>& getTileVariants(void) const override;
    const std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>>& getNeighbours(
        void) const;

    const std::map<TileId, bool>& getWalkableTiles(void) const override;
    GridTile::Walkability getTileWalkability(TileId id) override;

    TileId getEdgeTile(void) const override;
    TileId getRoomTile(void) const override;

    void load(void) override;
    void reset(void) override;

private:
    Symmetry getSymmetry(char symmetry);
    static TileVariant::TextureSymmetry toTextureSymmetry(Symmetry symmetry);

    bool isError;
    bool isLoaded;
    std::string rulesFile;

    std::vector<Tile<WFCTile>> tiles;
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> neighbours;
    std::map<TileId, bool> walkableTiles;
    std::set<TileVariant> tileVariants;

    TileId edgeTile;
    TileId roomTile;
};

}  // namespace SpaceRogueLite