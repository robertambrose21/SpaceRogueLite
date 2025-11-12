#include "servermessagehandler.h"
#include "actorspawner.h"

using namespace SpaceRogueLite;

ServerMessageHandler::ServerMessageHandler(entt::dispatcher& dispatcher) : dispatcher(dispatcher) {}

void ServerMessageHandler::processMessage(int clientIndex, MessageChannel channel, Message* message) {
    spdlog::debug("Received '{}' message from client {} on channel {}", message->getName(), clientIndex,
                  MessageChannelToString(channel));

    switch (message->GetType()) {
        case (int) MessageType::PING:
            break;

        case (int) MessageType::SPAWN_ACTOR: {
            auto* spawnMessage = static_cast<SpawnActorMessage*>(message);
            handleSpawnActorMessage(clientIndex, spawnMessage);
            break;
        }

        default:
            spdlog::warn("Unknown message type: {}", message->GetType());
            break;
    }
}

void ServerMessageHandler::handleSpawnActorMessage(int clientIndex, SpawnActorMessage* message) {
    spdlog::trace("Client {} requested spawn of actor '{}'", clientIndex, message->actorName);

    dispatcher.trigger<ActorSpawnEvent>({std::string(message->actorName)});
}
