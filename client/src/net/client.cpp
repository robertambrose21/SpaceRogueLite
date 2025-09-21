#include "client.h"

using namespace SpaceRogueLite;

// ---------------------------------------------------------------
// -- CLIENT -----------------------------------------------------
// ---------------------------------------------------------------
Client::Client(uint32_t clientId, const yojimbo::Address& serverAddress)
    : clientId(clientId),
      serverAddress(serverAddress),
      adapter(ClientAdapter()),
      client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), ConnectionConfig(), adapter, 0.0) {}

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

void Client::update(int64_t timeSinceLastFrame) {
    client.AdvanceTime(client.GetTime() + ((double)timeSinceLastFrame) / 1000.0f);
    client.ReceivePackets();

    if (client.IsConnected()) {
        processMessages();
    }

    // TODO: See message in gameserver.cpp
    if (client.HasMessagesToSend((int)MessageChannel::RELIABLE)) {
        client.SendPackets();
    }
}

void Client::processMessages(void) {
    for (int i = 0; i < connectionConfig.numChannels; i++) {
        yojimbo::Message* message = client.ReceiveMessage(i);
        while (message != NULL) {
            processMessage(message);
            client.ReleaseMessage(message);
            message = client.ReceiveMessage(i);
        }
    }
}

void Client::processMessage(yojimbo::Message* message) {
    spdlog::info("Received message of type {} from the server", message->GetType());
}

uint64_t Client::getClientId(void) const { return clientId; }

// ---------------------------------------------------------------
// -- CLIENT ADAPTER ---------------------------------------------
// ---------------------------------------------------------------
yojimbo::MessageFactory* ClientAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void ClientAdapter::OnServerClientConnected(int clientIndex) {
    // No-op
}

void ClientAdapter::OnServerClientDisconnected(int clientIndex) {
    // No-op
}