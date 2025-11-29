#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
namespace SpaceRogueLite {

class Camera;

class RenderLayer {
    friend class Window;

public:
    RenderLayer(const std::string& name) : name(name) {}
    virtual ~RenderLayer() = default;

    virtual void prepareFrame(SDL_GPUCommandBuffer* commandBuffer) {}

    virtual void render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                        const Camera& camera) = 0;

    virtual bool initialize() = 0;
    virtual int32_t getOrder() const = 0;

    std::string getName(void) const { return name; }

protected:
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;

private:
    bool doInitialize(SDL_GPUDevice* device, SDL_Window* window) {
        this->device = device;
        this->window = window;

        return initialize();
    }

    std::string name;
};

namespace LayerOrder {
constexpr int32_t BACKGROUND = 0;
constexpr int32_t TILES = 100;
constexpr int32_t ENTITIES = 200;
constexpr int32_t EFFECTS = 300;
constexpr int32_t UI = 1000;
}  // namespace LayerOrder

}  // namespace SpaceRogueLite
