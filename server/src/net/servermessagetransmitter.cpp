#include "servermessagetransmitter.h"

using namespace SpaceRogueLite;

ServerMessageTransmitter::ServerMessageTransmitter(Server& server) : server(server) {}

Message* ServerMessageTransmitter::createMessage(MessageType type, int clientIndex) {
    return server.createMessage(clientIndex, type);
}

void ServerMessageTransmitter::doSendMessage(Message* message, int clientIndex) {
    server.sendMessage(clientIndex, message);
}
