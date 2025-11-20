#pragma once

#include <yojimbo.h>

#include "connectionconfig.h"
#include "message.h"

namespace SpaceRogueLite {

// ============================================================================
// MESSAGE REGISTRY - Single source of truth for all message types
// ============================================================================
// Add new messages here
// Format: X(ENUM_NAME, MessageClass)
#define MESSAGE_LIST(X)  \
    X(PING, PingMessage) \
    X(SPAWN_ACTOR, SpawnActorMessage)

enum class MessageType {
#define MESSAGE_ENUM(name, messageClass) name,
    MESSAGE_LIST(MESSAGE_ENUM)
#undef MESSAGE_ENUM
        COUNT
};

class PingMessage : public Message {
public:
    PingMessage() : Message(MessageChannel::UNRELIABLE) {}

    constexpr const char* getName() const override { return "Ping"; }

    std::string toString(void) const { return getName(); }

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

class SpawnActorMessage : public Message {
public:
    SpawnActorMessage() : Message(MessageChannel::RELIABLE) {}

    constexpr const char* getName() const override { return "SpawnActor"; }

    char actorName[256];

    std::string toString(void) const { return std::string(getName()) + ": " + actorName; }

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

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int) MessageType::COUNT);
#define MESSAGE_FACTORY_REGISTER(name, messageClass) \
    YOJIMBO_DECLARE_MESSAGE_TYPE((int) MessageType::name, messageClass);
MESSAGE_LIST(MESSAGE_FACTORY_REGISTER)
#undef MESSAGE_FACTORY_REGISTER
YOJIMBO_MESSAGE_FACTORY_FINISH();

}  // namespace SpaceRogueLite