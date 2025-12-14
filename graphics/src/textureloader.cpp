#include "textureloader.h"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <fstream>

using namespace SpaceRogueLite;

TextureLoader::TextureLoader(SDL_GPUDevice* device) : device(device) {}

TextureLoader::~TextureLoader() {
    for (auto& [path, texture] : textureCache) {
        if (texture.sampler) {
            SDL_ReleaseGPUSampler(device, texture.sampler);
        }
        if (texture.texture) {
            SDL_ReleaseGPUTexture(device, texture.texture);
        }
    }

    textureCache.clear();
}

void TextureLoader::loadTextureDefinitions(const std::string& jsonPath,
                                           const std::string& basePath) {
    this->basePath = basePath;

    std::ifstream f(jsonPath);
    if (!f.is_open()) {
        spdlog::error("Failed to open texture definitions file: {}", jsonPath);
        return;
    }

    nlohmann::json data = nlohmann::json::parse(f);

    for (const auto& tex : data["textures"]) {
        TextureDefinition def;
        def.id = tex["id"].get<int>();
        def.name = tex["name"].get<std::string>();
        def.path = tex["path"].get<std::string>();

        textureDefinitions[def.id] = def;
        nameToIdMapping[def.name] = def.id;
    }

    spdlog::info("Loaded {} texture definitions from {}", textureDefinitions.size(), jsonPath);
}

SDL_Surface* TextureLoader::loadSurfaceFromPath(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        spdlog::error("Failed to load image '{}': {}", path, SDL_GetError());
        return nullptr;
    }

    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!convertedSurface) {
        spdlog::error("Failed to convert surface to RGBA32: {}", SDL_GetError());
        return nullptr;
    }

    return convertedSurface;
}

Texture* TextureLoader::loadAndAssignTexture(const std::string& path, const std::string& name,
                                             int id) {
    SDL_Surface* convertedSurface = loadSurfaceFromPath(path);
    if (!convertedSurface) {
        return nullptr;
    }

    Texture texture;
    texture.width = static_cast<uint32_t>(convertedSurface->w);
    texture.height = static_cast<uint32_t>(convertedSurface->h);
    texture.id = id;
    texture.name = name;

    // Create GPU texture
    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = texture.width;
    textureInfo.height = texture.height;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    texture.texture = SDL_CreateGPUTexture(device, &textureInfo);
    if (!texture.texture) {
        spdlog::error("Failed to create GPU texture: {}", SDL_GetError());
        SDL_DestroySurface(convertedSurface);
        return nullptr;
    }

    // Upload texture data
    uint32_t textureDataSize = texture.width * texture.height * 4;

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = textureDataSize;

    SDL_GPUTransferBuffer* textureTransfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!textureTransfer) {
        spdlog::error("Failed to create texture transfer buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture.texture);
        SDL_DestroySurface(convertedSurface);
        return nullptr;
    }

    // Copy data to transfer buffer, handling surface pitch correctly
    void* textureData = SDL_MapGPUTransferBuffer(device, textureTransfer, false);
    uint32_t expectedPitch = texture.width * 4;
    if (static_cast<uint32_t>(convertedSurface->pitch) == expectedPitch) {
        SDL_memcpy(textureData, convertedSurface->pixels, textureDataSize);
    } else {
        // Copy row by row to handle different pitch/stride
        uint8_t* dst = static_cast<uint8_t*>(textureData);
        uint8_t* src = static_cast<uint8_t*>(convertedSurface->pixels);
        for (uint32_t row = 0; row < texture.height; row++) {
            SDL_memcpy(dst + row * expectedPitch, src + row * convertedSurface->pitch, expectedPitch);
        }
    }
    SDL_UnmapGPUTransferBuffer(device, textureTransfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTextureTransferInfo transferSource = {};
    transferSource.transfer_buffer = textureTransfer;
    transferSource.offset = 0;
    transferSource.pixels_per_row = static_cast<Uint32>(convertedSurface->w);
    transferSource.rows_per_layer = static_cast<Uint32>(convertedSurface->h);

    SDL_GPUTextureRegion textureRegion = {};
    textureRegion.texture = texture.texture;
    textureRegion.x = 0;
    textureRegion.y = 0;
    textureRegion.z = 0;
    textureRegion.w = texture.width;
    textureRegion.h = texture.height;
    textureRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &transferSource, &textureRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, textureTransfer);

    SDL_DestroySurface(convertedSurface);

    // Create sampler
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    texture.sampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!texture.sampler) {
        spdlog::error("Failed to create sampler: {}", SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture.texture);
        return nullptr;
    }

    auto [it, inserted] = textureCache.emplace(name, texture);
    spdlog::debug("Loaded texture '{}' ({}x{}) at path '{}'", name, texture.width, texture.height,
                  path);
    return &it->second;
}

Texture* TextureLoader::getTexture(const std::string& name) {
    auto cached = textureCache.find(name);

    if (cached != textureCache.end()) {
        return &cached->second;
    }

    auto idMapping = nameToIdMapping.find(name);
    if (idMapping != nameToIdMapping.end()) {
        return getTextureById(idMapping->second);
    }

    spdlog::warn("Texture '{}' not found in cache or definitions", name);
    return nullptr;
}

Texture* TextureLoader::getTextureById(int id) {
    auto definition = textureDefinitions.find(id);
    if (definition == textureDefinitions.end()) {
        spdlog::warn("Texture definition with id {} not found", id);
        return nullptr;
    }

    const auto& def = definition->second;

    auto cached = textureCache.find(def.name);
    if (cached != textureCache.end()) {
        return &cached->second;
    }

    std::string fullPath = basePath + "/" + def.path;
    return loadAndAssignTexture(fullPath, def.name, def.id);
}

SDL_Surface* TextureLoader::loadSurfaceById(int textureId) {
    auto definition = textureDefinitions.find(textureId);
    if (definition == textureDefinitions.end()) {
        spdlog::warn("Texture definition with id {} not found", textureId);
        return nullptr;
    }

    const auto& def = definition->second;
    std::string fullPath = basePath + "/" + def.path;

    return loadSurfaceFromPath(fullPath);
}