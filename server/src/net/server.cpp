#include "server.h"

using namespace SpaceRogueLite;

Server::Server(const yojimbo::Address& address, int maxConnections)
    : adapter(this),
      address(address),
      server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, address, yojimbo::ClientServerConfig(), adapter, 0.0),
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
    spdlog::info("Server address is {}", buffer);
}

void Server::stop(void) {
    if (!server.IsRunning()) {
        spdlog::info("Cannot stop server, server is not running");
        return;
    }

    server.Stop();
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

Adapter::Adapter(Server* server) : server(server) {}

yojimbo::MessageFactory* Adapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}

void Adapter::OnServerClientConnected(int clientIndex) {
    if (server == nullptr) {
        return;
    }

    server->onClientConnected(clientIndex);
}

void Adapter::OnServerClientDisconnected(int clientIndex) {
    if (server == nullptr) {
        return;
    }

    server->onClientDisconnected(clientIndex);
}