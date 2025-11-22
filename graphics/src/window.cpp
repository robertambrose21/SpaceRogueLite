#include "window.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui.h"

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

    if (!initializeImgui()) {
        return false;
    }

    return true;
}

bool Window::initializeImgui(void) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(sdlWindow, sdlRenderer);
    ImGui_ImplSDLRenderer3_Init(sdlRenderer);

    return true;
}

void Window::close(void) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

void Window::update(int64_t timeSinceLastFrame, bool& quit) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            quit = true;
        }
    }

    updateUI(timeSinceLastFrame, quit);

    SDL_SetRenderDrawColor(sdlRenderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(sdlRenderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdlRenderer);
    SDL_RenderPresent(sdlRenderer);
}

void Window::updateUI(int64_t timeSinceLastFrame, bool& quit) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();
}