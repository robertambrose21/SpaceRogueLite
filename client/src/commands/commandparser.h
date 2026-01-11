#pragma once

#include <spdlog/spdlog.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sendcommand.h"

namespace SpaceRogueLite {

/**
 * Tokenizer utility - splits command string, respecting quotes
 * Returns tokens with command name as first element, e.g. ["send", "PING"]
 */
class CommandTokenizer {
public:
    static std::vector<std::string> tokenize(const std::string& str) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;
        char quoteChar = '\0';

        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];

            if (!inQuotes && (c == '\'' || c == '"')) {
                inQuotes = true;
                quoteChar = c;
            } else if (inQuotes && c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else if (!inQuotes && std::isspace(c)) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }

        return tokens;
    }
};

// Variant of all command types - add new commands here
using ParsedCommand = std::variant<SendCommand>;

/**
 * Try to parse command by iterating through all command types in ParsedCommand variant
 */
template <size_t I = 0>
std::optional<ParsedCommand> tryParseCommand(const std::string& cmdName,
                                              const std::vector<std::string>& tokens) {
    if constexpr (I < std::variant_size_v<ParsedCommand>) {
        using CmdType = std::variant_alternative_t<I, ParsedCommand>;
        if (cmdName == CmdType::name()) {
            auto cmd = CmdType::parse(tokens);
            if (cmd.has_value()) {
                return cmd.value();
            }
            return std::nullopt;
        }
        return tryParseCommand<I + 1>(cmdName, tokens);
    }
    return std::nullopt;
}

/**
 * Parse a command string and dispatch to appropriate command parser
 * @param input The full command string (e.g., "send PING")
 * @return Parsed command variant if valid, std::nullopt otherwise
 */
inline std::optional<ParsedCommand> parseCommand(const std::string& input) {
    auto tokens = CommandTokenizer::tokenize(input);
    if (tokens.empty()) {
        return std::nullopt;
    }

    auto result = tryParseCommand(tokens[0], tokens);
    if (!result.has_value()) {
        spdlog::warn("Unknown command '{}'", tokens[0]);
    }
    return result;
}

}  // namespace SpaceRogueLite
