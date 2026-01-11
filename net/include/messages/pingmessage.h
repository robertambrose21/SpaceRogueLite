#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include <string>
#include <vector>

#include "../connectionconfig.h"
#include "../message.h"

namespace SpaceRogueLite {

class PingMessage : public Message {
public:
    PingMessage() : Message(MessageChannel::UNRELIABLE) {}

    constexpr const char* getName() const override { return "Ping"; }

    std::string toString(void) const override { return getName(); }

    bool parseFromCommand(const std::vector<std::string>& args) override {
        if (!args.empty()) {
            spdlog::warn("PingMessage expects 0 arguments, but received {}", args.size());
            return false;
        }

        return true;
    }

    std::string getCommandHelpText(void) const override { return "Sends a ping message."; }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

}  // namespace SpaceRogueLite
