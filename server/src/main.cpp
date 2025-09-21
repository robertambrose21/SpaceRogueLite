#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <iostream>

#include "actorspawner.h"
#include "game.h"
#include "net/server.h"

struct Position {
    float x;
    float y;
};

int main() {
    // yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);

    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return 1;
    }

    spdlog::info("Yojimbo initialized successfully.");

    entt::registry registry;
    entt::dispatcher dispatcher;

    SpaceRogueLite::Game game;
    SpaceRogueLite::Server server(yojimbo::Address("127.0.0.1", 8081), 64);

    game.attachWorker({1, "ServerUpdateLoop",
                       [&server](int64_t timeSinceLastFrame, bool& quit) { server.update(timeSinceLastFrame); }});

    server.start();

    SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);
    SpaceRogueLite::ActorSystem actorSystem(registry, dispatcher);

    auto player = spawner.spawnActor("Player");
    auto enemy = spawner.spawnActor("Enemy");

    actorSystem.applyDamage(enemy, 50);
    actorSystem.applyDamage(enemy, 60);  // This should trigger despawn
    game.run();

    // std::cout << "test" << std::endl;

    server.stop();

    ShutdownYojimbo();

    return 0;
}
