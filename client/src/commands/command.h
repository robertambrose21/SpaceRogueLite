#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SpaceRogueLite {

/**
 * CRTP base class for commands.
 * Derived classes MUST implement:
 *   - static constexpr const char* name() - returns the command name (e.g., "send")
 *   - static std::optional<Derived> parse(const std::vector<std::string>& tokens)
 */
template <typename Derived>
struct Command {
    std::vector<std::string> arguments;

    static constexpr const char* name() { return Derived::name(); }

    static std::optional<Derived> parse(const std::vector<std::string>& tokens) {
        return Derived::parse(tokens);
    }
};

}  // namespace SpaceRogueLite
