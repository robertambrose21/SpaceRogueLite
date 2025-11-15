#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <cstring>
#include <entt/entt.hpp>

#include "actorspawner.h"
#include "game.h"
#include "message.h"
#include "messagefactory.h"
#include "net/client.h"
#include "net/clientmessagehandler.h"

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

    entt::registry registry;
    entt::dispatcher dispatcher;

    SpaceRogueLite::ClientMessageHandler messageHandler(dispatcher);
    SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);

    SpaceRogueLite::Game game;
    SpaceRogueLite::Client client(1, yojimbo::Address("127.0.0.1", 8081), messageHandler);

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

    // Send a test spawn message
    auto spawnMessage = client.createMessage(SpaceRogueLite::MessageType::SPAWN_ACTOR);
    auto* spawnActorMessage = static_cast<SpaceRogueLite::SpawnActorMessage*>(spawnMessage);
    spawnActorMessage->parseFromCommand({"Enemy2"});
    client.sendMessage(spawnMessage);

    game.run();

    client.disconnect();

    ShutdownYojimbo();

    return 0;
}
