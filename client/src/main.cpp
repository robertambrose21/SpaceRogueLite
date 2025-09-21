#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include "game.h"
#include "net/client.h"

int main() {
    // yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);

    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return 1;
    }

    spdlog::info("Yojimbo initialized successfully.");

    SpaceRogueLite::Game game;
    SpaceRogueLite::Client client(1, yojimbo::Address("127.0.0.1", 8081));

    game.attachWorker({1, "ClientUpdateLoop",
                       [&client](int64_t timeSinceLastFrame, bool& quit) { client.update(timeSinceLastFrame); }});

    client.connect();

    game.run();

    client.disconnect();

    ShutdownYojimbo();

    return 0;
}
