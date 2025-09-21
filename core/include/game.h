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

    void attachWorker(const Worker& worker);
    void detachWorker(uint32_t id);
    const std::map<uint32_t, Worker>& getWorkers(void) const;

private:
    std::map<uint32_t, Worker> workers;

    void loop(void);
};

}  // namespace SpaceRogueLite
