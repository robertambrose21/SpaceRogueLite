#pragma once

#include "messagefactory.h"
#include <array>

namespace SpaceRogueLite {

class ServerMessageHandler;

/**
 * Function pointer type for message handlers
 *
 * @param handler Pointer to the ServerMessageHandler instance
 * @param clientIndex Index of the client that sent the message
 * @param message Pointer to the message (will be cast to appropriate type by handler)
 */
using HandlerFunc = void (*)(ServerMessageHandler*, int, Message*);

/**
 * Compile-time handler lookup table
 *
 * This class creates an array that maps MessageType enum values to handler functions.
 * The handlers are registered after ServerMessageHandler is fully defined.
 *
 * Usage:
 *   HandlerFunc handler = getHandler(MessageType::PING);
 *   if (handler) {
 *       handler(this, clientIndex, message);
 *   }
 */
class HandlerRegistry {
private:
    static constexpr size_t MESSAGE_COUNT = static_cast<size_t>(MessageType::COUNT);
    std::array<HandlerFunc, MESSAGE_COUNT> handlers{};

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
    constexpr void registerHandler(MessageType type, HandlerFunc handler) {
        handlers[static_cast<size_t>(type)] = handler;
    }

    /**
     * Retrieves the handler function for a given message type
     *
     * @param type The message type to look up
     * @return Function pointer to the handler, or nullptr if type is invalid
     */
    constexpr HandlerFunc getHandler(MessageType type) const {
        size_t index = static_cast<size_t>(type);
        if (index >= MESSAGE_COUNT) {
            return nullptr;
        }
        return handlers[index];
    }
};

}  // namespace SpaceRogueLite
