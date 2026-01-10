#include <spdlog/spdlog.h>

#include "clientgame.h"

int main() {
#if !defined(NDEBUG)
    spdlog::set_level(spdlog::level::trace);
    // yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);
#endif

    ClientGame game;
    game.run();

    return 0;
}
