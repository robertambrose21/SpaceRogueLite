#pragma once

#include <SDL3/SDL.h>

#include <grid.h>
#include <tilevariant.h>
#include "textureloader.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace SpaceRogueLite {

constexpr int TILE_SIZE = 32;
constexpr uint8_t SYMMETRIC_TEXTURE_SLOTS = 1;
constexpr uint8_t ROTATABLE_TEXTURE_SLOTS = 4;

class TileAtlas {
public:
    struct TileAtlasVariant {
        uint32_t slot;
        uint8_t orientation;
    };

    explicit TileAtlas(SDL_GPUDevice* device, TextureLoader* textureLoader);
    TileAtlas(const TileAtlas&) = delete;
    TileAtlas& operator=(const TileAtlas&) = delete;
    ~TileAtlas();

    bool initialize();
    bool loadTileVariant(const TileVariant& variant);

    glm::vec4 getTileUV(const GridTile& tile) const;

    SDL_GPUTexture* getTexture() const;
    SDL_GPUSampler* getSampler() const;

    uint32_t getTileCount() const;

    void shutdown();

private:
    SDL_GPUDevice* device;
    TextureLoader* textureLoader;
    SDL_GPUTexture* atlasTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    std::vector<glm::vec4> tileUVs;
    uint32_t nextSlot = 1;

    std::unordered_map<TileVariantKey, std::vector<TileAtlasVariant>, TileVariantKeyHash> variants;

    static constexpr uint32_t ATLAS_SIZE = 1024;
    static constexpr uint32_t TILES_PER_ROW = ATLAS_SIZE / TILE_SIZE;
    static constexpr uint32_t MAX_TILES = TILES_PER_ROW * TILES_PER_ROW;

    bool uploadTileToAtlas(SDL_Surface* surface, uint32_t slot);
    glm::vec4 calculateUV(uint32_t slot) const;

    std::vector<TileAtlasVariant> loadSymmetricTile(SDL_Surface* surface, TileId id,
                                                    const std::string& type);
    std::vector<TileAtlasVariant> loadRotatableTile(SDL_Surface* surface, TileId id,
                                                    const std::string& type);
};

}  // namespace SpaceRogueLite
