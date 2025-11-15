#pragma once

#include <spdlog/spdlog.h>
#include <utility>
#include "client.h"
#include "messagefactory.h"

namespace SpaceRogueLite {

class ClientMessageTransmitter {
public:
    explicit ClientMessageTransmitter(Client& client);
    ~ClientMessageTransmitter() = default;

public:
    template <typename... Args>
    void sendMessage(MessageType type, Args&&... args) {
        auto message = client.createMessage(type);

        // Cast to correct message type and call parse with proper type information
        switch (type) {
#define PARSE_MESSAGE(name, messageClass)                                 \
    case MessageType::name:                                               \
        parseAndSend<messageClass>(message, std::forward<Args>(args)...); \
        return;
            MESSAGE_LIST(PARSE_MESSAGE)
#undef PARSE_MESSAGE
            default:
                spdlog::error("Unknown message type in sendMessage");
                return;
        }
    }

private:
    Client& client;

    // Helper template to parse message with correct type
    template <typename MessageClass, typename... Args>
    void parseAndSend(Message* message, Args&&... args) {
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

        client.sendMessage(message);
    }
};

}  // namespace SpaceRogueLite