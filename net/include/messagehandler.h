#pragma once

#include "connectionconfig.h"
#include "message.h"

namespace SpaceRogueLite {

/**
 * @brief Abstract base class for handling network messages
 *
 * This class defines the interface for processing incoming network messages.
 * Concrete implementations should handle message routing and processing logic.
 */
class MessageHandler {
public:
    virtual ~MessageHandler() = default;

    /**
     * @brief Process an incoming network message
     *
     * @param clientIndex The index of the client that sent the message
     * @param channel The channel the message was received on (RELIABLE/UNRELIABLE)
     * @param message The message to process
     */
    virtual void processMessage(int clientIndex, MessageChannel channel, Message* message) = 0;
};

}  // namespace SpaceRogueLite
