#include "servermessagehandler.h"

using namespace SpaceRogueLite;

ServerMessageHandler::ServerMessageHandler(entt::dispatcher& dispatcher) : dispatcher(dispatcher) {}

void ServerMessageHandler::processMessage(int clientIndex, MessageChannel channel, Message* message) {
    spdlog::debug("Received '{}' message from client {} on channel {}", message->getName(), clientIndex,
                  MessageChannelToString(channel));

    MessageType type = static_cast<MessageType>(message->GetType());

    if (auto handler = HANDLER_REGISTRY.getHandler(type)) {
        handler(this, clientIndex, message);
    } else {
        spdlog::warn("Unknown message type: {}", message->GetType());
    }
}
