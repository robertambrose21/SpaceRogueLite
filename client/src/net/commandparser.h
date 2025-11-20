#pragma once

#include <spdlog/spdlog.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "messagefactory.h"

namespace SpaceRogueLite {

/**
 * Parsed command structure
 */
struct ParsedCommand {
    MessageType messageType;
    std::vector<std::string> arguments;
};

/**
 * Parser for CLI commands
 * Supports format: /send MessageTypeName [args...]
 * Arguments can be quoted: /send SpawnActor 'Enemy5'
 */
class CommandParser {
public:
    /**
     * Parse a command string
     * @param commandString The full command string (e.g., "/send SpawnActor 'Enemy5'")
     * @return Parsed command if valid, std::nullopt otherwise
     */
    static std::optional<ParsedCommand> parse(const std::string& commandString) {
        if (commandString.empty()) {
            return std::nullopt;
        }

        // Tokenize the command string
        std::vector<std::string> tokens = tokenize(commandString);
        if (tokens.empty()) {
            return std::nullopt;
        }

        // Check for /send command
        if (tokens[0] != "/send") {
            spdlog::warn("Unknown command '{}'. Only '/send' is currently supported.", tokens[0]);
            return std::nullopt;
        }

        if (tokens.size() < 2) {
            spdlog::warn("Usage: /send <MessageType> [args...]");
            printAvailableMessages();
            return std::nullopt;
        }

        std::string messageTypeName = tokens[1];
        auto messageType = parseMessageType(messageTypeName);
        if (!messageType.has_value()) {
            spdlog::warn("Unknown message type '{}'", messageTypeName);
            printAvailableMessages();
            return std::nullopt;
        }

        std::vector<std::string> arguments(tokens.begin() + 2, tokens.end());

        return ParsedCommand{messageType.value(), arguments};
    }

private:
    /**
     * Tokenize a command string, respecting quoted strings
     */
    static std::vector<std::string> tokenize(const std::string& str) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;
        char quoteChar = '\0';

        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];

            if (!inQuotes && (c == '\'' || c == '"')) {
                // Start of quoted string
                inQuotes = true;
                quoteChar = c;
            } else if (inQuotes && c == quoteChar) {
                // End of quoted string
                inQuotes = false;
                quoteChar = '\0';
                // Don't add the quote character, but do push the token if non-empty
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else if (!inQuotes && std::isspace(c)) {
                // Whitespace outside quotes - token separator
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                // Regular character
                currentToken += c;
            }
        }

        // Add final token if any
        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }

        return tokens;
    }

    /**
     * Parse message type name to enum
     * Parses enum names like "PING" or "SPAWN_ACTOR"
     */
    static std::optional<MessageType> parseMessageType(const std::string& name) {
        std::string upperName = name;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        // clang-format off
#define MESSAGE_TYPE_MATCH(enumName, messageClass)     \
        if (upperName == #enumName) {                  \
            return MessageType::enumName;              \
        }
        MESSAGE_LIST(MESSAGE_TYPE_MATCH)
#undef MESSAGE_TYPE_MATCH
        // clang-format on

        return std::nullopt;
    }

    /**
     * Print available message types
     */
    static void printAvailableMessages() {
        spdlog::info("Available message types:");
        // clang-format off
#define PRINT_MESSAGE_HELP(enumName, messageClass)     \
        {                                              \
            spdlog::info("  - {}", #enumName);         \
        }
        MESSAGE_LIST(PRINT_MESSAGE_HELP)
#undef PRINT_MESSAGE_HELP
        // clang-format on
    }
};

}  // namespace SpaceRogueLite
