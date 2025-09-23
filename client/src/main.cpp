#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include "game.h"
#include "message.h"
#include "net/client.h"

int main() {
#if !defined(NDEBUG)
    spdlog::set_level(spdlog::level::trace);
    // yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);
#endif

    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return 1;
    }

    spdlog::info("Yojimbo initialized successfully.");

    SpaceRogueLite::Game game;
    SpaceRogueLite::Client client(1, yojimbo::Address("127.0.0.1", 8081));

    game.attachWorker({1, "ClientUpdateLoop",
                       [&client](int64_t timeSinceLastFrame, bool& quit) { client.update(timeSinceLastFrame); }});

    int64_t ticks = 0;
    int64_t delay = 1000;
    auto workerFunc = [&](int64_t timeSinceLastFrame, bool& quit) {
        ticks += timeSinceLastFrame;

        if (ticks < delay) {
            return;
        }

        auto message = client.createMessage(SpaceRogueLite::MessageType::PING);
        client.sendMessage(message);

        ticks = 0;
    };

    game.attachWorker({2, "PingTest", workerFunc});

    client.connect();

    game.run();

    client.disconnect();

    ShutdownYojimbo();

    return 0;
}
