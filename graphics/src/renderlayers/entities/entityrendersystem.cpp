#include "renderlayers/entities/entityrendersystem.h"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <glm/gtc/matrix_transform.hpp>

#include "components.h"
#include "rendercomponents.h"
#include "shaders/colored_quad_shaders.h"
#include "shaders/textured_quad_shaders.h"

namespace SpaceRogueLite {

EntityRenderSystem::EntityRenderSystem(entt::registry& registry)
    : RenderLayer("EntityRenderSystem"), registry(registry) {}

EntityRenderSystem::~EntityRenderSystem() { shutdown(); }

bool EntityRenderSystem::initialize() {
    if (!createTexturedPipeline()) {
        return false;
    }
    if (!createUntexturedPipeline()) {
        return false;
    }
    if (!createVertexBuffers()) {
        return false;
    }

    spdlog::debug("EntityRenderSystem initialized");
    return true;
}

void EntityRenderSystem::shutdown() {
    SDL_WaitForGPUIdle(device);

    // Release untextured pipeline resources
    if (untexturedVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, untexturedVertexBuffer);
        untexturedVertexBuffer = nullptr;
    }
    if (untexturedPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, untexturedPipeline);
        untexturedPipeline = nullptr;
    }
    if (untexturedFragmentShader) {
        SDL_ReleaseGPUShader(device, untexturedFragmentShader);
        untexturedFragmentShader = nullptr;
    }
    if (untexturedVertexShader) {
        SDL_ReleaseGPUShader(device, untexturedVertexShader);
        untexturedVertexShader = nullptr;
    }

    // Release textured pipeline resources
    if (texturedVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, texturedVertexBuffer);
        texturedVertexBuffer = nullptr;
    }
    if (texturedPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, texturedPipeline);
        texturedPipeline = nullptr;
    }
    if (texturedFragmentShader) {
        SDL_ReleaseGPUShader(device, texturedFragmentShader);
        texturedFragmentShader = nullptr;
    }
    if (texturedVertexShader) {
        SDL_ReleaseGPUShader(device, texturedVertexShader);
        texturedVertexShader = nullptr;
    }
}

void EntityRenderSystem::prepareFrame(SDL_GPUCommandBuffer* commandBuffer) {
    // No-op
}

void EntityRenderSystem::render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                                const Camera& camera) {
    renderTexturedEntities(commandBuffer, renderPass, camera);
    renderUntexturedEntities(commandBuffer, renderPass, camera);
}

