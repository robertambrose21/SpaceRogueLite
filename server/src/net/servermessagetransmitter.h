#pragma once

#include <utility>
#include "messagetransmitter.h"
#include "server.h"

namespace SpaceRogueLite {

class ServerMessageTransmitter : public MessageTransmitter {
public:
    explicit ServerMessageTransmitter(Server& server);
    ~ServerMessageTransmitter() override = default;

protected:
    Message* createMessage(MessageType type, int clientIndex) override;
    void doSendMessage(Message* message, int clientIndex) override;

private:
    Server& server;
};

}  // namespace SpaceRogueLite
