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
    std::string name;
};

class TextureLoader {
public:
    explicit TextureLoader(SDL_GPUDevice* device);
    ~TextureLoader();

    Texture* loadAndAssignTexture(const std::string& path, const std::string& name);
    Texture* getTexture(const std::string& name);

private:
    std::unordered_map<std::string, Texture> textureCache;
    SDL_GPUDevice* device = nullptr;
};

}  // namespace SpaceRogueLite