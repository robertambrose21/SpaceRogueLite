#pragma once

#include <spdlog/spdlog.h>
#include <entt/entt.hpp>

#include "actorspawner.h"
#include "handlerregistry.h"
#include "messagefactory.h"
#include "messagehandler.h"

namespace SpaceRogueLite {

/**
 * @brief Client-side implementation of MessageHandler
 *
 * Processes incoming messages from the server and dispatches events to the game logic
 * via an entt::dispatcher.
 */
class ClientMessageHandler : public MessageHandler {
public:
    /**
     * @brief Construct a new Client Message Handler
     *
     * @param dispatcher Reference to the event dispatcher for triggering game events
     */
    explicit ClientMessageHandler(entt::dispatcher& dispatcher);
    ~ClientMessageHandler() override = default;

    /**
     * @brief Process an incoming network message
     *
     * Routes messages to appropriate handlers based on message type.
     *
     * @param clientIndex The index of the connection (unused for client, always 0)
     * @param channel The channel the message was received on (RELIABLE/UNRELIABLE)
     * @param message The message to process
     */
    void processMessage(int clientIndex, MessageChannel channel, Message* message) override;

    /**
     * @brief Template handler method for type-safe message processing
     *
     * This method is specialized for each message type. The specializations are defined
     * below the class declaration.
     *
     * @tparam MessageClass The concrete message class to handle
     * @param message The typed message to handle
     */
    template <typename MessageClass>
    void handleMessage(MessageClass* message);

    /**
     * @brief Get the event dispatcher
     *
     * Public accessor for the dispatcher, needed by template handlers.
     *
     * @return Reference to the entt::dispatcher
     */
    entt::dispatcher& getDispatcher() { return dispatcher; }

private:
    entt::dispatcher& dispatcher;
};

template <>
inline void ClientMessageHandler::handleMessage<PingMessage>(PingMessage* message) {}

template <>
inline void ClientMessageHandler::handleMessage<SpawnActorMessage>(SpawnActorMessage* message) {
    dispatcher.trigger<ActorSpawnEvent>({std::string(message->actorName)});
}

/**
 * Creates a type-safe handler function for a specific message type
 *
 * This template function generates a handler that:
 * 1. Casts the generic Message* to the correct MessageClass*
 * 2. Calls the appropriate template specialization of handleMessage<MessageClass>
 *
 * @tparam MessageClass The concrete message class (e.g., PingMessage, SpawnActorMessage)
 * @return Function pointer that can be stored in the handler registry
 */
template <typename MessageClass>
constexpr auto makeHandler() {
    return +[](ClientMessageHandler* handler, int clientIndex, Message* message) {
        auto* typedMessage = static_cast<MessageClass*>(message);
        handler->template handleMessage<MessageClass>(typedMessage);
    };
}

/**
 * Initializes the handler registry with all message handlers
 *
 * This constexpr function is evaluated at compile time to create the registry.
 * Uses MESSAGE_LIST from messagefactory.h to automatically register all message types.
 *
 * @return Initialized HandlerRegistry
 */
constexpr auto initializeHandlerRegistry() {
    HandlerRegistry<ClientMessageHandler> registry;

    // Generate handler registrations from MESSAGE_LIST
#define MESSAGE_HANDLER_REGISTER(name, messageClass) \
    registry.registerHandler(MessageType::name, makeHandler<messageClass>());
    MESSAGE_LIST(MESSAGE_HANDLER_REGISTER)
#undef MESSAGE_HANDLER_REGISTER

    return registry;
}

/**
 * Global constexpr handler registry instance
 *
 * This is constructed at compile time and embedded directly in the binary.
 * No runtime initialization overhead - the registry is ready to use immediately.
 */
inline constexpr auto CLIENT_HANDLER_REGISTRY = initializeHandlerRegistry();

}  // namespace SpaceRogueLite
