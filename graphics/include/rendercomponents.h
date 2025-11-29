#pragma once

#include <glm/glm.hpp>
#include <string>

namespace SpaceRogueLite {

struct Renderable {
    glm::vec2 size;
    glm::vec4 color;
    std::string texturePath;
};

struct RenderableUntextured {
    glm::vec2 size;
    glm::vec4 color;
};

}  // namespace SpaceRogueLite
