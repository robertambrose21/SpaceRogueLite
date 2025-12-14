#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <cstring>
#include <entt/entt.hpp>

#include <actorspawner.h>
#include <components.h>
#include <game.h>
#include <grid.h>
#include <rendercomponents.h>
#include <renderlayers/entities/entityrendersystem.h>
#include <renderlayers/tiles/tileatlas.h>
#include <renderlayers/tiles/tilerenderer.h>
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
        // TODO: Remove this eventually
        SpaceRogueLite::InputCommandHandler inputHandler(messageTransmitter);

        SpaceRogueLite::Window window("SpaceRogueLite Client", 1920, 1080);
        window.initialize();

        auto tileRenderer = window.createRenderLayer<SpaceRogueLite::TileRenderer>();

        // Load texture definitions first (needed for loadTilesFromRules)
        window.getTextureLoader()->loadTextureDefinitions("../../../assets/textures.json",
                                                          "../../../assets/raw_textures");

        tileRenderer->loadTilesFromRules("../../../assets/tilesets/grass_and_rocks/rules.json");

        // Create a test grid (58x32 tiles)
        entt::locator<SpaceRogueLite::Grid>::emplace(58, 32);
        auto& grid = entt::locator<SpaceRogueLite::Grid>::value();

        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 58; ++x) {
                grid.setTile(x, y, SpaceRogueLite::GridTile{static_cast<SpaceRogueLite::TileId>((x + y) % 2 == 0 ? 1 : 2)});
            }
        }

        window.createRenderLayer<SpaceRogueLite::EntityRenderSystem>(registry);

        // Create a test entity with a spaceworm sprite
        auto testEntity = registry.create();
        registry.emplace<SpaceRogueLite::Position>(testEntity, 100, 100);
        registry.emplace<SpaceRogueLite::Renderable>(
            testEntity, glm::vec2(32.0f, 32.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), "SpaceWorm");

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
