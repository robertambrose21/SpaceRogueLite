#include "renderlayers/tiles/tileatlas.h"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

namespace SpaceRogueLite {

static SDL_Surface* rotateSurface90CCW(SDL_Surface* src) {
    if (!src) return nullptr;

    int w = src->w;
    int h = src->h;

    SDL_Surface* dst = SDL_CreateSurface(h, w, src->format);
    if (!dst) {
        spdlog::error("Failed to create rotated surface: {}", SDL_GetError());
        return nullptr;
    }

    if (!SDL_LockSurface(src)) {
        SDL_DestroySurface(dst);
        return nullptr;
    }
    if (!SDL_LockSurface(dst)) {
        SDL_UnlockSurface(src);
        SDL_DestroySurface(dst);
        return nullptr;
    }

    uint8_t* srcPixels = static_cast<uint8_t*>(src->pixels);
    uint8_t* dstPixels = static_cast<uint8_t*>(dst->pixels);
    int bytesPerPixel = SDL_BYTESPERPIXEL(src->format);

    // Rotate 90° CCW: src(x, y) -> dst(y, w-1-x)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int srcIdx = y * src->pitch + x * bytesPerPixel;
            int dstX = y;
            int dstY = w - 1 - x;
            int dstIdx = dstY * dst->pitch + dstX * bytesPerPixel;

            for (int b = 0; b < bytesPerPixel; b++) {
                dstPixels[dstIdx + b] = srcPixels[srcIdx + b];
            }
        }
    }

    SDL_UnlockSurface(src);
    SDL_UnlockSurface(dst);

    return dst;
}

TileAtlas::TileAtlas(SDL_GPUDevice* device) : device(device) {
    // Reserve slot 0 for TILE_EMPTY
    tileUVs.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
}

TileAtlas::~TileAtlas() { shutdown(); }

