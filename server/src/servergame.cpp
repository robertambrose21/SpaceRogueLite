#include "servergame.h"

#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include <generation/wfc/wfcstrategy.h>
#include <generation/wfc/wfctileset.h>
#include <grid.h>

#include "actorspawner.h"

void ServerGame::onLoad(void) {
    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return;
    }

    spdlog::info("Yojimbo initialized successfully.");

    entt::locator<SpaceRogueLite::Grid>::emplace(128, 128);

    SpaceRogueLite::WFCTileSet tileSet("../../../assets/tilesets/grass_and_rocks/rules.json");
    tileSet.load();

    SpaceRogueLite::WFCStrategy wfcStrategy({2, glm::ivec2(2, 2), glm::ivec2(6, 6), 0}, tileSet);
    auto generatedMap = wfcStrategy.generate();

    auto& grid = entt::locator<SpaceRogueLite::Grid>::value();
    grid.setTiles(generatedMap, wfcStrategy.getWidth(), wfcStrategy.getHeight());

    spdlog::info("Generated map: {}x{}", grid.getWidth(), grid.getHeight());

    messageHandler = std::make_unique<SpaceRogueLite::ServerMessageHandler>(dispatcher);
    server = std::make_unique<SpaceRogueLite::Server>(yojimbo::Address("127.0.0.1", 8081), 64,
                                                      *messageHandler);

    attachWorker({1, "ServerUpdateLoop", [this](int64_t timeSinceLastFrame, bool& quit) {
                      server->update(timeSinceLastFrame);
                  }});

    server->start();

    SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);
    SpaceRogueLite::ActorSystem actorSystem(registry, dispatcher);

    auto player = spawner.spawnActor("Player");
    auto enemy = spawner.spawnActor("Enemy");

    actorSystem.applyDamage(enemy, 50);
    actorSystem.applyDamage(enemy, 60);  // This should trigger despawn

    spdlog::info("ServerGame loaded successfully.");
}

void ServerGame::onUnload(void) {
    server->stop();
    server.reset();
    messageHandler.reset();
    ShutdownYojimbo();
}