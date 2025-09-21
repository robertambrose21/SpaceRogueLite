#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include "connectionconfig.h"
#include "messagefactory.h"

namespace SpaceRogueLite {

static const uint8_t CLIENT_DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = {0};

class ClientAdapter : public yojimbo::Adapter {
public:
    explicit ClientAdapter() = default;
    ~ClientAdapter() = default;

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override;

    void OnServerClientConnected(int clientIndex) override;
    void OnServerClientDisconnected(int clientIndex) override;
};

class Client {
public:
    explicit Client(uint32_t clientId, const yojimbo::Address& serverAddress);
    ~Client();

    void connect(void);
    void disconnect(void);

    uint64_t getClientId(void) const;

private:
    uint32_t clientId;

    yojimbo::Client client;
    yojimbo::Address serverAddress;

    ClientAdapter adapter;

    ConnectionConfig connectionConfig;
};

}  // namespace SpaceRogueLite
