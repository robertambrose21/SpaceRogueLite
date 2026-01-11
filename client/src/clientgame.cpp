#include "clientgame.h"

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <entt/entt.hpp>

#include <actorspawner.h>
#include <components.h>
#include <console.h>
#include <generation/wfc/wfctileset.h>
#include <grid.h>
#include <inputhandler.h>
#include <rendercomponents.h>
#include <renderlayers/entities/entityrendersystem.h>
#include <renderlayers/tiles/tilerenderer.h>
#include <window.h>

#include "input/camerainput.h"
#include "message.h"
#include "net/clientmessagehandler.h"
#include "net/clientmessagetransmitter.h"
#include "net/commandparser.h"

void ClientGame::onLoad(void) {
    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return;
    }

    spdlog::info("Yojimbo initialized successfully.");

    messageHandler = std::make_unique<SpaceRogueLite::ClientMessageHandler>(dispatcher);
    SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);

    client = std::make_unique<SpaceRogueLite::Client>(1, yojimbo::Address("127.0.0.1", 8081),
                                                      *messageHandler);
    messageTransmitter = std::make_unique<SpaceRogueLite::ClientMessageTransmitter>(*client);

    entt::locator<SpaceRogueLite::Grid>::emplace(128, 128);
    entt::locator<SpaceRogueLite::InputHandler>::emplace();

    window = std::make_unique<SpaceRogueLite::Window>("SpaceRogueLite Client", 1920, 1080);
    window->initialize();

    auto consoleSink = std::make_shared<SpaceRogueLite::ConsoleSinkMt>(window->getConsole());
    spdlog::default_logger()->sinks().push_back(consoleSink);

    window->getConsole()->setCommandCallback([this](const std::string& command) {
        auto parsed = SpaceRogueLite::CommandParser::parse(command);
        if (parsed.has_value()) {
            messageTransmitter->sendMessageFromCommand(parsed->messageType, parsed->arguments);
        }
    });

    cameraInput = std::make_unique<SpaceRogueLite::CameraInput>(window->getCamera(), 10);

    entt::locator<SpaceRogueLite::InputHandler>::value().attachWorker(cameraInput->getWorker());

    auto tileRenderer = window->createRenderLayer<SpaceRogueLite::TileRenderer>();

    window->getTextureLoader()->loadTextureDefinitions("../../../assets/textures.json",
                                                       "../../../assets/raw_textures");

    SpaceRogueLite::WFCTileSet tileSet("../../../assets/tilesets/grass_and_rocks/rules.json");
    tileSet.load();

    tileRenderer->loadTileVariantsIntoAtlas(tileSet.getTileVariants());

    window->createRenderLayer<SpaceRogueLite::EntityRenderSystem>(registry);

    // Create a test entity with a spaceworm sprite
    auto testEntity = registry.create();
    registry.emplace<SpaceRogueLite::Position>(testEntity, 1900, 100);
    registry.emplace<SpaceRogueLite::Renderable>(testEntity, glm::vec2(32.0f, 32.0f),
                                                 glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), "SpaceWorm");

    attachWorker("ClientUpdateLoop", [this](int64_t timeSinceLastFrame, bool& quit) {
        client->update(timeSinceLastFrame);
    });

    attachWorker("RenderLoop", [this](int64_t timeSinceLastFrame, bool& quit) {
        window->update(timeSinceLastFrame, quit);
    });

    attachWorker("CameraMovement", [this](int64_t timeSinceLastFrame, bool& quit) {
        cameraInput->update(timeSinceLastFrame, quit);
    });

    client->connect();

    // Send a test spawn message
    messageTransmitter->sendMessage(SpaceRogueLite::MessageType::SPAWN_ACTOR, "Enemy5");

    spdlog::info("ClientGame loaded successfully");
}

void ClientGame::onUnload(void) {
    client->disconnect();
    messageTransmitter.reset();
    client.reset();
    messageHandler.reset();
    ShutdownYojimbo();
}