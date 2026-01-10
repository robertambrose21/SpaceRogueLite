#pragma once

#include <game.h>

class ServerGame : public SpaceRogueLite::Game {
public:
    ServerGame() = default;
    ~ServerGame() = default;

    void onLoad(void) override;
};