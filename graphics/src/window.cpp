#include "window.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "imgui.h"

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

void Window::addRenderLayer(std::unique_ptr<RenderLayer> layer) {
    renderLayers.push_back(std::move(layer));
    layersSorted = false;
}

void Window::sortLayers() {
    std::sort(renderLayers.begin(), renderLayers.end(),
              [](const auto& a, const auto& b) { return a->getOrder() < b->getOrder(); });
    layersSorted = true;
}

void Window::close(void) {
    // Wait for GPU to finish before cleanup
    SDL_WaitForGPUIdle(gpuDevice);

    camera.reset();
    renderLayers.clear();

    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpuDevice, sdlWindow);
    SDL_DestroyGPUDevice(gpuDevice);
    SDL_DestroyWindow(sdlWindow);

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

    if (!layersSorted) {
        sortLayers();
    }
    for (auto& layer : renderLayers) {
        layer->prepareFrame(commandBuffer);
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
        // colorTarget.cycle = true; // TODO: Read into this more

        SDL_GPURenderPass* renderPass =
            SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);

        for (auto& layer : renderLayers) {
            layer->render(commandBuffer, renderPass, *camera);
        }

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