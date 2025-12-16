#pragma once
#include <SDL3/SDL.h>

#include <grid.h>
#include <tilevariant.h>

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace SpaceRogueLite {

constexpr int TILE_SIZE = 32;  // Pixel width/height of a tile

class TileAtlas {
public:
    struct TileAtlasVariant {
        uint32_t slot;
        uint8_t orientation;
    };

    explicit TileAtlas(SDL_GPUDevice* device);
    TileAtlas(const TileAtlas&) = delete;
    TileAtlas& operator=(const TileAtlas&) = delete;
    ~TileAtlas();

    // Initialize the atlas texture (must be called before loading tiles)
    bool initialize();

    // Load a tile from an existing surface (caller retains ownership and must free surface)
    // Returns false on failure
    bool loadTileFromSurface(SDL_Surface* surface, TileId id, const std::string& type);

    glm::vec4 getTileUV(const GridTile& tile) const;

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

    // Maps (TileId, type) to array of 4 TileVariants (one per orientation: 0, 90, 180, 270)
    std::unordered_map<TileVariantKey, std::array<TileAtlasVariant, 4>, TileVariantKeyHash>
        variants;

    static constexpr uint32_t ATLAS_SIZE = 1024;
    static constexpr uint32_t TILES_PER_ROW = ATLAS_SIZE / TILE_SIZE;
    static constexpr uint32_t MAX_TILES = TILES_PER_ROW * TILES_PER_ROW;

    bool uploadTileToAtlas(SDL_Surface* surface, uint32_t slot);
    glm::vec4 calculateUV(uint32_t slot) const;
};

}  // namespace SpaceRogueLite
