#include "renderlayers/tiles/tilerenderer.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include "shaders/chunk_display_shaders.h"
#include "shaders/tilecompose_shaders.h"

using json = nlohmann::json;

namespace SpaceRogueLite {

TileRenderer::TileRenderer() : RenderLayer("TileRenderer") {}

TileRenderer::~TileRenderer() { shutdown(); }

bool TileRenderer::initialize() {
    atlas = std::make_unique<TileAtlas>(device, textureLoader);
    if (!atlas->initialize()) {
        spdlog::error("Failed to initialize tile atlas");
        return false;
    }

    if (!createShaders()) {
        return false;
    }
    if (!createComposePipeline()) {
        return false;
    }
    if (!createDisplayPipeline()) {
        return false;
    }
    if (!createQuadVertexBuffer()) {
        return false;
    }
    if (!createChunkSampler()) {
        return false;
    }

    spdlog::debug("TileRenderer initialized");
    return true;
}

void TileRenderer::shutdown() {
    SDL_WaitForGPUIdle(device);

    destroyAllChunks();

    if (chunkSampler) {
        SDL_ReleaseGPUSampler(device, chunkSampler);
        chunkSampler = nullptr;
    }
    if (chunkTextureArray) {
        SDL_ReleaseGPUTexture(device, chunkTextureArray);
        chunkTextureArray = nullptr;
    }
    if (chunkInstanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, chunkInstanceTransfer);
        chunkInstanceTransfer = nullptr;
    }
    if (chunkInstanceBuffer) {
        SDL_ReleaseGPUBuffer(device, chunkInstanceBuffer);
        chunkInstanceBuffer = nullptr;
    }
    if (tileInstanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, tileInstanceTransfer);
        tileInstanceTransfer = nullptr;
    }
    if (tileInstanceBuffer) {
        SDL_ReleaseGPUBuffer(device, tileInstanceBuffer);
        tileInstanceBuffer = nullptr;
    }
    if (quadVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, quadVertexBuffer);
        quadVertexBuffer = nullptr;
    }
    if (displayPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, displayPipeline);
        displayPipeline = nullptr;
    }
    if (composePipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, composePipeline);
        composePipeline = nullptr;
    }
    if (displayFragmentShader) {
        SDL_ReleaseGPUShader(device, displayFragmentShader);
        displayFragmentShader = nullptr;
    }
    if (displayVertexShader) {
        SDL_ReleaseGPUShader(device, displayVertexShader);
        displayVertexShader = nullptr;
    }
    if (composeFragmentShader) {
        SDL_ReleaseGPUShader(device, composeFragmentShader);
        composeFragmentShader = nullptr;
    }
    if (composeVertexShader) {
        SDL_ReleaseGPUShader(device, composeVertexShader);
        composeVertexShader = nullptr;
    }

    atlas.reset();
}

void TileRenderer::loadTileVariantsIntoAtlas(const std::set<TileVariant>& variants) {
    uint32_t loadedCount = 0;

    for (const auto& variant : variants) {
        if (atlas->loadTileVariant(variant)) {
            loadedCount++;
        }
    }

    invalidateCache();

    spdlog::info("Loaded {}/{} tile variants into atlas", loadedCount, variants.size());
}

void TileRenderer::prepareFrame(SDL_GPUCommandBuffer* commandBuffer) {
    if (!entt::locator<Grid>::has_value()) {
        return;
    }

    auto& grid = entt::locator<Grid>::value();

    if (!atlas || grid.getWidth() == 0 || grid.getHeight() == 0) {
        return;
    }

    updateChunkGrid();

    if (!cacheValid) {
        for (auto& [coord, chunk] : chunks) {
            chunk.isDirty = true;
        }

        cacheValid = true;
    } else if (grid.isDirty()) {
        markDirtyChunksFromRegion(grid.getDirtyRegion());
    }

    grid.clearDirty();

    rebakeDirtyChunks(commandBuffer);
    uploadChunkInstances(commandBuffer);
}

void TileRenderer::render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                          const Camera& camera) {
    renderVisibleChunks(commandBuffer, renderPass, camera);
}

