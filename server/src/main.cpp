#include <spdlog/spdlog.h>

#include "servergame.h"

int main() {
#if !defined(NDEBUG)
    spdlog::set_level(spdlog::level::trace);
#endif

    ServerGame game;
    game.run();

    return 0;
}
