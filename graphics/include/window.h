#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>
#include <entt/entt.hpp>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include "camera.h"
#include "renderlayers/renderlayer.h"
#include "textureloader.h"

namespace SpaceRogueLite {

class Window {
public:
    explicit Window(const std::string& title, size_t width, size_t height);
    ~Window();

    bool initialize(void);
    bool initializeImgui(void);
    void close(void);

    template <typename T, typename... Args>
    T* createRenderLayer(Args&&... args) {
        static_assert(std::is_base_of_v<RenderLayer, T>, "T must derive from RenderLayer");

        auto layer = std::make_unique<T>(std::forward<Args>(args)...);

        if (!layer->doInitialize(gpuDevice, sdlWindow, textureLoader.get())) {
            spdlog::error("Failed to initialize render layer {}", layer->getName());
            return nullptr;
        }

        auto* layerPtr = layer.get();
        renderLayers.insert(std::move(layer));

        return layerPtr;
    }

    template <typename T>
    T* getRenderLayer() {
        static_assert(std::is_base_of_v<RenderLayer, T>, "T must derive from RenderLayer");
        for (const auto& layer : renderLayers) {
            if (auto* ptr = dynamic_cast<T*>(layer.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    TextureLoader* getTextureLoader();
    Camera* getCamera();

    void update(int64_t timeSinceLastFrame, bool& quit);
    void updateUI(int64_t timeSinceLastFrame, bool& quit);

private:
    std::string title;
    size_t width;
    size_t height;

    SDL_Window* sdlWindow = nullptr;
    SDL_GPUDevice* gpuDevice = nullptr;

    std::unique_ptr<Camera> camera;

    std::set<std::unique_ptr<RenderLayer>, RenderLayerComparator> renderLayers;

    std::unique_ptr<TextureLoader> textureLoader;
};

}  // namespace SpaceRogueLite