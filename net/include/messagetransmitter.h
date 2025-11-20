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
            // clang-format off
#define PARSE_MESSAGE(name, messageClass)                                                      \
            case MessageType::name:                                                            \
                parseAndSend<messageClass>(message, clientIndex, std::forward<Args>(args)...); \
                return;
            MESSAGE_LIST(PARSE_MESSAGE)
#undef PARSE_MESSAGE
            // clang-format on
            default:
                spdlog::error("Unknown message type in sendMessage");
                return;
        }
    }

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

        if (message->parseFromCommand(args)) {
            doSendMessage(message, clientIndex);
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
        auto* typedMessage = static_cast<MessageClass*>(message);

        // Only call parse if it's valid for these argument types
        if constexpr (requires { typedMessage->parse(std::forward<Args>(args)...); }) {
            if (typedMessage->parse(std::forward<Args>(args)...)) {
                doSendMessage(typedMessage, clientIndex);
            }
        } else {
            spdlog::critical(
                "Message '{}' does not have a parse() overload for {} argument(s) of the provided types. "
                "Not sending message.",
                typedMessage->getName(), sizeof...(args));
            return;
        }
    }
};

}  // namespace SpaceRogueLite
