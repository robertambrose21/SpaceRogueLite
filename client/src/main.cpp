#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <cstring>
#include <entt/entt.hpp>

#include <actorspawner.h>
#include <game.h>
#include <tileatlas.h>
#include <tilemap.h>
#include <tilerenderer.h>
#include <window.h>
#include "message.h"
#include "messagefactory.h"
#include "net/client.h"
#include "net/clientmessagehandler.h"
#include "net/clientmessagetransmitter.h"
#include "net/inputcommandhandler.h"

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

    {
        entt::registry registry;
        entt::dispatcher dispatcher;

        SpaceRogueLite::ClientMessageHandler messageHandler(dispatcher);
        SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);

        SpaceRogueLite::Game game;
        SpaceRogueLite::Client client(1, yojimbo::Address("127.0.0.1", 8081), messageHandler);
        SpaceRogueLite::ClientMessageTransmitter messageTransmitter(client);
        SpaceRogueLite::InputCommandHandler inputHandler(messageTransmitter);

        SpaceRogueLite::Window window("SpaceRogueLite Client", 1920, 1080);
        window.initialize();

        // Test tile rendering
        auto tileRenderer = window.getTileRenderer();
        tileRenderer->loadAtlasTiles(
            {"../../../assets/floor1.png", "../../../assets/rusty_metal.png"});

        // Create a test tile map (10x8 tiles)
        auto tileMap = std::make_unique<SpaceRogueLite::TileMap>(10, 8);
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 10; ++x) {
                tileMap->setTile(x, y, (x + y) % 2 == 0 ? 1 : 2);
            }
        }

        tileRenderer->setTileMap(std::move(tileMap));

        game.attachWorker(
            {1, "ClientUpdateLoop", [&client](int64_t timeSinceLastFrame, bool& quit) {
                 client.update(timeSinceLastFrame);
             }});

        game.attachWorker({2, "RenderLoop", [&window](int64_t timeSinceLastFrame, bool& quit) {
                               window.update(timeSinceLastFrame, quit);
                           }});

        game.attachWorker(
            {3, "InputHandler", [&inputHandler](int64_t timeSinceLastFrame, bool& quit) {
                 inputHandler.processCommands(timeSinceLastFrame);
             }});

        client.connect();

        // Send a test spawn message
        messageTransmitter.sendMessage(SpaceRogueLite::MessageType::SPAWN_ACTOR, "Enemy5");

        game.run();

        client.disconnect();
    }

    ShutdownYojimbo();

    return 0;
}
