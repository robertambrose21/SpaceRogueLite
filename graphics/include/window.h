#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace SpaceRogueLite {

class Window {
public:
    explicit Window(const std::string& title, size_t width, size_t height);
    ~Window();

    bool initialize(void);
    bool initializeImgui(void);
    bool initializeSquareRendering(void);
    bool initializeTexturedQuadRendering(void);
    void close(void);

    void update(int64_t timeSinceLastFrame, bool& quit);
    void updateUI(int64_t timeSinceLastFrame, bool& quit);
    void renderSquare(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);
    void renderTexturedQuad(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);

private:
    std::string title;
    size_t width;
    size_t height;

    SDL_Window* sdlWindow = nullptr;
    SDL_GPUDevice* gpuDevice = nullptr;

    // Square rendering resources
    SDL_GPUShader* squareVertexShader = nullptr;
    SDL_GPUShader* squareFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* squarePipeline = nullptr;
    SDL_GPUBuffer* squareVertexBuffer = nullptr;

    // Textured quad rendering resources
    SDL_GPUShader* texturedQuadVertexShader = nullptr;
    SDL_GPUShader* texturedQuadFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* texturedQuadPipeline = nullptr;
    SDL_GPUBuffer* texturedQuadVertexBuffer = nullptr;
    SDL_GPUTexture* texturedQuadTexture = nullptr;
    SDL_GPUSampler* texturedQuadSampler = nullptr;
};

}  // namespace SpaceRogueLite