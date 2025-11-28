#include "window.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "imgui.h"
#include "shaders/simple_square_shaders.h"
#include "shaders/textured_quad_shaders.h"

using namespace SpaceRogueLite;

Window::Window(const std::string& title, size_t width, size_t height)
    : title(title), width(width), height(height) {}

Window::~Window() { close(); }

bool Window::initialize(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        spdlog::critical("SDL could not be initialized: {}", SDL_GetError());
        return false;
    }

    if (!TTF_Init()) {
        spdlog::critical("SDL_ttf could not be initialized: {}", SDL_GetError());
        return false;
    }

    sdlWindow = SDL_CreateWindow(title.c_str(), static_cast<int>(width), static_cast<int>(height),
                                 SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!sdlWindow) {
        spdlog::critical("SDL window could not be created: {}", SDL_GetError());
        return false;
    }

#ifndef NDEBUG
    bool debugMode = true;
#else
    bool debugMode = false;
#endif

    gpuDevice = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        debugMode, nullptr);
    if (!gpuDevice) {
        spdlog::critical("SDL GPU device could not be created: {}", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpuDevice, sdlWindow)) {
        spdlog::critical("Could not claim window for GPU device: {}", SDL_GetError());
        return false;
    }

    if (!initializeImgui()) {
        spdlog::critical("Could not initialize ImGui");
        return false;
    }

    if (!initializeSquareRendering()) {
        spdlog::critical("Could not initialize square rendering");
        return false;
    }

    if (!initializeTexturedQuadRendering()) {
        spdlog::critical("Could not initialize textured quad rendering");
        return false;
    }

    camera = std::make_unique<Camera>(width, height);

    SDL_WaitForGPUIdle(gpuDevice);

    return true;
}

bool Window::initializeImgui(void) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLGPU(sdlWindow);
    ImGui_ImplSDLGPU3_InitInfo initInfo = {};
    initInfo.Device = gpuDevice;
    initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpuDevice, sdlWindow);
    initInfo.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&initInfo);

    return true;
}

bool Window::initializeSquareRendering(void) {
    // Create vertex shader
    SDL_GPUShaderCreateInfo vertexShaderInfo = {};
    vertexShaderInfo.code = simple_square_spirv_vertex;
    vertexShaderInfo.code_size = sizeof(simple_square_spirv_vertex);
    vertexShaderInfo.entrypoint = "main";
    vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexShaderInfo.num_uniform_buffers = 1;  // Using one uniform buffer for projection matrix
    vertexShaderInfo.num_storage_buffers = 0;
    vertexShaderInfo.num_storage_textures = 0;
    vertexShaderInfo.num_samplers = 0;

    squareVertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
    if (!squareVertexShader) {
        spdlog::error("Failed to create vertex shader: {}", SDL_GetError());
        return false;
    }

    // Create fragment shader
    SDL_GPUShaderCreateInfo fragmentShaderInfo = {};
    fragmentShaderInfo.code = simple_square_spirv_fragment;
    fragmentShaderInfo.code_size = sizeof(simple_square_spirv_fragment);
    fragmentShaderInfo.entrypoint = "main";
    fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragmentShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentShaderInfo.num_uniform_buffers = 0;
    fragmentShaderInfo.num_storage_buffers = 0;
    fragmentShaderInfo.num_storage_textures = 0;
    fragmentShaderInfo.num_samplers = 0;

    squareFragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
    if (!squareFragmentShader) {
        spdlog::error("Failed to create fragment shader: {}", SDL_GetError());
        return false;
    }

    // Create vertex buffer (two triangles forming a square at center: 64x64 pixels)
    // Vertices are in screen space (pixels), will be transformed to NDC in shader
    // Position at center will be calculated in render function
    // For now, create a 64x64 square at origin
    glm::vec2 vertices[6] = {
        // First triangle
        {0.0f, 0.0f},   // Top-left
        {64.0f, 0.0f},  // Top-right
        {0.0f, 64.0f},  // Bottom-left
        // Second triangle
        {64.0f, 0.0f},   // Top-right
        {64.0f, 64.0f},  // Bottom-right
        {0.0f, 64.0f}    // Bottom-left
    };

    SDL_GPUBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = sizeof(vertices);

    squareVertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
    if (!squareVertexBuffer) {
        spdlog::error("Failed to create vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload vertex data
    SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
    transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferInfo.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferInfo);
    if (!transferBuffer) {
        spdlog::error("Failed to create transfer buffer: {}", SDL_GetError());
        return false;
    }

    void* transferData = SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false);
    SDL_memcpy(transferData, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);

    SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation transferLocation = {};
    transferLocation.transfer_buffer = transferBuffer;
    transferLocation.offset = 0;

    SDL_GPUBufferRegion bufferRegion = {};
    bufferRegion.buffer = squareVertexBuffer;
    bufferRegion.offset = 0;
    bufferRegion.size = sizeof(vertices);

    SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
    SDL_WaitForGPUIdle(gpuDevice);  // Wait for upload to complete
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);

    // Create graphics pipeline
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(glm::vec2);
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
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, sdlWindow);
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
    pipelineInfo.vertex_shader = squareVertexShader;
    pipelineInfo.fragment_shader = squareFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    squarePipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
    if (!squarePipeline) {
        spdlog::error("Failed to create graphics pipeline: {}", SDL_GetError());
        return false;
    }

    spdlog::info("Square rendering initialized successfully");
    spdlog::info("Square resources created: pipeline={}, vbuffer={}", squarePipeline != nullptr,
                 squareVertexBuffer != nullptr);
    return true;
}

