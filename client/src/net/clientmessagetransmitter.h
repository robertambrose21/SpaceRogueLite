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
    /**
     * Client-specific convenience overload that doesn't require clientIndex.
     * Forwards to base class with clientIndex=0 (ignored by client implementation).
     */
    template <typename... Args>
    void sendMessage(MessageType type, Args&&... args) {
        MessageTransmitter::sendMessage(0, type, std::forward<Args>(args)...);
    }

protected:
    Message* createMessage(MessageType type, int clientIndex) override;
    void doSendMessage(Message* message, int clientIndex) override;

private:
    Client& client;
};

}  // namespace SpaceRogueLite