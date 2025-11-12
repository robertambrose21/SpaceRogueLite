#include "server.h"
#include "actorspawner.h"

using namespace SpaceRogueLite;

// ---------------------------------------------------------------
// -- SERVER -----------------------------------------------------
// ---------------------------------------------------------------
Server::Server(const yojimbo::Address& address, int maxConnections, entt::dispatcher& dispatcher)
    : adapter(this),
      address(address),
      server(yojimbo::GetDefaultAllocator(), SERVER_DEFAULT_PRIVATE_KEY, address, ConnectionConfig(), adapter, 0.0),
      maxConnections(maxConnections),
      dispatcher(dispatcher) {}

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
    spdlog::debug("Received '{}' message from client {} on channel {}", message->getName(), clientIndex,
                  MessageChannelToString(channel));

    // Dispatch based on message type
    switch (message->GetType()) {
        case (int) MessageType::PING:
            // Handle ping - currently just logged
            break;

        case (int) MessageType::SPAWN_ACTOR: {
            auto* spawnMessage = static_cast<SpawnActorMessage*>(message);
            handleSpawnActorMessage(clientIndex, spawnMessage);
            break;
        }

        default:
            spdlog::warn("Unknown message type: {}", message->GetType());
            break;
    }
}

void Server::handleSpawnActorMessage(int clientIndex, SpawnActorMessage* message) {
    spdlog::info("Client {} requested spawn of actor '{}'", clientIndex, message->actorName);

    // Trigger the actor spawn event
    dispatcher.trigger<ActorSpawnEvent>({std::string(message->actorName)});
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