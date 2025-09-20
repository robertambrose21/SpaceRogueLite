#include <spdlog/spdlog.h>

#include "actorspawner.h"
#include "components.h"

using namespace SpaceRogueLite;

ActorSpawner::ActorSpawner(entt::registry &registry, entt::dispatcher &dispatcher)
    : registry(registry), dispatcher(dispatcher), dispatchListener(*this, dispatcher) {}

entt::entity ActorSpawner::spawnActor(const std::string &name) {
    static uint32_t nextId = 1;

    entt::entity entity = registry.create();

    registry.emplace<ActorTag>(entity);
    registry.emplace<Health>(entity, 100, 100);
    registry.emplace<Position>(entity, 0, 0);
    registry.emplace<ExternalId>(entity, nextId++);

    spdlog::info("Spawned actor '{}' with entity ID {} and external ID {}", name, int(entity), nextId - 1);

    return entity;
}

void ActorSpawner::despawnActor(entt::entity entity) {
    if (registry.valid(entity)) {
        registry.destroy(entity);
        spdlog::info("Despawned actor with entity ID {}", int(entity));
    } else {
        spdlog::warn("Attempted to despawn invalid entity ID {}", int(entity));
    }
}

ActorSystem::ActorSystem(entt::registry &registry, entt::dispatcher &dispatcher)
    : registry(registry), dispatcher(dispatcher) {}

void ActorSystem::applyDamage(entt::entity entity, int damage) {
    if (auto *health = registry.try_get<Health>(entity)) {
        health->current -= damage;

        spdlog::info("Entity ID {} took {} damage, current health: {}", int(entity), damage, health->current);

        if (health->current <= 0) {
            dispatcher.trigger<ActorDespawnEvent>({entity});
            health->current = 0;
        }
    }
}