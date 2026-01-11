#include "game.h"

using namespace SpaceRogueLite;

Game::Game() : isLoaded(false) {}
Game::~Game() { isLoaded = false; }

void Game::run(void) {
    if (!isLoaded) {
        onLoad();
        isLoaded = true;
    }

    loop();

    onUnload();
    isLoaded = false;
}

void Game::loop(void) {
    int64_t currentTime = Utils::getMilliseconds();
    int64_t timeSinceLastFrame = 0;
    bool quit = false;

    while (!quit) {
        timeSinceLastFrame = Utils::getMilliseconds() - currentTime;
        currentTime = Utils::getMilliseconds();

        for (auto& [id, worker] : workers) {
            worker.function(timeSinceLastFrame, quit);
        }
    }
}

uint32_t Game::attachWorker(const std::string& name, std::function<void(int64_t, bool&)> function) {
    uint32_t id = nextWorkerId++;
    spdlog::info("Attaching worker {} with id {}", name, id);
    workers[id] = Worker{id, name, function};
    return id;
}

void Game::detachWorker(uint32_t id) {
    if (!workers.contains(id)) {
        spdlog::warn("Worker with id {} not found, cannot detach", id);
        return;
    }

    spdlog::info("Detaching worker {} with id {}", workers[id].name, id);
    workers.erase(id);
}

const std::map<uint32_t, Game::Worker>& Game::getWorkers(void) const { return workers; }