bool Window::initializeTexturedQuadRendering(void) {
    // Load texture using SDL_image
    const char* texturePath = "../../../assets/floor1.png";
    SDL_Surface* surface = IMG_Load(texturePath);
    if (!surface) {
        spdlog::error("Failed to load texture {}: {}", texturePath, SDL_GetError());
        return false;
    }

    // Convert to RGBA32 format for consistent GPU format
    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!convertedSurface) {
        spdlog::error("Failed to convert surface to RGBA32: {}", SDL_GetError());
        return false;
    }

    // Create GPU texture
    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = static_cast<Uint32>(convertedSurface->w);
    textureInfo.height = static_cast<Uint32>(convertedSurface->h);
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    texturedQuadTexture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
    if (!texturedQuadTexture) {
        spdlog::error("Failed to create GPU texture: {}", SDL_GetError());
        SDL_DestroySurface(convertedSurface);
        return false;
    }

    // Upload texture data via transfer buffer
    Uint32 textureDataSize = convertedSurface->w * convertedSurface->h * 4;

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = textureDataSize;

    SDL_GPUTransferBuffer* textureTransferBuffer =
        SDL_CreateGPUTransferBuffer(gpuDevice, &transferInfo);
    if (!textureTransferBuffer) {
        spdlog::error("Failed to create texture transfer buffer: {}", SDL_GetError());
        SDL_DestroySurface(convertedSurface);
        return false;
    }

    void* textureData = SDL_MapGPUTransferBuffer(gpuDevice, textureTransferBuffer, false);
    SDL_memcpy(textureData, convertedSurface->pixels, textureDataSize);
    SDL_UnmapGPUTransferBuffer(gpuDevice, textureTransferBuffer);

    // Upload to GPU texture
    SDL_GPUCommandBuffer* uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);

    SDL_GPUTextureTransferInfo transferSource = {};
    transferSource.transfer_buffer = textureTransferBuffer;
    transferSource.offset = 0;
    transferSource.pixels_per_row = convertedSurface->w;
    transferSource.rows_per_layer = convertedSurface->h;

    SDL_GPUTextureRegion textureRegion = {};
    textureRegion.texture = texturedQuadTexture;
    textureRegion.x = 0;
    textureRegion.y = 0;
    textureRegion.z = 0;
    textureRegion.w = convertedSurface->w;
    textureRegion.h = convertedSurface->h;
    textureRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &transferSource, &textureRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
    SDL_WaitForGPUIdle(gpuDevice);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, textureTransferBuffer);

    int texWidth = convertedSurface->w;
    int texHeight = convertedSurface->h;
    SDL_DestroySurface(convertedSurface);

    // Create sampler
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    texturedQuadSampler = SDL_CreateGPUSampler(gpuDevice, &samplerInfo);
    if (!texturedQuadSampler) {
        spdlog::error("Failed to create sampler: {}", SDL_GetError());
        return false;
    }

    // Create vertex shader
    SDL_GPUShaderCreateInfo vertexShaderInfo = {};
    vertexShaderInfo.code = textured_quad_spirv_vertex;
    vertexShaderInfo.code_size = sizeof(textured_quad_spirv_vertex);
    vertexShaderInfo.entrypoint = "main";
    vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexShaderInfo.num_uniform_buffers = 1;
    vertexShaderInfo.num_storage_buffers = 0;
    vertexShaderInfo.num_storage_textures = 0;
    vertexShaderInfo.num_samplers = 0;

    texturedQuadVertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
    if (!texturedQuadVertexShader) {
        spdlog::error("Failed to create textured quad vertex shader: {}", SDL_GetError());
        return false;
    }

    // Create fragment shader - num_samplers = 1 is critical for texture sampling
    SDL_GPUShaderCreateInfo fragmentShaderInfo = {};
    fragmentShaderInfo.code = textured_quad_spirv_fragment;
    fragmentShaderInfo.code_size = sizeof(textured_quad_spirv_fragment);
    fragmentShaderInfo.entrypoint = "main";
    fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragmentShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentShaderInfo.num_uniform_buffers = 0;
    fragmentShaderInfo.num_storage_buffers = 0;
    fragmentShaderInfo.num_storage_textures = 0;
    fragmentShaderInfo.num_samplers = 1;

    texturedQuadFragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
    if (!texturedQuadFragmentShader) {
        spdlog::error("Failed to create textured quad fragment shader: {}", SDL_GetError());
        return false;
    }

    // Vertex structure: position (float2) + texCoord (float2)
    struct TexturedVertex {
        float x, y;
        float u, v;
    };

    // Create a 128x128 quad
    float quadSize = 128.0f;
    TexturedVertex vertices[6] = {// First triangle (top-left, top-right, bottom-left)
                                  {0.0f, 0.0f, 0.0f, 0.0f},
                                  {quadSize, 0.0f, 1.0f, 0.0f},
                                  {0.0f, quadSize, 0.0f, 1.0f},
                                  // Second triangle (top-right, bottom-right, bottom-left)
                                  {quadSize, 0.0f, 1.0f, 0.0f},
                                  {quadSize, quadSize, 1.0f, 1.0f},
                                  {0.0f, quadSize, 0.0f, 1.0f}};

    SDL_GPUBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = sizeof(vertices);

    texturedQuadVertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
    if (!texturedQuadVertexBuffer) {
        spdlog::error("Failed to create textured quad vertex buffer: {}", SDL_GetError());
        return false;
    }

    // Upload vertex data
    SDL_GPUTransferBufferCreateInfo vtxTransferInfo = {};
    vtxTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vtxTransferInfo.size = sizeof(vertices);

    SDL_GPUTransferBuffer* vtxTransferBuffer =
        SDL_CreateGPUTransferBuffer(gpuDevice, &vtxTransferInfo);
    void* vtxData = SDL_MapGPUTransferBuffer(gpuDevice, vtxTransferBuffer, false);
    SDL_memcpy(vtxData, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(gpuDevice, vtxTransferBuffer);

    SDL_GPUCommandBuffer* vtxUploadCmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
    SDL_GPUCopyPass* vtxCopyPass = SDL_BeginGPUCopyPass(vtxUploadCmd);

    SDL_GPUTransferBufferLocation vtxTransferLoc = {};
    vtxTransferLoc.transfer_buffer = vtxTransferBuffer;
    vtxTransferLoc.offset = 0;

    SDL_GPUBufferRegion vtxBufferRegion = {};
    vtxBufferRegion.buffer = texturedQuadVertexBuffer;
    vtxBufferRegion.offset = 0;
    vtxBufferRegion.size = sizeof(vertices);

    SDL_UploadToGPUBuffer(vtxCopyPass, &vtxTransferLoc, &vtxBufferRegion, false);
    SDL_EndGPUCopyPass(vtxCopyPass);
    SDL_SubmitGPUCommandBuffer(vtxUploadCmd);
    SDL_WaitForGPUIdle(gpuDevice);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, vtxTransferBuffer);

    // Create graphics pipeline
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(TexturedVertex);
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    // Position attribute
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = 0;
    // TexCoord attribute
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = 8;

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, sdlWindow);
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
    pipelineInfo.vertex_shader = texturedQuadVertexShader;
    pipelineInfo.fragment_shader = texturedQuadFragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.has_depth_stencil_target = false;

    texturedQuadPipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
    if (!texturedQuadPipeline) {
        spdlog::error("Failed to create textured quad pipeline: {}", SDL_GetError());
        return false;
    }

    spdlog::info("Textured quad rendering initialized successfully (texture: {}x{})", texWidth,
                 texHeight);
    return true;
}

