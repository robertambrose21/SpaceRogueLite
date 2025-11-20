#include "clientmessagetransmitter.h"

using namespace SpaceRogueLite;

ClientMessageTransmitter::ClientMessageTransmitter(Client& client) : client(client) {}

Message* ClientMessageTransmitter::createMessage(MessageType type, int clientIndex) {
    // clientIndex is ignored for client - client only sends to server
    return client.createMessage(type);
}

void ClientMessageTransmitter::doSendMessage(Message* message, int clientIndex) {
    // clientIndex is ignored for client - client only sends to server
    client.sendMessage(message);
}