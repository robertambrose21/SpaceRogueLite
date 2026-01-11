#pragma once

#include <spdlog/spdlog.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "command.h"
#include "messagefactory.h"

namespace SpaceRogueLite {

/**
 * Command for sending network messages
 * Format: send <MessageType> [args...]
 */
struct SendCommand : Command<SendCommand> {
    static constexpr const char* name() { return "send"; }

    MessageType messageType;

    /**
     * Parse tokens into a SendCommand
     * tokens[0] = "send", tokens[1] = message type name, tokens[2+] = message args
     */
    static std::optional<SendCommand> parse(const std::vector<std::string>& tokens) {
        if (tokens.empty() || tokens[0] != name()) {
            return std::nullopt;
        }

        if (tokens.size() < 2) {
            spdlog::warn("Usage: send <MessageType> [args...]");
            printAvailableMessages();
            return std::nullopt;
        }

        auto msgType = parseMessageType(tokens[1]);
        if (!msgType.has_value()) {
            spdlog::warn("Unknown message type '{}'", tokens[1]);
            printAvailableMessages();
            return std::nullopt;
        }

        SendCommand cmd;
        cmd.messageType = msgType.value();
        cmd.arguments = std::vector<std::string>(tokens.begin() + 2, tokens.end());
        return cmd;
    }

private:
    static std::optional<MessageType> parseMessageType(const std::string& name) {
        std::string upperName = name;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        // clang-format off
#define MESSAGE_TYPE_MATCH(enumName, messageClass, direction)                              \
        if (upperName == #enumName && std::string(#direction) != "SERVER_TO_CLIENT") {     \
            return MessageType::enumName;                                                  \
        }
        MESSAGE_LIST(MESSAGE_TYPE_MATCH)
#undef MESSAGE_TYPE_MATCH
        // clang-format on

        return std::nullopt;
    }

    static void printAvailableMessages() {
        spdlog::info("Available message types:");
        // clang-format off
#define PRINT_MESSAGE_HELP(enumName, messageClass, direction)                    \
        if (std::string(#direction) != "SERVER_TO_CLIENT") {                     \
            spdlog::info("  - {}", #enumName);                                   \
        }
        MESSAGE_LIST(PRINT_MESSAGE_HELP)
#undef PRINT_MESSAGE_HELP
        // clang-format on
    }
};

}  // namespace SpaceRogueLite
