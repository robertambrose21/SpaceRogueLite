#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <cstring>
#include <entt/entt.hpp>

#include <actorspawner.h>
#include <components.h>
#include <game.h>
#include <generation/wfc/wfcstrategy.h>
#include <generation/wfc/wfctileset.h>
#include <grid.h>
#include <inputhandler.h>
#include <console.h>
#include <rendercomponents.h>
#include <renderlayers/entities/entityrendersystem.h>
#include <renderlayers/tiles/tileatlas.h>
#include <renderlayers/tiles/tilerenderer.h>
#include <window.h>
#include "input/camerainput.h"
#include "message.h"
#include "messagefactory.h"
#include "net/client.h"
#include "net/clientmessagehandler.h"
#include "net/clientmessagetransmitter.h"
#include "net/commandparser.h"

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

        entt::locator<SpaceRogueLite::Grid>::emplace(128, 128);
        entt::locator<SpaceRogueLite::InputHandler>::emplace();

        SpaceRogueLite::Window window("SpaceRogueLite Client", 1920, 1080);
        window.initialize();

        auto consoleSink = std::make_shared<SpaceRogueLite::ConsoleSinkMt>(window.getConsole());
        spdlog::default_logger()->sinks().push_back(consoleSink);

        window.getConsole()->setCommandCallback([&messageTransmitter](const std::string& command) {
            auto parsed = SpaceRogueLite::CommandParser::parse(command);
            if (parsed.has_value()) {
                messageTransmitter.sendMessageFromCommand(parsed->messageType, parsed->arguments);
            }
        });

        SpaceRogueLite::CameraInput cameraInput(window.getCamera(), 10);

        entt::locator<SpaceRogueLite::InputHandler>::value().attachWorker(cameraInput.getWorker());

        auto tileRenderer = window.createRenderLayer<SpaceRogueLite::TileRenderer>();

        window.getTextureLoader()->loadTextureDefinitions("../../../assets/textures.json",
                                                          "../../../assets/raw_textures");

        SpaceRogueLite::WFCTileSet tileSet("../../../assets/tilesets/grass_and_rocks/rules.json");
        tileSet.load();

        tileRenderer->loadTileVariantsIntoAtlas(tileSet.getTileVariants());

        SpaceRogueLite::WFCStrategy wfcStrategy({2, glm::ivec2(2, 2), glm::ivec2(6, 6), 0},
                                                tileSet);
        auto generatedMap = wfcStrategy.generate();

        auto& grid = entt::locator<SpaceRogueLite::Grid>::value();
        grid.setTiles(generatedMap, wfcStrategy.getWidth(), wfcStrategy.getHeight());

        window.createRenderLayer<SpaceRogueLite::EntityRenderSystem>(registry);

        // Create a test entity with a spaceworm sprite
        auto testEntity = registry.create();
        registry.emplace<SpaceRogueLite::Position>(testEntity, 1900, 100);
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
            {4, "CameraMovement", [&cameraInput](int64_t timeSinceLastFrame, bool& quit) {
                 cameraInput.update(timeSinceLastFrame, quit);
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
