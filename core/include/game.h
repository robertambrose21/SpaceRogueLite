#pragma once

#include <entt/entt.hpp>
#include <iostream>

namespace SpaceRogueLite {

class Game {
public:
    Game();
    ~Game();

    void run(void);

private:
    void loop(void);
};

}  // namespace SpaceRogueLite
