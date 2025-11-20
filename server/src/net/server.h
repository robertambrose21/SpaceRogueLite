#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <iostream>
#include <map>

#include "connectionconfig.h"
#include "message.h"
#include "messagefactory.h"
#include "messagehandler.h"

namespace SpaceRogueLite {

static const uint8_t SERVER_DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = {0};

static const int MAX_PLAYERS = 64;

class Server;

class ServerAdapter : public yojimbo::Adapter {
public:
    explicit ServerAdapter(Server* server = NULL);
    ~ServerAdapter() = default;

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;

    void OnServerClientConnected(int clientIndex) override;
    void OnServerClientDisconnected(int clientIndex) override;

private:
    Server* server;
};

class Server {
public:
    explicit Server(const yojimbo::Address& address, int maxConnections, MessageHandler& messageHandler);
    ~Server();

    void start(void);
    void stop(void);

    Message* tester(int& blah);

    Message* createMessage(int clientIndex, const MessageType& messageType);
    void sendMessage(int clientIndex, Message* message);

    void update(int64_t timeSinceLastFrame);

    void onClientConnected(int clientIndex);
    void onClientDisconnected(int clientIndex);

private:
    enum ConnectionState { CONNECTED = 0, RECONNECTED, DISCONNECTED };

    yojimbo::Server server;
    yojimbo::Address address;

    ServerAdapter adapter;
    ConnectionConfig connectionConfig;

    int maxConnections;
    std::map<uint64_t, ConnectionState> clientIds;

    MessageHandler& messageHandler;

    void processMessages(void);
    void processMessage(int clientIndex, MessageChannel channel, Message* message);
};

};  // namespace SpaceRogueLite
