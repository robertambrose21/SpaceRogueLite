#include "servergame.h"

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <algorithm>
#include <cstring>

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

    server->setOnClientConnectedCallback([this](int clientIndex) { sendMapToClient(clientIndex); });

    attachWorker("ServerUpdateLoop", [this](int64_t timeSinceLastFrame, bool& quit) {
        server->update(timeSinceLastFrame);
    });

    server->start();

    spawner = std::make_unique<SpaceRogueLite::ActorSpawner>(registry, dispatcher);
    actorSystem = std::make_unique<SpaceRogueLite::ActorSystem>(registry, dispatcher);

    auto player = spawner->spawnActor("Player");
    auto enemy = spawner->spawnActor("Enemy");

    actorSystem->applyDamage(enemy, 50);
    actorSystem->applyDamage(enemy, 60);  // This should trigger despawn

    spdlog::info("ServerGame loaded successfully.");
}

void ServerGame::onUnload(void) {
    server->stop();
    server.reset();
    messageHandler.reset();
    spawner.reset();
    actorSystem.reset();
    ShutdownYojimbo();
}

void ServerGame::sendMapToClient(int clientIndex) {
    auto& grid = entt::locator<SpaceRogueLite::Grid>::value();

    constexpr int CHUNK_SIZE = 16;
    uint16_t sequenceNumber = 0;
    uint16_t numSequences = (grid.getWidth() / CHUNK_SIZE) * (grid.getHeight() / CHUNK_SIZE);

    for (int chunkY = 0; chunkY < grid.getHeight(); chunkY += CHUNK_SIZE) {
        for (int chunkX = 0; chunkX < grid.getWidth(); chunkX += CHUNK_SIZE) {
            auto* msg = static_cast<SpaceRogueLite::LoadMapChunkMessage*>(
                server->createMessage(clientIndex, SpaceRogueLite::MessageType::LOAD_MAP_CHUNK));

            msg->chunk.posX = chunkX;
            msg->chunk.posY = chunkY;
            msg->chunk.width = std::min(CHUNK_SIZE, grid.getWidth() - chunkX);
            msg->chunk.height = std::min(CHUNK_SIZE, grid.getHeight() - chunkY);
            msg->sequenceNumber = ++sequenceNumber;
            msg->numSequences = numSequences;

            for (int y = 0; y < msg->chunk.height; y++) {
                for (int x = 0; x < msg->chunk.width; x++) {
                    auto gridTile = grid.getTile(chunkX + x, chunkY + y);
                    auto& chunkTile = msg->chunk.tiles[y * msg->chunk.width + x];

                    chunkTile.id = gridTile.id;
                    chunkTile.orientation = gridTile.orientation;
                    chunkTile.walkability = static_cast<uint8_t>(gridTile.walkable);
                    std::strncpy(chunkTile.type, gridTile.type.c_str(), 63);
                    chunkTile.type[63] = '\0';
                }
            }

            server->sendMessage(clientIndex, msg);
        }
    }

    spdlog::info("Sent map ({} chunks) to client {}",
                 (grid.getWidth() / CHUNK_SIZE) * (grid.getHeight() / CHUNK_SIZE), clientIndex);
}