#pragma once

#include <spdlog/spdlog.h>
#include <entt/entt.hpp>
#include <map>
#include <string>

#include "utils/timing.h"

namespace SpaceRogueLite {

class Game {
public:
    typedef struct _worker {
        uint32_t id;
        std::string name;
        std::function<void(int64_t, bool&)> function;
    } Worker;

    Game();
    ~Game();

    void run(void);

    virtual void onLoad(void) = 0;
    virtual void onUnload(void) = 0;

    uint32_t attachWorker(const std::string& name, std::function<void(int64_t, bool&)> function);
    void detachWorker(uint32_t id);
    const std::map<uint32_t, Worker>& getWorkers(void) const;

private:
    bool isLoaded;
    uint32_t nextWorkerId = 0;
    std::map<uint32_t, Worker> workers;

    void loop(void);
};

}  // namespace SpaceRogueLite
