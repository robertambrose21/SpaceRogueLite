#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <map>
#include <string>

#include "connectionconfig.h"

namespace SpaceRogueLite {

class Message : public yojimbo::Message {
public:
    Message(const std::string& name, MessageChannel channel) : name(name), channel(channel) {}
    virtual ~Message() = default;

    virtual std::string toString(void) const = 0;

    /**
     * @brief Parse arguments to populate this message's data fields.
     *
     * Message implementations should override this method with specific parameter types
     * that match their data requirements.
     *
     * Examples:
     *   - No-argument messages (like PingMessage): void parse() {}
     *   - Single argument: void parse(const std::string& name) { ... }
     *   - Multiple arguments: void parse(int x, int y, float z) { ... }
     *
     * If this default implementation is called with arguments, it indicates the message
     * class has not properly overridden parse() for its parameter types.
     */
    template <typename... Args>
    void parse(Args&&... args) {
        if constexpr (sizeof...(args) > 0) {
            spdlog::critical(
                "Message '{}' called default parse() with {} argument(s) but has not overridden parse() for these "
                "types. "
                "Please implement parse() in your message class.",
                getName(), sizeof...(args));
        }
    }

    virtual std::string getCommandHelpText(void) const = 0;

    std::string getName() const { return name; }
    MessageChannel getMessageChannel() const noexcept { return channel; }

private:
    std::string name;
    MessageChannel channel;
};

class MessageCommand {
public:
    explicit MessageCommand(Message* message, const std::string& command,
                            const std::map<std::string, std::string>& args)
        : command(command), args(args) {}

    std::string getCommand() const { return command; }
    const std::map<std::string, std::string>& getArgs() const { return args; }

private:
    std::string command;
    std::map<std::string, std::string> args;
};

}  // namespace SpaceRogueLite