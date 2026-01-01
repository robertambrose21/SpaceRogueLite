#pragma once

#include <grid.h>
#include <tilevariant.h>

#include <map>
#include <set>

namespace SpaceRogueLite {

class TileSet {
public:
    virtual ~TileSet() = default;

    virtual const std::set<TileVariant>& getTileVariants() const = 0;
    virtual const std::map<TileId, bool>& getWalkableTiles() const = 0;
    virtual GridTile::Walkability getTileWalkability(TileId id) = 0;

    virtual unsigned getEdgeTileIndex() const = 0;
    virtual unsigned getRoomTileIndex() const = 0;

    virtual void load() = 0;
    virtual void reset() = 0;
};

}  // namespace SpaceRogueLite
