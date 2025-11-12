#pragma once

#include <spdlog/spdlog.h>
#include <entt/entt.hpp>

#include "messagefactory.h"
#include "messagehandler.h"

namespace SpaceRogueLite {

/**
 * @brief Server-side implementation of MessageHandler
 *
 * Processes incoming messages from clients and dispatches events to the game logic
 * via an entt::dispatcher.
 */
class ServerMessageHandler : public MessageHandler {
public:
    /**
     * @brief Construct a new Server Message Handler
     *
     * @param dispatcher Reference to the event dispatcher for triggering game events
     */
    explicit ServerMessageHandler(entt::dispatcher& dispatcher);
    ~ServerMessageHandler() override = default;

    /**
     * @brief Process an incoming network message
     *
     * Routes messages to appropriate handlers based on message type.
     *
     * @param clientIndex The index of the client that sent the message
     * @param channel The channel the message was received on (RELIABLE/UNRELIABLE)
     * @param message The message to process
     */
    void processMessage(int clientIndex, MessageChannel channel, Message* message) override;

private:
    entt::dispatcher& dispatcher;

    /**
     * @brief Handle a spawn actor message from a client
     *
     * @param clientIndex The index of the client that sent the message
     * @param message The spawn actor message to handle
     */
    void handleSpawnActorMessage(int clientIndex, SpawnActorMessage* message);
};

}  // namespace SpaceRogueLite
