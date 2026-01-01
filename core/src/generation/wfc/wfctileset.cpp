#include "generation/wfc/wfctileset.h"

#include <spdlog/spdlog.h>

using namespace SpaceRogueLite;

WFCTileSet::WFCTileSet(const std::string& rulesFile)
    : rulesFile(rulesFile), isLoaded(false), isError(false), edgeTileIndex(0), roomTileIndex(0) {}

void WFCTileSet::load(void) {
    if (isError) {
        spdlog::error(
            "Error on loading previous tileset '{}', please reset before attempting "
            "to load again",
            rulesFile);
        return;
    }

    if (isLoaded) {
        spdlog::warn("Already loaded tileset '{}', please reset before attempting to load again",
                     rulesFile);
        return;
    }

    isError = true;

    if (!std::filesystem::exists(rulesFile)) {
        spdlog::error("Cannot load rules, path '{}' does not exist", rulesFile);
        return;
    }

    json data = json::parse(std::ifstream(rulesFile));

    auto tilesByName = parseTileDefinitions(data["tiles"]);

    if (!tilesByName) {
        return;
    }

    auto walkableSet = data["walkableTiles"].get<std::set<unsigned>>();
    std::map<std::string, unsigned> nameToIndex;
    unsigned index = 0;

    for (const auto& [name, tile] : *tilesByName) {
        nameToIndex[name] = index;
        index++;

        walkableTiles[tile.tileId] = walkableSet.contains(tile.tileId);
        tileVariants.insert(
            {tile.tileId, tile.name, tile.textureId, toTextureSymmetry(tile.symmetry)});

        auto wfcTile = buildWFCTile(tile);
        if (!wfcTile) {
            return;
        }

        tiles.push_back(*wfcTile);
    }

    parseNeighbours(data["neighbours"], nameToIndex);

    edgeTileIndex = nameToIndex.at(data["edgeTile"].get<std::string>());
    roomTileIndex = nameToIndex.at(data["rooms"]["roomTile"].get<std::string>());

    isError = false;
    isLoaded = true;
}

std::optional<std::map<std::string, WFCTileSet::WFCTile>> WFCTileSet::parseTileDefinitions(
    const json& tilesJson) {
    std::map<std::string, WFCTile> tilesByName;

    for (const auto& tileJson : tilesJson) {
        auto name = tileJson["type"].get<std::string>();

        if (tilesByName.contains(name)) {
            spdlog::error("Duplicate tile '{}' found in rules {}. Cannot generate", name,
                          rulesFile);
            return std::nullopt;
        }

        tilesByName[name] = {
            .tileId = tileJson["tile_id"].get<uint8_t>(),
            .symmetry = getSymmetry(tileJson["symmetry"].get<std::string>()),
            .name = name,
            .weight = tileJson["weight"].get<double>(),
            .textureId = tileJson["textureId"].get<uint16_t>(),
            .orientation = 0,
        };
    }

    return tilesByName;
}

std::optional<Tile<WFCTileSet::WFCTile>> WFCTileSet::buildWFCTile(const WFCTile& tile) {
    switch (tile.symmetry) {
        case Symmetry::X:
            return Tile<WFCTile>{{Array2D<WFCTile>(1, 1, tile)}, tile.symmetry, tile.weight};

        case Symmetry::T:
        case Symmetry::L: {
            std::vector<Array2D<WFCTile>> orientations;
            for (uint8_t i = 0; i < 4; i++) {
                auto variant = tile;
                variant.orientation = i;
                orientations.push_back(Array2D<WFCTile>(1, 1, variant));
            }
            return Tile<WFCTile>{orientations, tile.symmetry, tile.weight};
        }

        case Symmetry::I:
        case Symmetry::backslash:
        case Symmetry::P:
            spdlog::error("Symmetry I, \\ and P are not supported");
            return std::nullopt;

        default:
            spdlog::error("Unknown symmetry '{}'", static_cast<int>(tile.symmetry));
            return std::nullopt;
    }
}

void WFCTileSet::parseNeighbours(const json& neighboursJson,
                                 const std::map<std::string, unsigned>& nameToIndex) {
    for (const auto& neighbour : neighboursJson) {
        neighbours.push_back({
            nameToIndex.at(neighbour["left"].get<std::string>()),
            neighbour["left_orientation"].get<unsigned>(),
            nameToIndex.at(neighbour["right"].get<std::string>()),
            neighbour["right_orientation"].get<unsigned>(),
        });
    }
}

void WFCTileSet::reset(void) {
    tiles.clear();
    neighbours.clear();
    walkableTiles.clear();
    tileVariants.clear();
    isLoaded = false;
    isError = false;
}

Symmetry WFCTileSet::getSymmetry(const std::string& symmetry) {
    if (symmetry.length() != 1) {
        spdlog::warn("Symmetry must be a single character, got '{}', defaulting to 'X'", symmetry);
        return Symmetry::X;
    }

    switch (symmetry[0]) {
        case 'X':
            return Symmetry::X;
        case 'T':
            return Symmetry::T;
        case 'I':
            return Symmetry::I;
        case 'L':
            return Symmetry::L;
        case '\\':
            return Symmetry::backslash;
        case 'P':
            return Symmetry::P;
        default: {
            spdlog::warn("Cannot parse invalid symmetry '{}', defaulting to 'X'", symmetry);
            return Symmetry::X;
        }
    }
}

const std::vector<Tile<WFCTileSet::WFCTile>>& WFCTileSet::getWFCTileVariants(void) const {
    return tiles;
}

const std::set<TileVariant>& WFCTileSet::getTileVariants(void) const { return tileVariants; }

const std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>>& WFCTileSet::getNeighbours(
    void) const {
    return neighbours;
}

const std::map<TileId, bool>& WFCTileSet::getWalkableTiles(void) const { return walkableTiles; }

GridTile::Walkability WFCTileSet::getTileWalkability(TileId id) {
    if (walkableTiles.contains(id)) {
        return walkableTiles[id] ? GridTile::WALKABLE : GridTile::BLOCKED;
    }

    return GridTile::BLOCKED;
}

unsigned WFCTileSet::getEdgeTileIndex(void) const { return edgeTileIndex; }

unsigned WFCTileSet::getRoomTileIndex(void) const { return roomTileIndex; }

TileVariant::TextureSymmetry WFCTileSet::toTextureSymmetry(Symmetry symmetry) {
    if (symmetry == Symmetry::X) {
        return TileVariant::SYMMETRIC;
    }

    return TileVariant::ROTATABLE;
}
