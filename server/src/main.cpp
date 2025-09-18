#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <iostream>

#include "actorspawner.h"
#include "game.h"

struct Position {
    float x;
    float y;
};

int main() {
    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return 1;
    }

    spdlog::info("Yojimbo initialized successfully.");

    entt::registry registry;
    entt::dispatcher dispatcher;

    SpaceRogueLite::Game game;
    SpaceRogueLite::ActorSpawner spawner(registry, dispatcher);
    SpaceRogueLite::ActorSystem actorSystem(registry, dispatcher);

    auto player = spawner.spawnActor("Player");
    auto enemy = spawner.spawnActor("Enemy");

    actorSystem.applyDamage(enemy, 50);
    actorSystem.applyDamage(enemy, 60);  // This should trigger despawn
    // game.run();

    // std::cout << "test" << std::endl;

    ShutdownYojimbo();

    return 0;
}
