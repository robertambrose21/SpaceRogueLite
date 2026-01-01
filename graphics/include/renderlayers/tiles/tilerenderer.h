#pragma once
#include <SDL3/SDL.h>

#include <grid.h>
#include <tilevariant.h>
#include <entt/entt.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

    void loadTileVariantsIntoAtlas(const std::set<TileVariant>& variants);

    void prepareFrame(SDL_GPUCommandBuffer* commandBuffer) override;

    void render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                const Camera& camera) override;

    int32_t getOrder() const override { return LayerOrder::TILES; }

    void invalidateCache();

private:
    static constexpr int CHUNK_SIZE_TILES = 16;
    static constexpr int CHUNK_SIZE_PIXELS = CHUNK_SIZE_TILES * TILE_SIZE;  // 512

    struct TileChunk {
        glm::ivec2 chunkPos{0, 0};
        glm::ivec2 tileCount{0, 0};
        glm::uvec2 pixelSize{0, 0};
        glm::vec2 worldMin{0.0f, 0.0f};  // World-space bounds for culling
        glm::vec2 worldMax{0.0f, 0.0f};
        uint32_t layerIndex = 0;  // Index into chunk texture array
        bool isDirty = true;
        bool isVisible = true;  // false if all tiles empty (skip rendering)
    };

    struct ChunkInstance {
        glm::vec2 worldPos;  // Chunk top-left world position
        glm::vec2 size;      // Chunk size in pixels
        float layerIndex;    // Texture array layer
        float padding[3];    // Align to 32 bytes
    };

    struct QuadVertex {
        glm::vec2 position;
        glm::vec2 texCoord;
    };

    struct TileInstance {
        glm::vec2 position;  // World position
        glm::vec4 uvBounds;  // Atlas UV (u_min, v_min, u_max, v_max)
    };

    std::unique_ptr<TileAtlas> atlas = nullptr;

    SDL_GPUShader* composeVertexShader = nullptr;
    SDL_GPUShader* composeFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* composePipeline = nullptr;

    SDL_GPUShader* displayVertexShader = nullptr;
    SDL_GPUShader* displayFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* displayPipeline = nullptr;

    SDL_GPUBuffer* quadVertexBuffer = nullptr;
    SDL_GPUBuffer* tileInstanceBuffer = nullptr;
    SDL_GPUTransferBuffer* tileInstanceTransfer = nullptr;
    uint32_t tileInstanceBufferCapacity = 0;

    SDL_GPUTexture* chunkTextureArray = nullptr;
    uint32_t chunkTextureArrayLayers = 0;
    SDL_GPUSampler* chunkSampler = nullptr;

    SDL_GPUBuffer* chunkInstanceBuffer = nullptr;
    SDL_GPUTransferBuffer* chunkInstanceTransfer = nullptr;
    uint32_t chunkInstanceBufferCapacity = 0;

    std::unordered_map<glm::ivec2, TileChunk> chunks;
    std::vector<uint32_t> freeLayerIndices;
    uint32_t nextLayerIndex = 0;
    glm::ivec2 cachedGridSize{0, 0};
    glm::ivec2 chunkGridSize{0, 0};
    bool cacheValid = false;

    glm::vec2 cachedCameraPos{-1.0f, -1.0f};
    glm::vec2 cachedCameraSize{0.0f, 0.0f};
    std::vector<TileChunk*> cachedVisibleChunks;

    uint32_t uploadedChunkCount = 0;
    bool chunkInstancesNeedUpload = true;

    bool createShaders();
    bool createComposePipeline();
    bool createDisplayPipeline();
    bool createQuadVertexBuffer();
    bool createChunkSampler();
    bool ensureTileInstanceBuffer(uint32_t tileCount);
    bool ensureChunkInstanceBuffer(uint32_t chunkCount);

    bool ensureChunkTextureArray(uint32_t requiredLayers);
    uint32_t allocateLayerIndex();
    void freeLayerIndex(uint32_t index);

    void updateChunkGrid();
    void createAllChunks();
    TileChunk& getOrCreateChunk(glm::ivec2 chunkPos);
    void destroyChunk(const TileChunk& chunk);
    void destroyAllChunks();

    glm::ivec2 calculateChunkTileCount(glm::ivec2 chunkPos) const;

    void markDirtyChunksFromRegion(const GridRegion& dirtyRegion);

    void updateVisibleChunks(const Camera& camera);

    void rebakeChunk(SDL_GPUCommandBuffer* commandBuffer, TileChunk& chunk);
    void rebakeDirtyChunks(SDL_GPUCommandBuffer* commandBuffer);
    void uploadChunkInstances(SDL_GPUCommandBuffer* commandBuffer);
    void renderVisibleChunks(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                             const Camera& camera);
};

}  // namespace SpaceRogueLite
