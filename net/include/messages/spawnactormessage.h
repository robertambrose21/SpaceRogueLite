#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include <cstring>
#include <string>
#include <vector>

#include "../connectionconfig.h"
#include "../message.h"

namespace SpaceRogueLite {

class SpawnActorMessage : public Message {
public:
    char actorName[256];

    SpawnActorMessage() : Message(MessageChannel::RELIABLE) {}

    constexpr const char* getName() const override { return "SpawnActor"; }

    std::string toString(void) const override { return std::string(getName()) + ": " + actorName; }

    bool parseFromCommand(const std::vector<std::string>& args) override {
        if (args.size() != 1) {
            spdlog::warn("SpawnActorMessage expects 1 argument, but received {}", args.size());
            return false;
        }

        return parse(args[0]);
    }

    bool parse(const char* name) {
        if (name == nullptr) {
            spdlog::warn("SpawnActor parse received null actor name");
            return false;
        }

        if (strlen(name) >= sizeof(actorName)) {
            spdlog::warn("Actor name too long, must be less than {}", sizeof(actorName) - 1);
            return false;
        }

        if (strlcpy(actorName, name, sizeof(actorName)) > sizeof(actorName)) {
            spdlog::critical("Buffer overflow in actorName");
            return false;
        }

        return true;
    }

    bool parse(const std::string& name) { return parse(name.c_str()); }

    std::string getCommandHelpText(void) const override { return "Spawns a new actor."; }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_string(stream, actorName, 256);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

}  // namespace SpaceRogueLite
