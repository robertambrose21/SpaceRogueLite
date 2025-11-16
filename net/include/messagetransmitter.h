#pragma once

#include <spdlog/spdlog.h>
#include <utility>
#include "messagefactory.h"

namespace SpaceRogueLite {

/**
 * Base class for message transmitters that handle creating, parsing, and sending messages.
 * This class provides a generic interface that works for both Client and Server endpoints.
 *
 * Subclasses must implement:
 * - createMessage(): Create a message of the specified type for the given client
 * - doSendMessage(): Send the message to the appropriate destination
 */
class MessageTransmitter {
public:
    virtual ~MessageTransmitter() = default;

public:
    /**
     * Send a message to a specific client with the specified type and arguments from a command.
     * For Server usage, clientIndex specifies the target client.
     * For Client usage, clientIndex is ignored (typically passed as 0).
     *
     * @param clientIndex The target client index (ignored for Client implementations)
     * @param type The message type to send
     * @param args Vector of string arguments to pass to the message's parse() method
     */
    void sendMessageFromCommand(int clientIndex, MessageType type, const std::vector<std::string>& args) {
        auto message = createMessage(type, clientIndex);
        if (!message) {
            spdlog::error("Failed to create message for type {}", static_cast<int>(type));
            return;
        }

        message->parseFromCommand(args);

        doSendMessage(message, clientIndex);
    }

protected:
    /**
     * Create a message of the specified type.
     * For Client implementations, clientIndex is ignored.
     * For Server implementations, clientIndex specifies which client to create the message for.
     */
    virtual Message* createMessage(MessageType type, int clientIndex) = 0;

    /**
     * Send the message to the appropriate destination.
     * For Client implementations, clientIndex is ignored and the message is sent to the server.
     * For Server implementations, clientIndex specifies which client to send the message to.
     */
    virtual void doSendMessage(Message* message, int clientIndex) = 0;
};

}  // namespace SpaceRogueLite
