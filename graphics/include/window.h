#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "camera.h"
#include "renderlayers/renderlayer.h"

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
        auto layer = std::make_unique<T>(std::forward<Args>(args)...);

        if (!layer->doInitialize(gpuDevice, sdlWindow)) {
            spdlog::error("Failed to initialize render layer {}", layer->getName());
            return nullptr;
        }

        auto layerPtr = layer.get();
        renderLayers.push_back(std::move(layer));
        layersSorted = false;

        return layerPtr;
    }

    template <typename T>
    T* getRenderLayer() {
        for (auto& layer : renderLayers) {
            if (auto* ptr = dynamic_cast<T*>(layer.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    void update(int64_t timeSinceLastFrame, bool& quit);
    void updateUI(int64_t timeSinceLastFrame, bool& quit);

private:
    std::string title;
    size_t width;
    size_t height;

    SDL_Window* sdlWindow = nullptr;
    SDL_GPUDevice* gpuDevice = nullptr;

    std::unique_ptr<Camera> camera;

    std::vector<std::unique_ptr<RenderLayer>> renderLayers;
    bool layersSorted = false;

    void sortLayers();
};

}  // namespace SpaceRogueLite