void TileRenderer::invalidateCache() {
    cacheValid = false;

    // Also invalidate visibility cache and force re-upload
    cachedCameraPos = glm::vec2(-1.0f, -1.0f);
    cachedVisibleChunks.clear();
    chunkInstancesNeedUpload = true;
}

bool TileRenderer::createShaders() {
    // Compose vertex shader (tiles to chunk texture)
    SDL_GPUShaderCreateInfo vertexInfo = {};
    vertexInfo.code = tilecompose_spirv_vertex;
    vertexInfo.code_size = sizeof(tilecompose_spirv_vertex);
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_uniform_buffers = 1;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_samplers = 0;

    composeVertexShader = SDL_CreateGPUShader(device, &vertexInfo);
    if (!composeVertexShader) {
        spdlog::error("Failed to create compose vertex shader: {}", SDL_GetError());
        return false;
    }

    // Compose fragment shader
    SDL_GPUShaderCreateInfo fragInfo = {};
    fragInfo.code = tilecompose_spirv_fragment;
    fragInfo.code_size = sizeof(tilecompose_spirv_fragment);
    fragInfo.entrypoint = "main";
    fragInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragInfo.num_uniform_buffers = 0;
    fragInfo.num_storage_buffers = 0;
    fragInfo.num_storage_textures = 0;
    fragInfo.num_samplers = 1;

    composeFragmentShader = SDL_CreateGPUShader(device, &fragInfo);
    if (!composeFragmentShader) {
        spdlog::error("Failed to create compose fragment shader: {}", SDL_GetError());
        return false;
    }

    // Display vertex shader (chunks to screen, batched with texture array)
    SDL_GPUShaderCreateInfo displayVertInfo = {};
    displayVertInfo.code = chunk_display_spirv_vertex;
    displayVertInfo.code_size = sizeof(chunk_display_spirv_vertex);
    displayVertInfo.entrypoint = "main";
    displayVertInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    displayVertInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    displayVertInfo.num_uniform_buffers = 1;
    displayVertInfo.num_storage_buffers = 0;
    displayVertInfo.num_storage_textures = 0;
    displayVertInfo.num_samplers = 0;

    displayVertexShader = SDL_CreateGPUShader(device, &displayVertInfo);
    if (!displayVertexShader) {
        spdlog::error("Failed to create display vertex shader: {}", SDL_GetError());
        return false;
    }

    // Display fragment shader (samples from texture array)
    SDL_GPUShaderCreateInfo displayFragInfo = {};
    displayFragInfo.code = chunk_display_spirv_fragment;
    displayFragInfo.code_size = sizeof(chunk_display_spirv_fragment);
    displayFragInfo.entrypoint = "main";
    displayFragInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    displayFragInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    displayFragInfo.num_uniform_buffers = 0;
    displayFragInfo.num_storage_buffers = 0;
    displayFragInfo.num_storage_textures = 0;
    displayFragInfo.num_samplers = 1;

    displayFragmentShader = SDL_CreateGPUShader(device, &displayFragInfo);
    if (!displayFragmentShader) {
        spdlog::error("Failed to create display fragment shader: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool TileRenderer::createComposePipeline() {
    SDL_GPUVertexBufferDescription vertexBufferDescs[2] = {};

    // Slot 0: Per-vertex quad data
    vertexBufferDescs[0].slot = 0;
    vertexBufferDescs[0].pitch = sizeof(QuadVertex);
    vertexBufferDescs[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescs[0].instance_step_rate = 0;

    // Slot 1: Per-instance tile data
    vertexBufferDescs[1].slot = 1;
    vertexBufferDescs[1].pitch = sizeof(TileInstance);
    vertexBufferDescs[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    vertexBufferDescs[1].instance_step_rate = 0;  // Must be 0 for SDL3 GPU

    // Vertex attributes
    SDL_GPUVertexAttribute vertexAttributes[4] = {};

    // Per-vertex: localPos (location 0)
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(QuadVertex, position);

    // Per-vertex: localUV (location 1)
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = offsetof(QuadVertex, texCoord);

    // Per-instance: tilePos (location 2)
    vertexAttributes[2].location = 2;
    vertexAttributes[2].buffer_slot = 1;
    vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[2].offset = offsetof(TileInstance, position);

    // Per-instance: tileUV (location 3)
    vertexAttributes[3].location = 3;
    vertexAttributes[3].buffer_slot = 1;
    vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[3].offset = offsetof(TileInstance, uvBounds);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = vertexBufferDescs;
    vertexInputState.num_vertex_buffers = 2;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 4;

    // Color target for render-to-texture
    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    colorTargetDesc.blend_state.enable_blend = true;
    colorTargetDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorTargetDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.color_write_mask = 0xF;

    SDL_GPURasterizerState rasterizerState = {};
    rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = composeVertexShader;
    pipelineInfo.fragment_shader = composeFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    composePipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!composePipeline) {
        spdlog::error("Failed to create compose pipeline: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool TileRenderer::createDisplayPipeline() {
    SDL_GPUVertexBufferDescription vertexBufferDescs[2] = {};

    // Slot 0: Per-vertex quad data (unit quad)
    vertexBufferDescs[0].slot = 0;
    vertexBufferDescs[0].pitch = sizeof(QuadVertex);
    vertexBufferDescs[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescs[0].instance_step_rate = 0;

    // Slot 1: Per-instance chunk data
    vertexBufferDescs[1].slot = 1;
    vertexBufferDescs[1].pitch = sizeof(ChunkInstance);
    vertexBufferDescs[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    vertexBufferDescs[1].instance_step_rate = 0;

    // Vertex attributes
    SDL_GPUVertexAttribute vertexAttributes[5] = {};

    // Per-vertex: localPos (location 0)
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(QuadVertex, position);

    // Per-vertex: localUV (location 1)
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = offsetof(QuadVertex, texCoord);

    // Per-instance: chunkWorldPos (location 2)
    vertexAttributes[2].location = 2;
    vertexAttributes[2].buffer_slot = 1;
    vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[2].offset = offsetof(ChunkInstance, worldPos);

    // Per-instance: chunkSize (location 3)
    vertexAttributes[3].location = 3;
    vertexAttributes[3].buffer_slot = 1;
    vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[3].offset = offsetof(ChunkInstance, size);

    // Per-instance: layerIndex (location 4)
    vertexAttributes[4].location = 4;
    vertexAttributes[4].buffer_slot = 1;
    vertexAttributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    vertexAttributes[4].offset = offsetof(ChunkInstance, layerIndex);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = vertexBufferDescs;
    vertexInputState.num_vertex_buffers = 2;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 5;

    // Use swapchain format for display
    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    colorTargetDesc.blend_state.enable_blend = true;
    colorTargetDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorTargetDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.color_write_mask = 0xF;

    SDL_GPURasterizerState rasterizerState = {};
    rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = displayVertexShader;
    pipelineInfo.fragment_shader = displayFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    displayPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!displayPipeline) {
        spdlog::error("Failed to create display pipeline: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool TileRenderer::createQuadVertexBuffer() {
    QuadVertex vertices[6] = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}},  // Top-left
        {{1.0f, 0.0f}, {1.0f, 0.0f}},  // Top-right
        {{0.0f, 1.0f}, {0.0f, 1.0f}},  // Bottom-left
        {{1.0f, 0.0f}, {1.0f, 0.0f}},  // Top-right
        {{1.0f, 1.0f}, {1.0f, 1.0f}},  // Bottom-right
        {{0.0f, 1.0f}, {0.0f, 1.0f}},  // Bottom-left
    };

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = sizeof(vertices);

    quadVertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!quadVertexBuffer) {
        spdlog::error("Failed to create quad vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    void* data = SDL_MapGPUTransferBuffer(device, transfer, false);
    SDL_memcpy(data, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation src = {transfer, 0};
    SDL_GPUBufferRegion dst = {quadVertexBuffer, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return true;
}

bool TileRenderer::createChunkSampler() {
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    chunkSampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!chunkSampler) {
        spdlog::error("Failed to create chunk sampler: {}", SDL_GetError());
        return false;
    }
    return true;
}

bool TileRenderer::ensureChunkTextureArray(uint32_t requiredLayers) {
    if (requiredLayers <= chunkTextureArrayLayers && chunkTextureArray) {
        return true;
    }

    // Add some slack to avoid frequent reallocations
    uint32_t newLayerCount = requiredLayers + 16;

    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = CHUNK_SIZE_PIXELS;
    textureInfo.height = CHUNK_SIZE_PIXELS;
    textureInfo.layer_count_or_depth = newLayerCount;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* newTextureArray = SDL_CreateGPUTexture(device, &textureInfo);
    if (!newTextureArray) {
        spdlog::error("Failed to create chunk texture array: {}", SDL_GetError());
        return false;
    }

    // If we had an old array, we'd need to copy data - but for simplicity, we'll just
    // mark all chunks as dirty to rebake them
    if (chunkTextureArray) {
        SDL_ReleaseGPUTexture(device, chunkTextureArray);
        for (auto& [coord, chunk] : chunks) {
            chunk.isDirty = true;
        }
    }

    chunkTextureArray = newTextureArray;
    chunkTextureArrayLayers = newLayerCount;

    spdlog::trace("Created chunk texture array with {} layers", newLayerCount);
    return true;
}

uint32_t TileRenderer::allocateLayerIndex() {
    if (!freeLayerIndices.empty()) {
        uint32_t index = freeLayerIndices.back();
        freeLayerIndices.pop_back();
        return index;
    }

    return nextLayerIndex++;
}

void TileRenderer::freeLayerIndex(uint32_t index) { freeLayerIndices.push_back(index); }

void TileRenderer::updateChunkGrid() {
    if (!entt::locator<Grid>::has_value()) {
        return;
    }

    auto& grid = entt::locator<Grid>::value();
    glm::ivec2 gridSize(grid.getWidth(), grid.getHeight());

    if (gridSize == cachedGridSize) {
        return;
    }

    glm::ivec2 newChunkGridSize = (gridSize + CHUNK_SIZE_TILES - 1) / CHUNK_SIZE_TILES;

    for (auto it = chunks.begin(); it != chunks.end();) {
        if (it->first.x >= newChunkGridSize.x || it->first.y >= newChunkGridSize.y) {
            destroyChunk(it->second);
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }

    cachedGridSize = gridSize;
    chunkGridSize = newChunkGridSize;

    createAllChunks();

    cachedCameraPos = glm::vec2(-1.0f, -1.0f);
    cachedVisibleChunks.clear();
}

void TileRenderer::createAllChunks() {
    uint32_t totalChunks = chunkGridSize.x * chunkGridSize.y;
    if (totalChunks == 0) {
        return;
    }

    if (!ensureChunkTextureArray(totalChunks)) {
        return;
    }

    for (int cy = 0; cy < chunkGridSize.y; ++cy) {
        for (int cx = 0; cx < chunkGridSize.x; ++cx) {
            glm::ivec2 coord(cx, cy);
            if (chunks.find(coord) == chunks.end()) {
                getOrCreateChunk(coord);
                chunkInstancesNeedUpload = true;
            }
        }
    }
}

TileRenderer::TileChunk& TileRenderer::getOrCreateChunk(glm::ivec2 chunkPos) {
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        return it->second;
    }

    TileChunk chunk;
    chunk.chunkPos = chunkPos;
    chunk.tileCount = calculateChunkTileCount(chunkPos);
    chunk.pixelSize = glm::uvec2(chunk.tileCount * TILE_SIZE);
    chunk.worldMin = glm::vec2(chunkPos * CHUNK_SIZE_PIXELS);
    chunk.worldMax = chunk.worldMin + glm::vec2(chunk.pixelSize);
    chunk.layerIndex = allocateLayerIndex();
    chunk.isDirty = true;
    chunk.isVisible = true;

    ensureChunkTextureArray(chunk.layerIndex + 1);

    auto [insertIt, _] = chunks.emplace(chunkPos, std::move(chunk));
    return insertIt->second;
}

void TileRenderer::destroyChunk(const TileChunk& chunk) { freeLayerIndex(chunk.layerIndex); }

void TileRenderer::destroyAllChunks() {
    chunks.clear();
    freeLayerIndices.clear();
    nextLayerIndex = 0;
    cachedGridSize = glm::ivec2(0, 0);
    chunkGridSize = glm::ivec2(0, 0);
    cachedVisibleChunks.clear();
    uploadedChunkCount = 0;
    chunkInstancesNeedUpload = true;
}

glm::ivec2 TileRenderer::calculateChunkTileCount(glm::ivec2 chunkPos) const {
    glm::ivec2 startTile = chunkPos * CHUNK_SIZE_TILES;
    glm::ivec2 endTile = glm::min(startTile + CHUNK_SIZE_TILES, cachedGridSize);
    return endTile - startTile;
}

void TileRenderer::markDirtyChunksFromRegion(const GridRegion& region) {
    if (region.width <= 0 || region.height <= 0) {
        return;
    }

    glm::ivec2 startChunk = glm::ivec2(region.x, region.y) / CHUNK_SIZE_TILES;
    glm::ivec2 endChunk =
        (glm::ivec2(region.x + region.width, region.y + region.height) - 1) / CHUNK_SIZE_TILES;

    startChunk = glm::max(startChunk, glm::ivec2(0));
    endChunk = glm::min(endChunk, chunkGridSize - 1);

    for (int cy = startChunk.y; cy <= endChunk.y; ++cy) {
        for (int cx = startChunk.x; cx <= endChunk.x; ++cx) {
            glm::ivec2 coord(cx, cy);
            auto it = chunks.find(coord);

            if (it != chunks.end()) {
                it->second.isDirty = true;
            }
        }
    }
}

void TileRenderer::updateVisibleChunks(const Camera& camera) {
    glm::vec2 pos = camera.getPosition();
    glm::vec2 size(static_cast<float>(camera.getScreenWidth()),
                   static_cast<float>(camera.getScreenHeight()));

    if (pos == cachedCameraPos && size == cachedCameraSize && !cachedVisibleChunks.empty()) {
        return;
    }

    cachedCameraPos = pos;
    cachedCameraSize = size;
    cachedVisibleChunks.clear();

    if (chunkGridSize.x <= 0 || chunkGridSize.y <= 0) {
        return;
    }

    glm::ivec2 startChunk = glm::max(glm::ivec2(0), glm::ivec2(pos) / CHUNK_SIZE_PIXELS);
    glm::ivec2 endChunk = glm::min(chunkGridSize - 1, glm::ivec2(pos + size) / CHUNK_SIZE_PIXELS);

    for (int cy = startChunk.y; cy <= endChunk.y; ++cy) {
        for (int cx = startChunk.x; cx <= endChunk.x; ++cx) {
            auto it = chunks.find(glm::ivec2(cx, cy));

            if (it != chunks.end() && it->second.isVisible) {
                cachedVisibleChunks.push_back(&it->second);
            }
        }
    }
}

bool TileRenderer::ensureTileInstanceBuffer(uint32_t tileCount) {
    if (tileCount <= tileInstanceBufferCapacity) {
        return true;
    }

    if (tileInstanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, tileInstanceTransfer);
        tileInstanceTransfer = nullptr;
    }
    if (tileInstanceBuffer) {
        SDL_ReleaseGPUBuffer(device, tileInstanceBuffer);
        tileInstanceBuffer = nullptr;
    }

    uint32_t newCapacity = tileCount + 256;  // Add some slack
    uint32_t bufferSize = newCapacity * sizeof(TileInstance);

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = bufferSize;

    tileInstanceBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!tileInstanceBuffer) {
        spdlog::error("Failed to create tile instance buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;

    tileInstanceTransfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!tileInstanceTransfer) {
        spdlog::error("Failed to create tile instance transfer buffer: {}", SDL_GetError());
        return false;
    }

    tileInstanceBufferCapacity = newCapacity;
    return true;
}

bool TileRenderer::ensureChunkInstanceBuffer(uint32_t chunkCount) {
    if (chunkCount <= chunkInstanceBufferCapacity) {
        return true;
    }

    if (chunkInstanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, chunkInstanceTransfer);
        chunkInstanceTransfer = nullptr;
    }
    if (chunkInstanceBuffer) {
        SDL_ReleaseGPUBuffer(device, chunkInstanceBuffer);
        chunkInstanceBuffer = nullptr;
    }

    uint32_t newCapacity = chunkCount + 16;  // Add some slack
    uint32_t bufferSize = newCapacity * sizeof(ChunkInstance);

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = bufferSize;

    chunkInstanceBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!chunkInstanceBuffer) {
        spdlog::error("Failed to create chunk instance buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;

    chunkInstanceTransfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!chunkInstanceTransfer) {
        spdlog::error("Failed to create chunk instance transfer buffer: {}", SDL_GetError());
        return false;
    }

    chunkInstanceBufferCapacity = newCapacity;
    return true;
}

void TileRenderer::rebakeChunk(SDL_GPUCommandBuffer* commandBuffer, TileChunk& chunk) {
    if (!entt::locator<Grid>::has_value()) {
        return;
    }

    const auto& grid = entt::locator<Grid>::value();

    glm::ivec2 startTile = chunk.chunkPos * CHUNK_SIZE_TILES;
    glm::ivec2 endTile = startTile + chunk.tileCount;

    uint32_t maxTiles = chunk.tileCount.x * chunk.tileCount.y;
    if (!ensureTileInstanceBuffer(maxTiles)) {
        return;
    }

    TileInstance* instances =
        static_cast<TileInstance*>(SDL_MapGPUTransferBuffer(device, tileInstanceTransfer, true));

    uint32_t instanceIdx = 0;
    for (int y = startTile.y; y < endTile.y; ++y) {
        for (int x = startTile.x; x < endTile.x; ++x) {
            GridTile tile = grid.getTile(x, y);

            if (tile.id != TILE_EMPTY) {
                int localX = x - startTile.x;
                int localY = y - startTile.y;
                instances[instanceIdx].position = glm::vec2(localX * TILE_SIZE, localY * TILE_SIZE);
                instances[instanceIdx].uvBounds = atlas->getTileUV(tile);
                instanceIdx++;
            }
        }
    }

    SDL_UnmapGPUTransferBuffer(device, tileInstanceTransfer);

    uint32_t visibleTileCount = instanceIdx;
    chunk.isVisible = (visibleTileCount > 0);

    if (visibleTileCount == 0) {
        chunk.isDirty = false;
        return;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    SDL_GPUTransferBufferLocation src = {tileInstanceTransfer, 0};
    SDL_GPUBufferRegion dst = {tileInstanceBuffer, 0,
                               static_cast<Uint32>(visibleTileCount * sizeof(TileInstance))};
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = chunkTextureArray;
    colorTarget.layer_or_depth_plane = chunk.layerIndex;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(renderPass, composePipeline);

    SDL_GPUViewport viewport = {
        0,    0,   static_cast<float>(CHUNK_SIZE_PIXELS), static_cast<float>(CHUNK_SIZE_PIXELS),
        0.0f, 1.0f};
    SDL_SetGPUViewport(renderPass, &viewport);

    SDL_Rect scissor = {0, 0, CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS};
    SDL_SetGPUScissor(renderPass, &scissor);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(CHUNK_SIZE_PIXELS),
                                      static_cast<float>(CHUNK_SIZE_PIXELS), 0.0f, -1.0f, 1.0f);
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &projection, sizeof(glm::mat4));

    SDL_GPUBufferBinding vertexBindings[2] = {};
    vertexBindings[0].buffer = quadVertexBuffer;
    vertexBindings[0].offset = 0;
    vertexBindings[1].buffer = tileInstanceBuffer;
    vertexBindings[1].offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBindings, 2);

    SDL_GPUTextureSamplerBinding texBinding = {};
    texBinding.texture = atlas->getTexture();
    texBinding.sampler = atlas->getSampler();
    SDL_BindGPUFragmentSamplers(renderPass, 0, &texBinding, 1);

    SDL_DrawGPUPrimitives(renderPass, 6, visibleTileCount, 0, 0);

    SDL_EndGPURenderPass(renderPass);

    chunk.isDirty = false;
}

void TileRenderer::rebakeDirtyChunks(SDL_GPUCommandBuffer* commandBuffer) {
    for (auto& [coord, chunk] : chunks) {
        if (chunk.isDirty) {
            rebakeChunk(commandBuffer, chunk);
            chunkInstancesNeedUpload = true;
        }
    }
}

void TileRenderer::uploadChunkInstances(SDL_GPUCommandBuffer* commandBuffer) {
    if (!chunkInstancesNeedUpload) {
        return;
    }

    if (chunks.empty() || !chunkTextureArray) {
        uploadedChunkCount = 0;
        chunkInstancesNeedUpload = false;
        return;
    }

    // Determine visible chunk range based on cached camera position
    // If no cached position yet, upload all non-empty chunks for first frame
    glm::ivec2 startChunk(0, 0);
    glm::ivec2 endChunk = chunkGridSize - 1;

    if (cachedCameraPos.x >= 0 && cachedCameraSize.x > 0) {
        startChunk = glm::max(glm::ivec2(0), glm::ivec2(cachedCameraPos) / CHUNK_SIZE_PIXELS);
        endChunk = glm::min(chunkGridSize - 1,
                            glm::ivec2(cachedCameraPos + cachedCameraSize) / CHUNK_SIZE_PIXELS);
    }

    uint32_t visibleCount = 0;
    for (int cy = startChunk.y; cy <= endChunk.y; ++cy) {
        for (int cx = startChunk.x; cx <= endChunk.x; ++cx) {
            auto it = chunks.find(glm::ivec2(cx, cy));

            if (it != chunks.end() && it->second.isVisible) {
                visibleCount++;
            }
        }
    }

    if (visibleCount == 0) {
        uploadedChunkCount = 0;
        return;
    }

    if (!ensureChunkInstanceBuffer(visibleCount)) {
        uploadedChunkCount = 0;
        return;
    }

    ChunkInstance* instances =
        static_cast<ChunkInstance*>(SDL_MapGPUTransferBuffer(device, chunkInstanceTransfer, true));

    uint32_t idx = 0;
    for (int cy = startChunk.y; cy <= endChunk.y; ++cy) {
        for (int cx = startChunk.x; cx <= endChunk.x; ++cx) {
            auto it = chunks.find(glm::ivec2(cx, cy));

            if (it != chunks.end() && it->second.isVisible) {
                const TileChunk& chunk = it->second;
                instances[idx].worldPos = chunk.worldMin;
                instances[idx].size = glm::vec2(chunk.pixelSize);
                instances[idx].layerIndex = static_cast<float>(chunk.layerIndex);
                idx++;
            }
        }
    }

    SDL_UnmapGPUTransferBuffer(device, chunkInstanceTransfer);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    SDL_GPUTransferBufferLocation src = {chunkInstanceTransfer, 0};
    SDL_GPUBufferRegion dst = {chunkInstanceBuffer, 0,
                               static_cast<Uint32>(visibleCount * sizeof(ChunkInstance))};
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);

    uploadedChunkCount = visibleCount;
    chunkInstancesNeedUpload = false;
}

void TileRenderer::renderVisibleChunks(SDL_GPUCommandBuffer* commandBuffer,
                                       SDL_GPURenderPass* renderPass, const Camera& camera) {
    if (!chunkTextureArray || uploadedChunkCount == 0) {
        return;
    }

    glm::vec2 newCameraPos = camera.getPosition();
    glm::vec2 newCameraSize = glm::vec2(static_cast<float>(camera.getScreenWidth()),
                                        static_cast<float>(camera.getScreenHeight()));

    if (newCameraPos != cachedCameraPos || newCameraSize != cachedCameraSize) {
        chunkInstancesNeedUpload = true;
    }

    cachedCameraPos = newCameraPos;
    cachedCameraSize = newCameraSize;

    SDL_BindGPUGraphicsPipeline(renderPass, displayPipeline);

    glm::mat4 mvp = camera.getViewProjectionMatrix();
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

    ViewportData vp = camera.getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera.getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    SDL_GPUBufferBinding vertexBindings[2] = {};
    vertexBindings[0].buffer = quadVertexBuffer;
    vertexBindings[0].offset = 0;
    vertexBindings[1].buffer = chunkInstanceBuffer;
    vertexBindings[1].offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBindings, 2);

    SDL_GPUTextureSamplerBinding texBinding = {};
    texBinding.texture = chunkTextureArray;
    texBinding.sampler = chunkSampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &texBinding, 1);

    SDL_DrawGPUPrimitives(renderPass, 6, uploadedChunkCount, 0, 0);
}

}  // namespace SpaceRogueLite
