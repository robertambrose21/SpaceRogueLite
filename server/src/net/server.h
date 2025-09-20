#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <iostream>
#include <map>

#include "messagefactory.h"

namespace SpaceRogueLite {

static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = {0};

static const int MAX_PLAYERS = 64;

enum class MessageChannel { RELIABLE, UNRELIABLE, COUNT };

struct ConnectionConfig2 : yojimbo::ClientServerConfig {
    ConnectionConfig2() {
        numChannels = 2;
        channel[(int)MessageChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int)MessageChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

class Server;

class Adapter : public yojimbo::Adapter {
public:
    explicit Adapter(Server* server = NULL);
    ~Adapter() {};

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;

    void OnServerClientConnected(int clientIndex) override;
    void OnServerClientDisconnected(int clientIndex) override;

private:
    Server* server;
};

class Server {
public:
    explicit Server(const yojimbo::Address& address, int maxConnections);
    ~Server();

    void start(void);
    void stop(void);

    void onClientConnected(int clientIndex);
    void onClientDisconnected(int clientIndex);

private:
    enum ConnectionState { CONNECTED = 0, RECONNECTED, DISCONNECTED };

    yojimbo::Server server;
    yojimbo::Address address;

    Adapter adapter;
    ConnectionConfig2 connectionConfig;

    int maxConnections;
    std::map<uint64_t, ConnectionState> clientIds;
};

};  // namespace SpaceRogueLite
