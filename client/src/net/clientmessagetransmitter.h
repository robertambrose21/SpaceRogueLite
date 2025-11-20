#pragma once

#include <utility>
#include "client.h"
#include "messagetransmitter.h"

namespace SpaceRogueLite {

class ClientMessageTransmitter : public MessageTransmitter {
public:
    explicit ClientMessageTransmitter(Client& client);
    ~ClientMessageTransmitter() override = default;

public:
    template <typename... Args>
    void sendMessage(MessageType type, Args&&... args) {
        MessageTransmitter::sendMessage(0, type, std::forward<Args>(args)...);
    }

    /**
     * Client-specific convenience overload that doesn't require clientIndex.
     * Forwards to base class with clientIndex=0 (ignored by client implementation).
     *
     * @param type The message type to send
     * @param args Vector of string arguments to pass to the message's parse() method
     */
    void sendMessageFromCommand(MessageType type, const std::vector<std::string>& args) {
        MessageTransmitter::sendMessageFromCommand(0, type, args);
    }

protected:
    Message* createMessage(MessageType type, int clientIndex) override;
    void doSendMessage(Message* message, int clientIndex) override;

private:
    Client& client;
};

}  // namespace SpaceRogueLite