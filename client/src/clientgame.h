#pragma once

#include <game.h>
#include <entt/entt.hpp>
#include <window.h>
#include "input/camerainput.h"
#include "net/client.h"
#include "net/clientmessagehandler.h"
#include "net/clientmessagetransmitter.h"

class ClientGame : public SpaceRogueLite::Game {
public:
    ClientGame() = default;
    ~ClientGame() = default;

    void onLoad(void) override;
    void onUnload(void) override;

private:
    entt::registry registry;
    entt::dispatcher dispatcher;

    std::unique_ptr<SpaceRogueLite::ClientMessageHandler> messageHandler;
    std::unique_ptr<SpaceRogueLite::Client> client;
    std::unique_ptr<SpaceRogueLite::Window> window;
    std::unique_ptr<SpaceRogueLite::CameraInput> cameraInput;
    std::unique_ptr<SpaceRogueLite::ClientMessageTransmitter> messageTransmitter;
};