bool TileAtlas::initialize() {
    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = ATLAS_SIZE;
    textureInfo.height = ATLAS_SIZE;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    atlasTexture = SDL_CreateGPUTexture(device, &textureInfo);
    if (!atlasTexture) {
        spdlog::error("Failed to create tile atlas texture: {}", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    sampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!sampler) {
        spdlog::error("Failed to create tile atlas sampler: {}", SDL_GetError());
        return false;
    }

    spdlog::debug("TileAtlas initialized ({}x{}, max {} tiles)", ATLAS_SIZE, ATLAS_SIZE, MAX_TILES);
    return true;
}

bool TileAtlas::loadTileFromSurface(SDL_Surface* surface, TileId id, const std::string& type,
                                    TileVariant::TextureSymmetry symmetry) {
    if (!surface) {
        spdlog::error("Cannot load tile variant '{}': null surface provided", type);
        return false;
    }

    std::vector<TileAtlasVariant> tileVariants;

    switch (symmetry) {
        case TileVariant::SYMMETRIC:
            tileVariants = loadSymmetricTile(surface, id, type);
            break;
        case TileVariant::ROTATABLE:
            tileVariants = loadRotatableTile(surface, id, type);
            break;
    }

    if (tileVariants.empty()) {
        return false;
    }

    variants[{id, type}] = std::move(tileVariants);
    return true;
}

std::vector<TileAtlas::TileAtlasVariant> TileAtlas::loadSymmetricTile(SDL_Surface* surface,
                                                                      TileId id,
                                                                      const std::string& type) {
    if (nextSlot + SYMMETRIC_TEXTURE_SLOTS > MAX_TILES) {
        spdlog::error("Tile atlas is full (max tiles: {}), cannot load: {}", MAX_TILES, type);
        return {};
    }

    if (!uploadTileToAtlas(surface, nextSlot)) {
        return {};
    }

    glm::vec4 uv = calculateUV(nextSlot);
    tileUVs.push_back(uv);

    std::vector<TileAtlasVariant> result;
    result.push_back({nextSlot, 0});

    spdlog::debug("Loaded tile '{}' (id={}) with single slot [{}]", type, id, nextSlot);
    nextSlot++;
    return result;
}

std::vector<TileAtlas::TileAtlasVariant> TileAtlas::loadRotatableTile(SDL_Surface* surface,
                                                                      TileId id,
                                                                      const std::string& type) {
    if (nextSlot + ROTATABLE_TEXTURE_SLOTS > MAX_TILES) {
        spdlog::error("Tile atlas is full (max tiles: {}), cannot load: {}", MAX_TILES, type);
        return {};
    }

    std::vector<TileAtlasVariant> result;

    if (!uploadTileToAtlas(surface, nextSlot)) {
        return {};
    }

    result.push_back({nextSlot, 0});
    tileUVs.push_back(calculateUV(nextSlot));
    nextSlot++;

    SDL_Surface* rotated = surface;
    for (uint8_t orientation = 1; orientation <= 3; orientation++) {
        SDL_Surface* newRotated = rotateSurface90CCW(rotated);
        if (!newRotated) {
            spdlog::error("Failed to rotate tile '{}' for orientation {}", type, orientation);
            return {};
        }

        if (rotated != surface) {
            SDL_DestroySurface(rotated);
        }
        rotated = newRotated;

        if (!uploadTileToAtlas(rotated, nextSlot)) {
            SDL_DestroySurface(rotated);
            return {};
        }
        result.push_back({nextSlot, orientation});
        tileUVs.push_back(calculateUV(nextSlot));
        nextSlot++;
    }

    if (rotated != surface) {
        SDL_DestroySurface(rotated);
    }

    spdlog::debug("Loaded tile '{}' (id={}) with rotation slots [{}, {}, {}, {}]", type, id,
                  result[0].slot, result[1].slot, result[2].slot, result[3].slot);

    return result;
}

glm::vec4 TileAtlas::getTileUV(const GridTile& tile) const {
    auto it = variants.find({tile.id, tile.type});

    if (it == variants.end()) {
        spdlog::warn(
            "TileAtlas::getTileUV: TileId {} with type '{}' not found, returning empty UVs",
            tile.id, tile.type);
        return glm::vec4(0.0f);
    }

    const auto& tileVariants = it->second;
    size_t index = tile.orientation % tileVariants.size();

    return tileUVs[tileVariants[index].slot];
}

SDL_GPUTexture* TileAtlas::getTexture() const { return atlasTexture; }

SDL_GPUSampler* TileAtlas::getSampler() const { return sampler; }

uint32_t TileAtlas::getTileCount() const { return nextSlot; }

void TileAtlas::shutdown() {
    if (sampler) {
        SDL_ReleaseGPUSampler(device, sampler);
        sampler = nullptr;
    }
    if (atlasTexture) {
        SDL_ReleaseGPUTexture(device, atlasTexture);
        atlasTexture = nullptr;
    }
    tileUVs.clear();
    tileUVs.push_back(glm::vec4(0.0f));  // Reserve slot 0
    variants.clear();
    nextSlot = 1;
}

bool TileAtlas::uploadTileToAtlas(SDL_Surface* surface, uint32_t slot) {
    uint32_t dataSize = TILE_SIZE * TILE_SIZE * 4;

    // Scale surface to TILE_SIZE if needed
    SDL_Surface* scaledSurface = nullptr;
    SDL_Surface* srcSurface = surface;

    if (surface->w != TILE_SIZE || surface->h != TILE_SIZE) {
        scaledSurface = SDL_CreateSurface(TILE_SIZE, TILE_SIZE, SDL_PIXELFORMAT_RGBA32);

        if (!scaledSurface) {
            spdlog::error("Failed to create scaled surface: {}", SDL_GetError());
            return false;
        }

        SDL_Rect srcRect = {0, 0, surface->w, surface->h};
        SDL_Rect dstRect = {0, 0, TILE_SIZE, TILE_SIZE};

        if (!SDL_BlitSurfaceScaled(surface, &srcRect, scaledSurface, &dstRect,
                                   SDL_SCALEMODE_NEAREST)) {
            spdlog::error("Failed to scale surface: {}", SDL_GetError());
            SDL_DestroySurface(scaledSurface);
            return false;
        }

        srcSurface = scaledSurface;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        spdlog::error("Failed to create transfer buffer: {}", SDL_GetError());
        if (scaledSurface) SDL_DestroySurface(scaledSurface);
        return false;
    }

    void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    uint32_t expectedPitch = TILE_SIZE * 4;
    if (static_cast<uint32_t>(srcSurface->pitch) == expectedPitch) {
        SDL_memcpy(data, srcSurface->pixels, dataSize);
    } else {
        // Copy row by row to handle different pitch/stride
        uint8_t* dst = static_cast<uint8_t*>(data);
        uint8_t* src = static_cast<uint8_t*>(srcSurface->pixels);
        for (uint32_t row = 0; row < TILE_SIZE; row++) {
            SDL_memcpy(dst + row * expectedPitch, src + row * srcSurface->pitch, expectedPitch);
        }
    }
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    uint32_t atlasX = (slot % TILES_PER_ROW) * TILE_SIZE;
    uint32_t atlasY = (slot / TILES_PER_ROW) * TILE_SIZE;

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTextureTransferInfo transferSource = {};
    transferSource.transfer_buffer = transferBuffer;
    transferSource.offset = 0;
    transferSource.pixels_per_row = TILE_SIZE;
    transferSource.rows_per_layer = TILE_SIZE;

    SDL_GPUTextureRegion textureRegion = {};
    textureRegion.texture = atlasTexture;
    textureRegion.x = atlasX;
    textureRegion.y = atlasY;
    textureRegion.z = 0;
    textureRegion.w = TILE_SIZE;
    textureRegion.h = TILE_SIZE;
    textureRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &transferSource, &textureRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    if (scaledSurface) {
        SDL_DestroySurface(scaledSurface);
    }

    return true;
}

glm::vec4 TileAtlas::calculateUV(uint32_t slot) const {
    float tileU = static_cast<float>(TILE_SIZE) / static_cast<float>(ATLAS_SIZE);
    float tileV = static_cast<float>(TILE_SIZE) / static_cast<float>(ATLAS_SIZE);

    uint32_t col = slot % TILES_PER_ROW;
    uint32_t row = slot / TILES_PER_ROW;

    float u_min = col * tileU;
    float v_min = row * tileV;
    float u_max = (col + 1) * tileU;
    float v_max = (row + 1) * tileV;

    return glm::vec4(u_min, v_min, u_max, v_max);
}

}  // namespace SpaceRogueLite
