#include "generation/wfctileset.h"

#include <spdlog/spdlog.h>

using namespace SpaceRogueLite;

WFCTileSet::WFCTileSet(const std::string& rulesFile)
    : rulesFile(rulesFile), isLoaded(false), isError(false), edgeTile(0), roomTile(0) {}

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

    // Assume an error state until we've finished loading
    isError = true;

    if (!std::filesystem::exists(rulesFile)) {
        spdlog::error("Cannot load rules, path '{}' does not exist", rulesFile);
        return;
    }

    std::ifstream f(rulesFile);
    json data = json::parse(f);

    auto tilesJson = data["tiles"].get<std::vector<json>>();
    std::map<std::string, WFCTile> tileMapping;

    for (auto const& tile : tilesJson) {
        auto type = tile["type"].get<std::string>();

        if (tileMapping.contains(type)) {
            spdlog::error("Duplicate tile '{}' found in rules {}. Cannot generate", type,
                          rulesFile);
            return;
        }

        tileMapping[type] = {tile["tile_id"].get<uint8_t>(),
                             getSymmetry(tile["symmetry"].get<std::string>()[0]), type,
                             tile["weight"].get<double>(), tile["textureId"].get<uint16_t>()};
    }

    std::map<std::string, unsigned> tileIndexMapping;
    unsigned index = 0;
    for (auto [type, tile] : tileMapping) {
        tileIndexMapping[type] = index++;
    }

    auto neighboursJson = data["neighbours"].get<std::vector<json>>();

    for (auto const& neighbour : neighboursJson) {
        neighbours.push_back({tileIndexMapping[neighbour["left"].get<std::string>()],
                              neighbour["left_orientation"].get<unsigned>(),
                              tileIndexMapping[neighbour["right"].get<std::string>()],
                              neighbour["right_orientation"].get<unsigned>()});
    }

    auto walkableTilesJson = data["walkableTiles"].get<std::set<unsigned>>();

    for (auto const& [name, tile] : tileMapping) {
        walkableTiles[tile.tileId] = walkableTilesJson.contains(tile.tileId);

        tileVariants.insert(
            {tile.tileId, tile.name, tile.textureId, toTextureSymmetry(tile.symmetry)});

        switch (tile.symmetry) {
            // No symmetry
            case Symmetry::X:
                tiles.push_back({{Array2D<WFCTile>(1, 1, tile)}, tile.symmetry, tile.weight});
                break;

            // Rotational symmetry
            case Symmetry::T:
            case Symmetry::L: {
                std::vector<Array2D<WFCTile>> data;

                // 0 -> 3 becomes 0 -> 270 rotational degrees
                for (int i = 0; i < 4; i++) {
                    auto variant = tile;
                    variant.orientation = i;
                    data.push_back(Array2D<WFCTile>(1, 1, variant));
                }

                tiles.push_back({data, tile.symmetry, tile.weight});
                break;
            }

            // TODO:
            case Symmetry::I:
            case Symmetry::backslash:
            case Symmetry::P:
                spdlog::warn("Symmetry I, \\ and P are currently unsupported");
                break;

            // Exit if we somehow get a weird symmetry
            default:
                spdlog::error("Unknown symmetry '{}'", (int) tile.symmetry);
                return;
        }
    }

    edgeTile = tileIndexMapping[data["edgeTile"].get<std::string>()];
    roomTile = tileIndexMapping[data["rooms"]["roomTile"].get<std::string>()];

    isError = false;
    isLoaded = true;
}

void WFCTileSet::reset(void) {
    tiles.clear();
    neighbours.clear();
    walkableTiles.clear();
    tileVariants.clear();
    isLoaded = false;
    isError = false;
}

Symmetry WFCTileSet::getSymmetry(char symmetry) {
    switch (symmetry) {
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

const std::vector<Tile<WFCTileSet::WFCTile>>& WFCTileSet::getTiles(void) const { return tiles; }

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

TileId WFCTileSet::getEdgeTile(void) const { return edgeTile; }

TileId WFCTileSet::getRoomTile(void) const { return roomTile; }

TileVariant::TextureSymmetry WFCTileSet::toTextureSymmetry(Symmetry symmetry) {
    if (symmetry == Symmetry::X) {
        return TileVariant::SYMMETRIC;
    }

    return TileVariant::ROTATABLE;
}
