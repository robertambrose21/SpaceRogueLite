#include "tileatlas.h"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

namespace SpaceRogueLite {

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

TileId TileAtlas::loadTile(const std::string& path) {
    if (nextSlot >= MAX_TILES) {
        spdlog::error("Tile atlas is full (max tiles: {}), cannot load: {}", MAX_TILES, path);
        return TILE_EMPTY;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        spdlog::error("Failed to load tile image {}: {}", path, SDL_GetError());
        return TILE_EMPTY;
    }

    // TODO: Consider re-enabling at some point
    // if (surface->w != TILE_SIZE || surface->h != TILE_SIZE) {
    //     spdlog::error("Tile {} has wrong dimensions ({}x{}), expected {}x{}", path, surface->w,
    //                   surface->h, TILE_SIZE, TILE_SIZE);
    //     SDL_DestroySurface(surface);
    //     return TILE_EMPTY;
    // }

    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!convertedSurface) {
        spdlog::error("Failed to convert tile surface: {}", SDL_GetError());
        return TILE_EMPTY;
    }

    uint32_t slot = nextSlot;
    if (!uploadTileToAtlas(convertedSurface, slot)) {
        SDL_DestroySurface(convertedSurface);
        return TILE_EMPTY;
    }

    SDL_DestroySurface(convertedSurface);

    glm::vec4 uv = calculateUV(slot);
    tileUVs.push_back(uv);
    nextSlot++;

    spdlog::debug("Loaded tile {} as TileId {} (UV: {},{} - {},{})", path, slot, uv.x, uv.y, uv.z,
                  uv.w);

    return static_cast<TileId>(slot);
}

glm::vec4 TileAtlas::getTileUV(TileId id) const {
    if (id >= tileUVs.size()) {
        return glm::vec4(0.0f);
    }
    return tileUVs[id];
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
    nextSlot = 1;
}

bool TileAtlas::uploadTileToAtlas(SDL_Surface* surface, uint32_t slot) {
    uint32_t dataSize = TILE_SIZE * TILE_SIZE * 4;

    // Create transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        spdlog::error("Failed to create transfer buffer: {}", SDL_GetError());
        return false;
    }

    // Copy data to transfer buffer
    void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(data, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Calculate position in atlas
    uint32_t atlasX = (slot % TILES_PER_ROW) * TILE_SIZE;
    uint32_t atlasY = (slot / TILES_PER_ROW) * TILE_SIZE;

    // Upload to texture
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

    return true;
}

glm::vec4 TileAtlas::calculateUV(uint32_t slot) const {
    float tileU = static_cast<float>(TILE_SIZE) / static_cast<float>(ATLAS_SIZE);
    float tileV = static_cast<float>(TILE_SIZE) / static_cast<float>(ATLAS_SIZE);

    uint32_t col = slot % TILES_PER_ROW;
    uint32_t row = slot / TILES_PER_ROW;

    float u_min = col * tileU;
    float v_min = row * tileV;
    float u_max = u_min + tileU;
    float v_max = v_min + tileV;

    return glm::vec4(u_min, v_min, u_max, v_max);
}

}  // namespace SpaceRogueLite
