#include "client.h"

using namespace SpaceRogueLite;

Client::Client(uint32_t clientId, const yojimbo::Address& serverAddress)
    : clientId(clientId),
      serverAddress(serverAddress),
      adapter(ClientAdapter()),
      client(yojimbo::GetDefaultAllocator(), serverAddress, ConnectionConfig(), adapter, 0.0) {}

Client::~Client() { client.Disconnect(); }

void Client::connect(void) {
    char buffer[256];
    serverAddress.ToString(buffer, sizeof(buffer));
    spdlog::info("Connecting to server at {} with client id [{}]", buffer, clientId);

    client.InsecureConnect(CLIENT_DEFAULT_PRIVATE_KEY, clientId, serverAddress);
}

void Client::disconnect(void) {
    if (!client.IsConnected()) {
        spdlog::info("Cannot disconnect client, client is not connected");
        return;
    }

    spdlog::info("Disconnecting client");
    client.Disconnect();
}

uint64_t Client::getClientId(void) const { return clientId; }

yojimbo::MessageFactory* ClientAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void ClientAdapter::OnServerClientConnected(int clientIndex) {
    // No-op
}

void ClientAdapter::OnServerClientDisconnected(int clientIndex) {
    // No-op
}