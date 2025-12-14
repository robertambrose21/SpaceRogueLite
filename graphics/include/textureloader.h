#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

namespace SpaceRogueLite {

struct Texture {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    int id = -1;
    std::string name;
};

class TextureLoader {
public:
    explicit TextureLoader(SDL_GPUDevice* device);
    ~TextureLoader();

    void loadTextureDefinitions(const std::string& jsonPath, const std::string& basePath);
    Texture* getTexture(const std::string& name);
    Texture* getTextureById(int id);
    SDL_Surface* loadSurfaceById(int textureId);

private:
    struct TextureDefinition {
        int id;
        std::string name;
        std::string path;
    };

    Texture* loadAndAssignTexture(const std::string& path, const std::string& name, int id = -1);
    SDL_Surface* loadSurfaceFromPath(const std::string& path);

    std::unordered_map<std::string, Texture> textureCache;
    std::unordered_map<int, TextureDefinition> textureDefinitions;
    std::unordered_map<std::string, int> nameToIdMapping;
    std::string basePath;
    SDL_GPUDevice* device = nullptr;
};

}  // namespace SpaceRogueLite