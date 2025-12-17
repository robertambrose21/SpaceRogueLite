#include "renderlayers/tiles/tilerenderer.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include "shaders/textured_quad_shaders.h"
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

    spdlog::debug("TileRenderer initialized");
    return true;
}

void TileRenderer::shutdown() {
    SDL_WaitForGPUIdle(device);

    if (displayVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, displayVertexBuffer);
        displayVertexBuffer = nullptr;
    }
    if (bakedSampler) {
        SDL_ReleaseGPUSampler(device, bakedSampler);
        bakedSampler = nullptr;
    }
    if (bakedTexture) {
        SDL_ReleaseGPUTexture(device, bakedTexture);
        bakedTexture = nullptr;
    }
    if (instanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, instanceTransfer);
        instanceTransfer = nullptr;
    }
    if (instanceBuffer) {
        SDL_ReleaseGPUBuffer(device, instanceBuffer);
        instanceBuffer = nullptr;
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

    auto grid = entt::locator<Grid>::value();

    if (!atlas || grid.getWidth() == 0 || grid.getHeight() == 0) {
        return;
    }

    uint32_t requiredWidth = grid.getWidth() * TILE_SIZE;
    uint32_t requiredHeight = grid.getHeight() * TILE_SIZE;

    if (bakedWidth != requiredWidth || bakedHeight != requiredHeight) {
        if (!createRenderTarget(requiredWidth, requiredHeight)) {
            return;
        }
        cacheValid = false;
    }

    if (!cacheValid || grid.isDirty()) {
        rebakeTiles(commandBuffer);
        grid.clearDirty();
        cacheValid = true;
    }
}

void TileRenderer::render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                          const Camera& camera) {
    if (!bakedTexture || bakedWidth == 0 || bakedHeight == 0) {
        return;
    }

    renderBakedTexture(commandBuffer, renderPass, camera);
}

void TileRenderer::invalidateCache() { cacheValid = false; }

