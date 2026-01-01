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
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <optional>
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

    const std::vector<Tile<WFCTile>>& getWFCTileVariants(void) const;
    const std::set<TileVariant>& getTileVariants(void) const override;
    const std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>>& getNeighbours(
        void) const;

    const std::unordered_map<TileId, bool>& getWalkableTiles(void) const override;
    GridTile::Walkability getTileWalkability(TileId id) override;

    unsigned getEdgeTileIndex(void) const override;
    unsigned getRoomTileIndex(void) const override;

    void load(void) override;
    void reset(void) override;

private:
    Symmetry getSymmetry(const std::string& symmetry);
    static TileVariant::TextureSymmetry toTextureSymmetry(Symmetry symmetry);

    std::optional<std::unordered_map<std::string, WFCTile>> parseTileDefinitions(const json& tilesJson);
    std::optional<Tile<WFCTile>> buildWFCTile(const WFCTile& tile);
    void parseNeighbours(const json& neighboursJson,
                         const std::unordered_map<std::string, unsigned>& nameToIndex);

    bool isError;
    bool isLoaded;
    std::string rulesFile;

    std::vector<Tile<WFCTile>> tiles;
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> neighbours;
    std::unordered_map<TileId, bool> walkableTiles;
    std::set<TileVariant> tileVariants;

    unsigned edgeTileIndex;
    unsigned roomTileIndex;
};

}  // namespace SpaceRogueLite