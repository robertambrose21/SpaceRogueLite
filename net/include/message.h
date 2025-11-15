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

    virtual void parseFromCommand(const std::vector<std::string>& args) = 0;
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