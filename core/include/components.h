#pragma once

#include <glm/glm.hpp>

// COMMON
typedef glm::ivec2 Position;

// ACTOR
struct ActorTag {};
struct Health {
    int current;
    int max;
};
typedef uint32_t ExternalId;