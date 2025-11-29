#pragma once
#include <cstdint>

namespace SpaceRogueLite {

using TileId = uint16_t;  // Supports 65535 tile types

constexpr TileId TILE_EMPTY = 0;
constexpr int TILE_SIZE = 32;  // Pixels per tile

}  // namespace SpaceRogueLite
