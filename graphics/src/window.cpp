#include "window.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "imgui.h"
#include "shaders/simple_square_shaders.h"

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
    vertexShaderInfo.code = spirv_vertex;
    vertexShaderInfo.code_size = sizeof(spirv_vertex);
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
    fragmentShaderInfo.code = spirv_fragment;
    fragmentShaderInfo.code_size = sizeof(spirv_fragment);
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

void Window::close(void) {
    // Wait for GPU to finish before cleanup
    SDL_WaitForGPUIdle(gpuDevice);

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

    // Push uniform data immediately after binding pipeline (before other bindings)
    // Create orthographic projection matrix using glm
    // This transforms from pixel coordinates to NDC (-1 to 1)
    glm::mat4 projection =
        glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);

    // Calculate position to center the square
    float centerX = static_cast<float>(width) / 2.0f - 32.0f;
    float centerY = static_cast<float>(height) / 2.0f - 32.0f;

    // Translate to center position
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, 0.0f));

    // Combine projection and model matrices
    glm::mat4 mvp = projection * model;

    // Push projection matrix as uniform data (must be after pipeline bind, before draw)
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &mvp, sizeof(glm::mat4));

    // Set viewport and scissor
    SDL_GPUViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    SDL_Rect scissor = {};
    scissor.x = 0;
    scissor.y = 0;
    scissor.w = static_cast<int>(width);
    scissor.h = static_cast<int>(height);
    SDL_SetGPUScissor(renderPass, &scissor);

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = squareVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // Draw
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
}