void Window::close(void) {
    // Wait for GPU to finish before cleanup
    SDL_WaitForGPUIdle(gpuDevice);

    camera.reset();

    // Cleanup square rendering resources
    if (squarePipeline) {
        SDL_ReleaseGPUGraphicsPipeline(gpuDevice, squarePipeline);
        squarePipeline = nullptr;
    }
    if (squareVertexBuffer) {
        SDL_ReleaseGPUBuffer(gpuDevice, squareVertexBuffer);
        squareVertexBuffer = nullptr;
    }
    if (squareVertexShader) {
        SDL_ReleaseGPUShader(gpuDevice, squareVertexShader);
        squareVertexShader = nullptr;
    }
    if (squareFragmentShader) {
        SDL_ReleaseGPUShader(gpuDevice, squareFragmentShader);
        squareFragmentShader = nullptr;
    }

    // Cleanup textured quad rendering resources
    if (texturedQuadPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(gpuDevice, texturedQuadPipeline);
        texturedQuadPipeline = nullptr;
    }
    if (texturedQuadVertexBuffer) {
        SDL_ReleaseGPUBuffer(gpuDevice, texturedQuadVertexBuffer);
        texturedQuadVertexBuffer = nullptr;
    }
    if (texturedQuadVertexShader) {
        SDL_ReleaseGPUShader(gpuDevice, texturedQuadVertexShader);
        texturedQuadVertexShader = nullptr;
    }
    if (texturedQuadFragmentShader) {
        SDL_ReleaseGPUShader(gpuDevice, texturedQuadFragmentShader);
        texturedQuadFragmentShader = nullptr;
    }
    if (texturedQuadTexture) {
        SDL_ReleaseGPUTexture(gpuDevice, texturedQuadTexture);
        texturedQuadTexture = nullptr;
    }
    if (texturedQuadSampler) {
        SDL_ReleaseGPUSampler(gpuDevice, texturedQuadSampler);
        texturedQuadSampler = nullptr;
    }

    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpuDevice, sdlWindow);
    SDL_DestroyGPUDevice(gpuDevice);
    SDL_DestroyWindow(sdlWindow);

    // Cleanup SDL_ttf (SDL_image needs no cleanup in SDL3)
    TTF_Quit();

    SDL_Quit();
}

