#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace SpaceRogueLite {

struct ActorSpawnEvent {
    std::string name;
};

struct ActorDespawnEvent {
    entt::entity entity;
};

class ActorSpawner {
public:
    ActorSpawner(entt::registry &registry, entt::dispatcher &dispatcher);
    ~ActorSpawner() = default;

    entt::entity spawnActor(const std::string &name);
    void despawnActor(entt::entity entity);

    struct DispatchListener {
        DispatchListener(ActorSpawner &spawner, entt::dispatcher &dispatcher)
            : spawner(spawner) {
            dispatcher.sink<ActorSpawnEvent>()
                .connect<&DispatchListener::handleActorSpawnEvent>(*this);
            dispatcher.sink<ActorDespawnEvent>()
                .connect<&DispatchListener::handleActorDespawnEvent>(*this);
        }

        void handleActorSpawnEvent(const ActorSpawnEvent &event) {
            spawner.spawnActor(event.name);
        }

        void handleActorDespawnEvent(const ActorDespawnEvent &event) {
            spawner.despawnActor(event.entity);
        }

        ActorSpawner &spawner;
    };

private:
    entt::registry &registry;
    entt::dispatcher &dispatcher;
    DispatchListener dispatchListener;
};

class ActorSystem {
public:
    ActorSystem(entt::registry &registry, entt::dispatcher &dispatcher);
    ~ActorSystem() = default;

    void applyDamage(entt::entity entity, int damage);

private:
    entt::registry &registry;
    entt::dispatcher &dispatcher;
};

}  // namespace SpaceRogueLite