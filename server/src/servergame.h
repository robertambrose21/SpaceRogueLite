#pragma once

#include <entt/entt.hpp>
#include <game.h>

#include <actorspawner.h>

#include "net/server.h"
#include "net/servermessagehandler.h"

class ServerGame : public SpaceRogueLite::Game {
public:
    ServerGame() = default;
    ~ServerGame() = default;

    void onLoad(void) override;
    void onUnload(void) override;

    void sendMapToClient(int clientIndex);

private:
    entt::registry registry;
    entt::dispatcher dispatcher;

    std::unique_ptr<SpaceRogueLite::ServerMessageHandler> messageHandler;
    std::unique_ptr<SpaceRogueLite::Server> server;
    std::unique_ptr<SpaceRogueLite::ActorSpawner> spawner;
    std::unique_ptr<SpaceRogueLite::ActorSystem> actorSystem;
};