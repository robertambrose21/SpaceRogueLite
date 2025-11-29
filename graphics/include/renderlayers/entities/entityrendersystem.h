#pragma once

#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

#include "camera.h"
#include "renderlayers/renderlayer.h"

namespace SpaceRogueLite {

class EntityRenderSystem : public RenderLayer {
public:
    explicit EntityRenderSystem(entt::registry& registry);
    EntityRenderSystem(const EntityRenderSystem&) = delete;
    EntityRenderSystem& operator=(const EntityRenderSystem&) = delete;

    ~EntityRenderSystem() override;

    bool initialize() override;
    void shutdown();

    void prepareFrame(SDL_GPUCommandBuffer* commandBuffer) override;

    void render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                const Camera& camera) override;

    int32_t getOrder() const override { return LayerOrder::ENTITIES; }

private:
    entt::registry& registry;

    struct EntityTexture {
        SDL_GPUTexture* texture = nullptr;
        SDL_GPUSampler* sampler = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    std::unordered_map<std::string, EntityTexture> textureCache;

    SDL_GPUShader* texturedVertexShader = nullptr;
    SDL_GPUShader* texturedFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* texturedPipeline = nullptr;
    SDL_GPUBuffer* texturedVertexBuffer = nullptr;

    SDL_GPUShader* untexturedVertexShader = nullptr;
    SDL_GPUShader* untexturedFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* untexturedPipeline = nullptr;
    SDL_GPUBuffer* untexturedVertexBuffer = nullptr;

    struct TexturedVertex {
        glm::vec2 position;
        glm::vec2 texCoord;
    };

    struct UntexturedVertex {
        glm::vec2 position;
    };

    bool createTexturedPipeline();
    bool createUntexturedPipeline();
    bool createVertexBuffers();

    EntityTexture* loadTexture(const std::string& path);

    void renderTexturedEntities(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                                const Camera& camera);
    void renderUntexturedEntities(SDL_GPUCommandBuffer* commandBuffer,
                                  SDL_GPURenderPass* renderPass, const Camera& camera);
};

}  // namespace SpaceRogueLite
