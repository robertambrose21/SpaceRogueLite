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
    void close(void);

    void update(int64_t timeSinceLastFrame, bool& quit);
    void updateUI(int64_t timeSinceLastFrame, bool& quit);

private:
    std::string title;
    size_t width;
    size_t height;

    SDL_Window* sdlWindow = nullptr;
    SDL_GPUDevice* gpuDevice = nullptr;
};

}  // namespace SpaceRogueLite