bool TileRenderer::createShaders() {
    // Compose vertex shader
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

    // Display vertex shader
    SDL_GPUShaderCreateInfo displayVertInfo = {};
    displayVertInfo.code = textured_quad_spirv_vertex;
    displayVertInfo.code_size = sizeof(textured_quad_spirv_vertex);
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

    // Display fragment shader
    SDL_GPUShaderCreateInfo displayFragInfo = {};
    displayFragInfo.code = textured_quad_spirv_fragment;
    displayFragInfo.code_size = sizeof(textured_quad_spirv_fragment);
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
    // Vertex buffer descriptions
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
    // Simple vertex buffer for position + texcoord
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(QuadVertex);
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(QuadVertex, position);

    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = offsetof(QuadVertex, texCoord);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

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

bool TileRenderer::createRenderTarget(uint32_t width, uint32_t height) {
    // Release old resources
    if (displayVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, displayVertexBuffer);
        displayVertexBuffer = nullptr;
    }
    if (bakedSampler) {
        SDL_ReleaseGPUSampler(device, bakedSampler);
        bakedSampler = nullptr;
    }
    if (bakedTexture) {
        SDL_ReleaseGPUTexture(device, bakedTexture);
        bakedTexture = nullptr;
    }

    // Create render target texture
    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = width;
    textureInfo.height = height;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    bakedTexture = SDL_CreateGPUTexture(device, &textureInfo);
    if (!bakedTexture) {
        spdlog::error("Failed to create baked texture: {}", SDL_GetError());
        return false;
    }

    // Create sampler for baked texture
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    bakedSampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!bakedSampler) {
        spdlog::error("Failed to create baked sampler: {}", SDL_GetError());
        return false;
    }

    // Create display vertex buffer (full size quad)
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);

    QuadVertex displayVertices[6] = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{w, 0.0f}, {1.0f, 0.0f}}, {{0.0f, h}, {0.0f, 1.0f}},
        {{w, 0.0f}, {1.0f, 0.0f}},    {{w, h}, {1.0f, 1.0f}},    {{0.0f, h}, {0.0f, 1.0f}},
    };

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = sizeof(displayVertices);

    displayVertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!displayVertexBuffer) {
        spdlog::error("Failed to create display vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload display vertices
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(displayVertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    void* data = SDL_MapGPUTransferBuffer(device, transfer, false);
    SDL_memcpy(data, displayVertices, sizeof(displayVertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation src = {transfer, 0};
    SDL_GPUBufferRegion dst = {displayVertexBuffer, 0, sizeof(displayVertices)};
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    bakedWidth = width;
    bakedHeight = height;

    spdlog::debug("Created tile render target ({}x{})", width, height);
    return true;
}

bool TileRenderer::ensureInstanceBuffer(uint32_t tileCount) {
    if (tileCount <= instanceBufferCapacity) {
        return true;
    }

    // Release old buffers
    if (instanceTransfer) {
        SDL_ReleaseGPUTransferBuffer(device, instanceTransfer);
        instanceTransfer = nullptr;
    }
    if (instanceBuffer) {
        SDL_ReleaseGPUBuffer(device, instanceBuffer);
        instanceBuffer = nullptr;
    }

    uint32_t newCapacity = tileCount + 256;  // Add some slack
    uint32_t bufferSize = newCapacity * sizeof(TileInstance);

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = bufferSize;

    instanceBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!instanceBuffer) {
        spdlog::error("Failed to create instance buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;

    instanceTransfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!instanceTransfer) {
        spdlog::error("Failed to create instance transfer buffer: {}", SDL_GetError());
        return false;
    }

    instanceBufferCapacity = newCapacity;
    return true;
}

void TileRenderer::rebakeTiles(SDL_GPUCommandBuffer* commandBuffer) {
    if (!entt::locator<Grid>::has_value()) {
        spdlog::warn("No grid found for TileRenderer; skipping rebake");
        return;
    }

    auto grid = entt::locator<Grid>::value();

    int mapWidth = grid.getWidth();
    int mapHeight = grid.getHeight();
    uint32_t tileCount = mapWidth * mapHeight;

    if (!ensureInstanceBuffer(tileCount)) {
        return;
    }

    // Fill instance buffer
    TileInstance* instances =
        static_cast<TileInstance*>(SDL_MapGPUTransferBuffer(device, instanceTransfer, true));

    uint32_t instanceIdx = 0;
    grid.forEachTile([&](int x, int y, const GridTile& tile) {
        if (tile.id != TILE_EMPTY) {
            instances[instanceIdx].position = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);
            instances[instanceIdx].uvBounds = atlas->getTileUV(tile);
            instanceIdx++;
        }
    });

    SDL_UnmapGPUTransferBuffer(device, instanceTransfer);

    uint32_t visibleTileCount = instanceIdx;

    if (visibleTileCount == 0) {
        return;
    }

    // Upload instance data
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    SDL_GPUTransferBufferLocation src = {instanceTransfer, 0};
    SDL_GPUBufferRegion dst = {instanceBuffer, 0,
                               static_cast<Uint32>(visibleTileCount * sizeof(TileInstance))};
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);

    // Render to texture
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = bakedTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(renderPass, composePipeline);

    // Set viewport for render target
    SDL_GPUViewport viewport = {
        0, 0, static_cast<float>(bakedWidth), static_cast<float>(bakedHeight), 0.0f, 1.0f};
    SDL_SetGPUViewport(renderPass, &viewport);

    SDL_Rect scissor = {0, 0, static_cast<int>(bakedWidth), static_cast<int>(bakedHeight)};
    SDL_SetGPUScissor(renderPass, &scissor);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(bakedWidth),
                                      static_cast<float>(bakedHeight), 0.0f, -1.0f, 1.0f);
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &projection, sizeof(glm::mat4));

    SDL_GPUBufferBinding vertexBindings[2] = {};
    vertexBindings[0].buffer = quadVertexBuffer;
    vertexBindings[0].offset = 0;
    vertexBindings[1].buffer = instanceBuffer;
    vertexBindings[1].offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBindings, 2);

    SDL_GPUTextureSamplerBinding texBinding = {};
    texBinding.texture = atlas->getTexture();
    texBinding.sampler = atlas->getSampler();
    SDL_BindGPUFragmentSamplers(renderPass, 0, &texBinding, 1);

    SDL_DrawGPUPrimitives(renderPass, 6, visibleTileCount, 0, 0);

    SDL_EndGPURenderPass(renderPass);
}

void TileRenderer::renderBakedTexture(SDL_GPUCommandBuffer* commandBuffer,
                                      SDL_GPURenderPass* renderPass, const Camera& camera) {
    SDL_BindGPUGraphicsPipeline(renderPass, displayPipeline);

    glm::mat4 mvp = camera.getViewProjectionMatrix();
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

    ViewportData vp = camera.getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera.getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = displayVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    SDL_GPUTextureSamplerBinding texBinding = {};
    texBinding.texture = bakedTexture;
    texBinding.sampler = bakedSampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &texBinding, 1);

    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
}

}  // namespace SpaceRogueLite
