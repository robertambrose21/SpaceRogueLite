#pragma once
#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <map>
#include <memory>

#include "camera.h"
#include "renderlayers/renderlayer.h"
#include "tileatlas.h"

namespace SpaceRogueLite {

class TileRenderer : public RenderLayer {
public:
    explicit TileRenderer();
    TileRenderer(const TileRenderer&) = delete;
    TileRenderer& operator=(const TileRenderer&) = delete;

    ~TileRenderer() override;

    bool initialize() override;
    void shutdown();

    void setGrid(std::unique_ptr<Grid> grid);
    Grid* getGrid();

    TileId loadAtlasTile(const std::string& path);
    std::map<std::string, TileId> loadAtlasTiles(const std::vector<std::string>& paths);

    void prepareFrame(SDL_GPUCommandBuffer* commandBuffer) override;

    void render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                const Camera& camera) override;

    int32_t getOrder() const override { return LayerOrder::TILES; }

    void invalidateCache();

private:
    std::unique_ptr<TileAtlas> atlas = nullptr;
    std::unique_ptr<Grid> grid = nullptr;

    SDL_GPUShader* composeVertexShader = nullptr;
    SDL_GPUShader* composeFragmentShader = nullptr;

    SDL_GPUGraphicsPipeline* composePipeline = nullptr;

    SDL_GPUShader* displayVertexShader = nullptr;
    SDL_GPUShader* displayFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* displayPipeline = nullptr;

    SDL_GPUBuffer* quadVertexBuffer = nullptr;
    SDL_GPUBuffer* instanceBuffer = nullptr;
    SDL_GPUTransferBuffer* instanceTransfer = nullptr;
    uint32_t instanceBufferCapacity = 0;

    SDL_GPUTexture* bakedTexture = nullptr;
    SDL_GPUSampler* bakedSampler = nullptr;
    SDL_GPUBuffer* displayVertexBuffer = nullptr;
    uint32_t bakedWidth = 0;
    uint32_t bakedHeight = 0;
    bool cacheValid = false;

    bool createShaders();
    bool createComposePipeline();
    bool createDisplayPipeline();
    bool createQuadVertexBuffer();
    bool createRenderTarget(uint32_t width, uint32_t height);
    bool ensureInstanceBuffer(uint32_t tileCount);
    void rebakeTiles(SDL_GPUCommandBuffer* commandBuffer);
    void renderBakedTexture(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                            const Camera& camera);

    struct QuadVertex {
        glm::vec2 position;
        glm::vec2 texCoord;
    };

    struct TileInstance {
        glm::vec2 position;  // World position
        glm::vec4 uvBounds;  // Atlas UV (u_min, v_min, u_max, v_max)
    };
};

}  // namespace SpaceRogueLite
