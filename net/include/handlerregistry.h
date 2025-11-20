#pragma once

#include "messagefactory.h"
#include <array>

namespace SpaceRogueLite {

/**
 * Function pointer type for message handlers
 *
 * Template parameter HandlerType allows this to work with any MessageHandler implementation
 * (e.g., ServerMessageHandler, ClientMessageHandler)
 *
 * @tparam HandlerType The concrete message handler class type
 * @param handler Pointer to the MessageHandler instance
 * @param clientIndex Index of the client/connection that sent the message
 * @param message Pointer to the message (will be cast to appropriate type by handler)
 */
template <typename HandlerType>
using HandlerFunc = void (*)(HandlerType*, int, Message*);

/**
 * Compile-time handler lookup table
 *
 * This class creates an array that maps MessageType enum values to handler functions.
 * The template parameter allows the same registry structure to be used by both
 * client and server with their respective handler types.
 *
 * Usage:
 *   HandlerFunc<ServerMessageHandler> handler = getHandler(MessageType::PING);
 *   if (handler) {
 *       handler(this, clientIndex, message);
 *   }
 *
 * @tparam HandlerType The concrete message handler class (ServerMessageHandler, ClientMessageHandler, etc.)
 */
template <typename HandlerType>
class HandlerRegistry {
private:
    static constexpr size_t MESSAGE_COUNT = static_cast<size_t>(MessageType::COUNT);
    std::array<HandlerFunc<HandlerType>, MESSAGE_COUNT> handlers{};

public:
    /**
     * Default constructor - handlers will be registered via registerHandler()
     */
    constexpr HandlerRegistry() : handlers{} {}

    /**
     * Registers a handler for a specific message type
     *
     * @param type The message type to register
     * @param handler The handler function pointer
     */
    constexpr void registerHandler(MessageType type, HandlerFunc<HandlerType> handler) {
        handlers[static_cast<size_t>(type)] = handler;
    }

    /**
     * Retrieves the handler function for a given message type
     *
     * @param type The message type to look up
     * @return Function pointer to the handler, or nullptr if type is invalid
     */
    constexpr HandlerFunc<HandlerType> getHandler(MessageType type) const {
        size_t index = static_cast<size_t>(type);
        if (index >= MESSAGE_COUNT) {
            return nullptr;
        }
        return handlers[index];
    }
};

}  // namespace SpaceRogueLite
