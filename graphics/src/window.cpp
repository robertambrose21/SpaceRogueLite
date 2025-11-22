#include "window.h"

using namespace SpaceRogueLite;

Window::Window(const std::string& title, size_t width, size_t height)
    : title(title), width(width), height(height) {}

Window::~Window() { close(); }

bool Window::initialize(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        spdlog::critical("SDL could not be initialized: {}", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("Window", static_cast<int>(width), static_cast<int>(height), 0,
                                     &sdlWindow, &sdlRenderer)) {
        spdlog::critical("SDL window could not be created: {}", SDL_GetError());
        return false;
    }

    return true;
}

void Window::close(void) {
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

void Window::update(int64_t timeSinceLastFrame, bool& quit) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit = true;
        }
    }

    SDL_SetRenderDrawColor(sdlRenderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(sdlRenderer);

    SDL_RenderPresent(sdlRenderer);
}