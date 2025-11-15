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
     * Send a message to a specific client with the specified type and arguments.
     * For Server usage, clientIndex specifies the target client.
     * For Client usage, clientIndex is ignored (typically passed as 0).
     */
    template <typename... Args>
    void sendMessage(int clientIndex, MessageType type, Args&&... args) {
        auto message = createMessage(type, clientIndex);

        // Cast to correct message type and call parse with proper type information
        switch (type) {
#define PARSE_MESSAGE(name, messageClass)                                        \
    case MessageType::name:                                                      \
        parseAndSend<messageClass>(message, clientIndex, std::forward<Args>(args)...); \
        return;
            MESSAGE_LIST(PARSE_MESSAGE)
#undef PARSE_MESSAGE
            default:
                spdlog::error("Unknown message type in sendMessage");
                return;
        }
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

private:
    // Helper template to parse message with correct type
    template <typename MessageClass, typename... Args>
    void parseAndSend(Message* message, int clientIndex, Args&&... args) {
        auto* typedMsg = static_cast<MessageClass*>(message);

        // Only call parse if it's valid for these argument types
        if constexpr (requires { typedMsg->parse(std::forward<Args>(args)...); }) {
            typedMsg->parse(std::forward<Args>(args)...);
        } else {
            spdlog::critical(
                "Message '{}' does not have a parse() overload for {} argument(s) of the provided types. "
                "Not sending message.",
                typedMsg->getName(), sizeof...(args));
            return;
        }

        doSendMessage(message, clientIndex);
    }
};

}  // namespace SpaceRogueLite
