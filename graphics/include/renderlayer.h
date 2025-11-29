#pragma once
#include <SDL3/SDL.h>
#include <cstdint>

namespace SpaceRogueLite {

class Camera;

class RenderLayer {
public:
    virtual ~RenderLayer() = default;

    virtual void prepareFrame(SDL_GPUCommandBuffer* commandBuffer) {}

    virtual void render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                        const Camera& camera) = 0;

    virtual int32_t getOrder() const = 0;
};

namespace LayerOrder {
constexpr int32_t BACKGROUND = 0;
constexpr int32_t TILES = 100;
constexpr int32_t ENTITIES = 200;
constexpr int32_t EFFECTS = 300;
constexpr int32_t UI = 1000;
}  // namespace LayerOrder

}  // namespace SpaceRogueLite
