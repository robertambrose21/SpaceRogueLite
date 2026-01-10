#pragma once

#include <game.h>

class ClientGame : public SpaceRogueLite::Game {
public:
    ClientGame() = default;
    ~ClientGame() = default;

    void onLoad(void) override;
};