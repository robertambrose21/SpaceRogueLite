#include "game.h"

using namespace SpaceRogueLite;

Game::Game() {}
Game::~Game() {}

void Game::run(void) { loop(); }

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

void Game::attachWorker(const Worker& worker) {
    if (workers.contains(worker.id)) {
        spdlog::warn("Worker {} with id {} already attached, skipping", worker.name, worker.id);
        return;
    }

    spdlog::info("Attaching worker {} with id {}", worker.name, worker.id);
    workers[worker.id] = worker;
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
