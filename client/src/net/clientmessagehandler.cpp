#include "clientmessagehandler.h"

using namespace SpaceRogueLite;

ClientMessageHandler::ClientMessageHandler(entt::dispatcher& dispatcher) : dispatcher(dispatcher) {}

void ClientMessageHandler::processMessage(int clientIndex, MessageChannel channel, Message* message) {
    spdlog::debug("Received '{}' message from server on channel {}", message->getName(),
                  MessageChannelToString(channel));

    MessageType type = static_cast<MessageType>(message->GetType());

    if (auto handler = CLIENT_HANDLER_REGISTRY.getHandler(type)) {
        handler(this, 0, message);
    } else {
        spdlog::warn("Unknown message type: {}", message->GetType());
    }
}
