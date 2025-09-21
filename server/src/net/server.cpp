#include "server.h"

using namespace SpaceRogueLite;

// ---------------------------------------------------------------
// -- SERVER -----------------------------------------------------
// ---------------------------------------------------------------
Server::Server(const yojimbo::Address& address, int maxConnections)
    : adapter(this),
      address(address),
      server(yojimbo::GetDefaultAllocator(), SERVER_DEFAULT_PRIVATE_KEY, address, ConnectionConfig(), adapter, 0.0),
      maxConnections(maxConnections) {}

Server::~Server() {
    if (server.IsRunning()) {
        server.Stop();
    }
}

void Server::start(void) {
    server.Start(maxConnections);

    if (!server.IsRunning()) {
        throw std::runtime_error("Could not start server at port " + std::to_string(address.GetPort()));
    }

    char buffer[256];
    server.GetAddress().ToString(buffer, sizeof(buffer));
    spdlog::info("Starting server at {}", buffer);
}

void Server::stop(void) {
    if (!server.IsRunning()) {
        spdlog::info("Cannot stop server, server is not running");
        return;
    }

    spdlog::info("Stopping server");
    server.Stop();
}

void Server::update(int64_t timeSinceLastFrame) {
    server.AdvanceTime(server.GetTime() + ((double)timeSinceLastFrame) / 1000.0f);
    server.ReceivePackets();

    processMessages();

    // TODO: Send a ping every second or so: https://github.com/networkprotocol/yojimbo/issues/138 and
    // https://github.com/networkprotocol/yojimbo/issues/146 Packets are intended to be sent pretty regulary - we can
    // remove this when we're sending packets more regularly
    if (server.HasMessagesToSend(0, (int)MessageChannel::RELIABLE)) {
        server.SendPackets();
    }
}

void Server::processMessages(void) {
    for (int i = 0; i < maxConnections; i++) {
        if (!server.IsClientConnected(i)) {
            continue;
        }

        for (int j = 0; j < connectionConfig.numChannels; j++) {
            yojimbo::Message* message = server.ReceiveMessage(i, j);
            while (message != NULL) {
                processMessage(i, message);
                server.ReleaseMessage(i, message);
                message = server.ReceiveMessage(i, j);
            }
        }
    }
}

void Server::processMessage(int clientIndex, yojimbo::Message* message) {
    spdlog::info("Received message of type {} from client {}", message->GetType(), clientIndex);
}

void Server::onClientConnected(int clientIndex) {
    uint64_t clientId = server.GetClientId(clientIndex);

    if (clientIds.contains(clientId)) {
        clientIds[clientId] = RECONNECTED;
        spdlog::info("Client {}:[{}] reconnected", clientIndex, clientId);
    } else {
        clientIds[clientId] = CONNECTED;
        spdlog::info("Client {}:[{}] connected", clientIndex, clientId);
    }
}

void Server::onClientDisconnected(int clientIndex) {
    uint64_t clientId = server.GetClientId(clientIndex);
    clientIds[clientId] = DISCONNECTED;

    spdlog::info("Client {}:[{}] disconnected", clientIndex, clientId);
}

// ---------------------------------------------------------------
// -- SERVER ADAPTER ---------------------------------------------
// ---------------------------------------------------------------
ServerAdapter::ServerAdapter(Server* server) : server(server) {}

yojimbo::MessageFactory* ServerAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void ServerAdapter::OnServerClientConnected(int clientIndex) {
    if (server == nullptr) {
        return;
    }

    server->onClientConnected(clientIndex);
}

void ServerAdapter::OnServerClientDisconnected(int clientIndex) {
    if (server == nullptr) {
        return;
    }

    server->onClientDisconnected(clientIndex);
}