bool EntityRenderSystem::createTexturedPipeline() {
    // Vertex shader
    SDL_GPUShaderCreateInfo vertexInfo = {};
    vertexInfo.code = textured_quad_spirv_vertex;
    vertexInfo.code_size = sizeof(textured_quad_spirv_vertex);
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_uniform_buffers = 1;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_samplers = 0;

    texturedVertexShader = SDL_CreateGPUShader(device, &vertexInfo);
    if (!texturedVertexShader) {
        spdlog::error("Failed to create textured entity vertex shader: {}", SDL_GetError());
        return false;
    }

    // Fragment shader
    SDL_GPUShaderCreateInfo fragInfo = {};
    fragInfo.code = textured_quad_spirv_fragment;
    fragInfo.code_size = sizeof(textured_quad_spirv_fragment);
    fragInfo.entrypoint = "main";
    fragInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragInfo.num_uniform_buffers = 0;
    fragInfo.num_storage_buffers = 0;
    fragInfo.num_storage_textures = 0;
    fragInfo.num_samplers = 1;

    texturedFragmentShader = SDL_CreateGPUShader(device, &fragInfo);
    if (!texturedFragmentShader) {
        spdlog::error("Failed to create textured entity fragment shader: {}", SDL_GetError());
        return false;
    }

    // Pipeline setup
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(TexturedVertex);
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(TexturedVertex, position);

    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = offsetof(TexturedVertex, texCoord);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

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
    pipelineInfo.vertex_shader = texturedVertexShader;
    pipelineInfo.fragment_shader = texturedFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    texturedPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!texturedPipeline) {
        spdlog::error("Failed to create textured entity pipeline: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool EntityRenderSystem::createUntexturedPipeline() {
    SDL_GPUShaderCreateInfo vertexInfo = {};
    vertexInfo.code = colored_quad_spirv_vertex;
    vertexInfo.code_size = sizeof(colored_quad_spirv_vertex);
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_uniform_buffers = 1;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_samplers = 0;

    untexturedVertexShader = SDL_CreateGPUShader(device, &vertexInfo);
    if (!untexturedVertexShader) {
        spdlog::error("Failed to create untextured entity vertex shader: {}", SDL_GetError());
        return false;
    }

    SDL_GPUShaderCreateInfo fragInfo = {};
    fragInfo.code = colored_quad_spirv_fragment;
    fragInfo.code_size = sizeof(colored_quad_spirv_fragment);
    fragInfo.entrypoint = "main";
    fragInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragInfo.num_uniform_buffers = 1;
    fragInfo.num_storage_buffers = 0;
    fragInfo.num_storage_textures = 0;
    fragInfo.num_samplers = 0;

    untexturedFragmentShader = SDL_CreateGPUShader(device, &fragInfo);
    if (!untexturedFragmentShader) {
        spdlog::error("Failed to create untextured entity fragment shader: {}", SDL_GetError());
        return false;
    }

    // Pipeline setup
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(UntexturedVertex);
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttribute = {};
    vertexAttribute.location = 0;
    vertexAttribute.buffer_slot = 0;
    vertexAttribute.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttribute.offset = 0;

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = &vertexAttribute;
    vertexInputState.num_vertex_attributes = 1;

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
    pipelineInfo.vertex_shader = untexturedVertexShader;
    pipelineInfo.fragment_shader = untexturedFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    untexturedPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!untexturedPipeline) {
        spdlog::error("Failed to create untextured entity pipeline: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool EntityRenderSystem::createVertexBuffers() {
    TexturedVertex texturedVertices[6] = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
        {{1.0f, 0.0f}, {1.0f, 0.0f}}, {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
    };

    SDL_GPUBufferCreateInfo texturedBufferInfo = {};
    texturedBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    texturedBufferInfo.size = sizeof(texturedVertices);

    texturedVertexBuffer = SDL_CreateGPUBuffer(device, &texturedBufferInfo);
    if (!texturedVertexBuffer) {
        spdlog::error("Failed to create textured vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload textured vertices
    SDL_GPUTransferBufferCreateInfo texturedTransferInfo = {};
    texturedTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    texturedTransferInfo.size = sizeof(texturedVertices);

    SDL_GPUTransferBuffer* texturedTransfer =
        SDL_CreateGPUTransferBuffer(device, &texturedTransferInfo);
    void* texturedData = SDL_MapGPUTransferBuffer(device, texturedTransfer, false);
    SDL_memcpy(texturedData, texturedVertices, sizeof(texturedVertices));
    SDL_UnmapGPUTransferBuffer(device, texturedTransfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation texturedSrc = {texturedTransfer, 0};
    SDL_GPUBufferRegion texturedDst = {texturedVertexBuffer, 0, sizeof(texturedVertices)};
    SDL_UploadToGPUBuffer(copyPass, &texturedSrc, &texturedDst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, texturedTransfer);

    // Untextured unit quad (0,0 to 1,1)
    UntexturedVertex untexturedVertices[6] = {
        {{0.0f, 0.0f}}, {{1.0f, 0.0f}}, {{0.0f, 1.0f}},
        {{1.0f, 0.0f}}, {{1.0f, 1.0f}}, {{0.0f, 1.0f}},
    };

    SDL_GPUBufferCreateInfo untexturedBufferInfo = {};
    untexturedBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    untexturedBufferInfo.size = sizeof(untexturedVertices);

    untexturedVertexBuffer = SDL_CreateGPUBuffer(device, &untexturedBufferInfo);
    if (!untexturedVertexBuffer) {
        spdlog::error("Failed to create untextured vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload untextured vertices
    SDL_GPUTransferBufferCreateInfo untexturedTransferInfo = {};
    untexturedTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    untexturedTransferInfo.size = sizeof(untexturedVertices);

    SDL_GPUTransferBuffer* untexturedTransfer =
        SDL_CreateGPUTransferBuffer(device, &untexturedTransferInfo);
    void* untexturedData = SDL_MapGPUTransferBuffer(device, untexturedTransfer, false);
    SDL_memcpy(untexturedData, untexturedVertices, sizeof(untexturedVertices));
    SDL_UnmapGPUTransferBuffer(device, untexturedTransfer);

    commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation untexturedSrc = {untexturedTransfer, 0};
    SDL_GPUBufferRegion untexturedDst = {untexturedVertexBuffer, 0, sizeof(untexturedVertices)};
    SDL_UploadToGPUBuffer(copyPass, &untexturedSrc, &untexturedDst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, untexturedTransfer);

    return true;
}

void EntityRenderSystem::renderTexturedEntities(SDL_GPUCommandBuffer* commandBuffer,
                                                SDL_GPURenderPass* renderPass,
                                                const Camera& camera) {
    auto view = registry.view<Position, Renderable>();
    if (view.size_hint() == 0) {
        return;
    }

    SDL_BindGPUGraphicsPipeline(renderPass, texturedPipeline);

    ViewportData vp = camera.getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera.getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    SDL_GPUBufferBinding vertexBinding = {texturedVertexBuffer, 0};
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    for (auto [_, pos, renderable] : view.each()) {
        auto texture = textureLoader->getTexture(renderable.textureName);

        if (texture == nullptr) {
            continue;
        }

        glm::mat4 model = glm::translate(
            glm::mat4(1.0f), glm::vec3(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0f));
        model = glm::scale(model, glm::vec3(renderable.size.x, renderable.size.y, 1.0f));

        glm::mat4 mvp = camera.getViewProjectionMatrix() * model;
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

        SDL_GPUTextureSamplerBinding texBinding = {texture->texture, texture->sampler};
        SDL_BindGPUFragmentSamplers(renderPass, 0, &texBinding, 1);

        SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
    }
}

void EntityRenderSystem::renderUntexturedEntities(SDL_GPUCommandBuffer* commandBuffer,
                                                  SDL_GPURenderPass* renderPass,
                                                  const Camera& camera) {
    auto view = registry.view<Position, RenderableUntextured>();
    if (view.size_hint() == 0) {
        return;
    }

    SDL_BindGPUGraphicsPipeline(renderPass, untexturedPipeline);

    ViewportData vp = camera.getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera.getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    SDL_GPUBufferBinding vertexBinding = {untexturedVertexBuffer, 0};
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    for (auto [_, pos, renderable] : view.each()) {
        glm::mat4 model = glm::translate(
            glm::mat4(1.0f), glm::vec3(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0f));
        model = glm::scale(model, glm::vec3(renderable.size.x, renderable.size.y, 1.0f));

        glm::mat4 mvp = camera.getViewProjectionMatrix() * model;

        SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &renderable.color, sizeof(glm::vec4));

        SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
    }
}

}  // namespace SpaceRogueLite