void Window::update(int64_t timeSinceLastFrame, bool& quit) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            quit = true;
        }
    }

    updateUI(timeSinceLastFrame, quit);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
    if (!commandBuffer) {
        spdlog::error("Failed to acquire GPU command buffer: {}", SDL_GetError());
        return;
    }

    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->Valid) {
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, sdlWindow, &swapchainTexture, nullptr,
                                               nullptr)) {
        spdlog::error("Failed to acquire swapchain texture: {}", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }

    if (swapchainTexture != nullptr) {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        colorTarget.clear_color.r = 0.0f;
        colorTarget.clear_color.g = 0.0f;
        colorTarget.clear_color.b = 0.0f;
        colorTarget.clear_color.a = 1.0f;

        SDL_GPURenderPass* renderPass =
            SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);

        // Render square first (behind ImGui)
        renderSquare(commandBuffer, renderPass);

        // Render textured quad (next to the red square)
        renderTexturedQuad(commandBuffer, renderPass);

        if (drawData && drawData->Valid) {
            ImGui_ImplSDLGPU3_RenderDrawData(drawData, commandBuffer, renderPass);
        }

        SDL_EndGPURenderPass(renderPass);
    }

    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void Window::updateUI(int64_t timeSinceLastFrame, bool& quit) {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();
}

void Window::renderSquare(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass) {
    if (!squarePipeline || !squareVertexBuffer) {
        spdlog::warn("Square rendering resources not initialized");
        return;
    }

    // Bind pipeline first
    SDL_BindGPUGraphicsPipeline(renderPass, squarePipeline);

    // Calculate position to center the square
    float centerX = static_cast<float>(width) / 2.0f - 32.0f;
    float centerY = static_cast<float>(height) / 2.0f - 32.0f;

    // Create model matrix for square position
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, 0.0f));

    // Use camera to calculate MVP
    glm::mat4 mvp = camera->getViewProjectionMatrix() * model;

    // Push MVP matrix as uniform data
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

    // Set viewport and scissor from camera
    ViewportData vp = camera->getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera->getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = squareVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // Draw
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
}

void Window::renderTexturedQuad(SDL_GPUCommandBuffer* commandBuffer,
                                SDL_GPURenderPass* renderPass) {
    if (!texturedQuadPipeline || !texturedQuadVertexBuffer || !texturedQuadTexture ||
        !texturedQuadSampler) {
        return;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, texturedQuadPipeline);

    // Position: offset 100 pixels right from center
    float posX = static_cast<float>(width) / 2.0f + 50.0f;
    float posY = static_cast<float>(height) / 2.0f - 64.0f;

    // Create model matrix for quad position
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, 0.0f));

    // Use camera to calculate MVP
    glm::mat4 mvp = camera->getViewProjectionMatrix() * model;

    SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

    // Set viewport and scissor from camera
    ViewportData vp = camera->getViewport();
    SDL_GPUViewport viewport = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    SDL_SetGPUViewport(renderPass, &viewport);

    ScissorData sc = camera->getScissor();
    SDL_Rect scissor = {sc.x, sc.y, sc.width, sc.height};
    SDL_SetGPUScissor(renderPass, &scissor);

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = texturedQuadVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // Bind texture-sampler pair for fragment shader
    SDL_GPUTextureSamplerBinding textureSamplerBinding = {};
    textureSamplerBinding.texture = texturedQuadTexture;
    textureSamplerBinding.sampler = texturedQuadSampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

    // Draw
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
}