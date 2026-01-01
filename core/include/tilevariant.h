#pragma once

#include <grid.h>

#include <cstdint>
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
    enum TextureSymmetry : uint8_t { SYMMETRIC, ROTATABLE };

    TileId tileId;
    std::string type;
    uint16_t textureId;
    TextureSymmetry symmetry;

    bool operator<(const TileVariant& other) const {
        if (tileId != other.tileId) return tileId < other.tileId;
        if (type != other.type) return type < other.type;
        if (textureId != other.textureId) return textureId < other.textureId;
        return symmetry < other.symmetry;
    }
};

}  // namespace SpaceRogueLite
