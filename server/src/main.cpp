#include <entt/entt.hpp>
#include <iostream>

struct Position {
    float x;
    float y;
};

int main() {
    entt::registry registry;

    const auto entity1 = registry.create();
    const auto entity2 = registry.create();
    registry.emplace<Position>(entity1, 1.0f, 2.0f);
    registry.emplace<Position>(entity2, 3.0f, 5.1f);

    auto view = registry.view<Position>();
    for (auto [entity, position] : view.each()) {
        std::cout << "Entity " << int(entity) << " Position: (" << position.x
                  << ", " << position.y << ")\n";
    }

    return 0;
}
