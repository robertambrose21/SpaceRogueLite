#pragma once

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <functional>
#include <string>
#include <unordered_map>

namespace SpaceRogueLite {

class InputHandler {
public:
    typedef struct _worker {
        uint32_t id;
        std::string name;
        std::function<void(const SDL_Event&)> function;
    } InputWorker;

    explicit InputHandler() = default;

    void handleEvent(const SDL_Event& event);

    void attachWorker(const InputWorker& worker);
    void detachWorker(uint32_t id);
    const std::unordered_map<uint32_t, InputWorker>& getWorkers(void) const;

private:
    std::unordered_map<uint32_t, InputWorker> workers;
};

}  // namespace SpaceRogueLite
