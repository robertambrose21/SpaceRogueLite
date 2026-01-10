#pragma once

#include <entt/entt.hpp>
#include <game.h>

#include "net/server.h"
#include "net/servermessagehandler.h"

class ServerGame : public SpaceRogueLite::Game {
public:
    ServerGame() = default;
    ~ServerGame() = default;

    void onLoad(void) override;
    void onUnload(void) override;

private:
    entt::registry registry;
    entt::dispatcher dispatcher;

    std::unique_ptr<SpaceRogueLite::ServerMessageHandler> messageHandler;
    std::unique_ptr<SpaceRogueLite::Server> server;
};