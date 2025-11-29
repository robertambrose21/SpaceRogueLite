#pragma once
#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "tiletypes.h"

namespace SpaceRogueLite {

class TileAtlas {
public:
    explicit TileAtlas(SDL_GPUDevice* device);
    TileAtlas(const TileAtlas&) = delete;
    TileAtlas& operator=(const TileAtlas&) = delete;
    ~TileAtlas();

    // Initialize the atlas texture (must be called before loading tiles)
    bool initialize();

    // Load a tile image, returns assigned TileId (0 is reserved for empty)
    // Returns TILE_EMPTY on failure
    TileId loadTile(const std::string& path);

    glm::vec4 getTileUV(TileId id) const;

    SDL_GPUTexture* getTexture() const;
    SDL_GPUSampler* getSampler() const;

    uint32_t getTileCount() const;

    void shutdown();

private:
    SDL_GPUDevice* device;
    SDL_GPUTexture* atlasTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    std::vector<glm::vec4> tileUVs;  // UV coordinates for each loaded tile
    uint32_t nextSlot = 1;           // Start at 1, 0 is TILE_EMPTY

    static constexpr uint32_t ATLAS_SIZE = 1024;
    static constexpr uint32_t TILES_PER_ROW = ATLAS_SIZE / TILE_SIZE;
    static constexpr uint32_t MAX_TILES = TILES_PER_ROW * TILES_PER_ROW;

    bool uploadTileToAtlas(SDL_Surface* surface, uint32_t slot);
    glm::vec4 calculateUV(uint32_t slot) const;
};

}  // namespace SpaceRogueLite
