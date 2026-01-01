#include "inputhandler.h"

using namespace SpaceRogueLite;

void InputHandler::handleEvent(const SDL_Event& event) {
    for (const auto& worker : workers) {
        worker.second.function(event);
    }
}

void InputHandler::attachWorker(const InputWorker& worker) {
    if (workers.contains(worker.id)) {
        spdlog::warn("Input worker {} with id {} already attached, skipping", worker.name,
                     worker.id);
        return;
    }

    spdlog::info("Attaching input worker {} with id {}", worker.name, worker.id);
    workers[worker.id] = worker;
}

void InputHandler::detachWorker(uint32_t id) {
    if (!workers.contains(id)) {
        spdlog::warn("Input worker with id {} not found, cannot detach", id);
        return;
    }

    spdlog::info("Detaching input worker {} with id {}", workers[id].name, id);
    workers.erase(id);
}

const std::unordered_map<uint32_t, InputHandler::InputWorker>& InputHandler::getWorkers() const {
    return workers;
}
