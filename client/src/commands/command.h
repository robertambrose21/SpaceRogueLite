#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SpaceRogueLite {

/**
 * CRTP base class for commands.
 * Derived classes MUST implement: static std::optional<Derived> parse(const std::vector<std::string>& tokens)
 */
template <typename Derived>
struct Command {
    std::vector<std::string> arguments;

    // Enforce parse interface at compile time
    static std::optional<Derived> parse(const std::vector<std::string>& tokens) {
        return Derived::parse(tokens);
    }
};

}  // namespace SpaceRogueLite
