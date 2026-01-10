#include "server.h"

#include <algorithm>
#include <cstring>

using namespace SpaceRogueLite;

// ---------------------------------------------------------------
// -- SERVER -----------------------------------------------------
// ---------------------------------------------------------------
Server::Server(const yojimbo::Address& address, int maxConnections, MessageHandler& messageHandler)
    : adapter(this),
      address(address),
      server(yojimbo::GetDefaultAllocator(), SERVER_DEFAULT_PRIVATE_KEY, address, ConnectionConfig(), adapter, 0.0),
      maxConnections(maxConnections),
      messageHandler(messageHandler) {}

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

Message* Server::createMessage(int clientIndex, const MessageType& messageType) {
    return dynamic_cast<Message*>(server.CreateMessage(clientIndex, static_cast<int>(messageType)));
}

void Server::sendMessage(int clientIndex, Message* message) {
    server.SendMessage(clientIndex, static_cast<int>(message->getMessageChannel()), message);
}

void Server::update(int64_t timeSinceLastFrame) {
    server.AdvanceTime(server.GetTime() + ((double) timeSinceLastFrame) / 1000.0f);
    server.ReceivePackets();

    processMessages();

    // TODO: Send a ping every second or so: https://github.com/networkprotocol/yojimbo/issues/138 and
    // https://github.com/networkprotocol/yojimbo/issues/146 Packets are intended to be sent pretty regulary - we can
    // remove this when we're sending packets more regularly
    if (server.HasMessagesToSend(0, (int) MessageChannel::RELIABLE) ||
        server.HasMessagesToSend(0, (int) MessageChannel::UNRELIABLE)) {
        server.SendPackets();
    }
}

void Server::processMessages(void) {
    for (int i = 0; i < maxConnections; i++) {
        if (!server.IsClientConnected(i)) {
            continue;
        }

        for (int j = 0; j < connectionConfig.numChannels; j++) {
            yojimbo::Message* yojimboMessage = server.ReceiveMessage(i, j);
            while (yojimboMessage != NULL) {
                auto message = dynamic_cast<Message*>(yojimboMessage);
                if (!message) {
                    spdlog::critical("Invalid dynamic_cast for yojimbo::Message, type is: '{}'. Check message factory",
                                     yojimboMessage->GetType());
                    continue;
                }

                processMessage(i, static_cast<MessageChannel>(j), message);
                server.ReleaseMessage(i, yojimboMessage);
                yojimboMessage = server.ReceiveMessage(i, j);
            }
        }
    }
}

void Server::processMessage(int clientIndex, MessageChannel channel, Message* message) {
    messageHandler.processMessage(clientIndex, channel, message);
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

    sendMapToClient(clientIndex);
}

void Server::onClientDisconnected(int clientIndex) {
    uint64_t clientId = server.GetClientId(clientIndex);
    clientIds[clientId] = DISCONNECTED;

    spdlog::info("Client {}:[{}] disconnected", clientIndex, clientId);
}

void Server::sendMapToClient(int clientIndex) {
    auto& grid = entt::locator<Grid>::value();

    constexpr int CHUNK_SIZE = 16;

    for (int chunkY = 0; chunkY < grid.getHeight(); chunkY += CHUNK_SIZE) {
        for (int chunkX = 0; chunkX < grid.getWidth(); chunkX += CHUNK_SIZE) {
            auto* msg = static_cast<LoadMapChunkMessage*>(
                createMessage(clientIndex, MessageType::LOAD_MAP_CHUNK));

            msg->chunk.posX = chunkX;
            msg->chunk.posY = chunkY;
            msg->chunk.width = std::min(CHUNK_SIZE, grid.getWidth() - chunkX);
            msg->chunk.height = std::min(CHUNK_SIZE, grid.getHeight() - chunkY);

            for (int y = 0; y < msg->chunk.height; y++) {
                for (int x = 0; x < msg->chunk.width; x++) {
                    auto gridTile = grid.getTile(chunkX + x, chunkY + y);
                    auto& chunkTile = msg->chunk.tiles[y * msg->chunk.width + x];

                    chunkTile.id = gridTile.id;
                    chunkTile.orientation = gridTile.orientation;
                    chunkTile.walkability = static_cast<uint8_t>(gridTile.walkable);
                    std::strncpy(chunkTile.type, gridTile.type.c_str(), 63);
                    chunkTile.type[63] = '\0';
                }
            }

            sendMessage(clientIndex, msg);
        }
    }

    spdlog::info("Sent map ({} chunks) to client {}",
                 (grid.getWidth() / CHUNK_SIZE) * (grid.getHeight() / CHUNK_SIZE),
                 clientIndex);
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