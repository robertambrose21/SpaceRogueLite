#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <iostream>

#include "actorspawner.h"
#include "game.h"
#include "net/server.h"
#include "net/servermessagehandler.h"

#include <grid.h>
#include <generation/wfc/wfcstrategy.h>
#include <generation/wfc/wfctileset.h>

struct Position {
    float x;
    float y;
};

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

    entt::locator<SpaceRogueLite::Grid>::emplace(128, 128);

    SpaceRogueLite::WFCTileSet tileSet("../../../assets/tilesets/grass_and_rocks/rules.json");
    tileSet.load();

    SpaceRogueLite::WFCStrategy wfcStrategy({2, glm::ivec2(2, 2), glm::ivec2(6, 6), 0}, tileSet);
    auto generatedMap = wfcStrategy.generate();

    auto& grid = entt::locator<SpaceRogueLite::Grid>::value();
    grid.setTiles(generatedMap, wfcStrategy.getWidth(), wfcStrategy.getHeight());

    spdlog::info("Generated map: {}x{}", grid.getWidth(), grid.getHeight());

    SpaceRogueLite::Game game;
    SpaceRogueLite::ServerMessageHandler messageHandler(dispatcher);
    SpaceRogueLite::Server server(yojimbo::Address("127.0.0.1", 8081), 64, messageHandler);

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
