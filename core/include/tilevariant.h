#pragma once

#include <grid.h>

#include <functional>
#include <string>
#include <utility>

namespace SpaceRogueLite {

using TileVariantKey = std::pair<TileId, std::string>;

struct TileVariantKeyHash {
    std::size_t operator()(const TileVariantKey& key) const {
        std::size_t h1 = std::hash<TileId>{}(key.first);
        std::size_t h2 = std::hash<std::string>{}(key.second);
        return h1 ^ (h2 << 1);
    }
};

struct TileVariant {
    TileId tileId;
    std::string type;
    uint16_t textureId;

    bool operator<(const TileVariant& other) const {
        if (tileId != other.tileId) return tileId < other.tileId;
        return type < other.type;
    }
};

}  // namespace SpaceRogueLite
