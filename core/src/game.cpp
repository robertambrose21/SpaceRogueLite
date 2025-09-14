#include "game.h"

using namespace SpaceRogueLite;

Game::Game() {}
Game::~Game() {}

void Game::run(void) { loop(); }

void Game::loop(void) {
    bool quit = false;

    while (!quit) {
        std::cout << "Game loop running..." << std::endl;
    }
}