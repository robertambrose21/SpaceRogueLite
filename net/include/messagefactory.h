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
    PingMessage() : Message("Ping", MessageChannel::UNRELIABLE) {}

    std::string toString(void) const { return getName(); }

    void parse() {}

    std::string getCommandHelpText(void) const { return "Usage: Sends a ping message to the server."; }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

class SpawnActorMessage : public Message {
public:
    SpawnActorMessage() : Message("SpawnActor", MessageChannel::RELIABLE) {}

    char actorName[256];

    std::string toString(void) const { return getName() + ": " + actorName; }

    void parse(const char* name) {
        if (name == nullptr) {
            spdlog::warn("SpawnActor parse received null actor name");
            return;
        }

        size_t nameLen = strlen(name);
        if (nameLen >= sizeof(actorName)) {
            spdlog::warn("Actor name too long, must be less than {}", sizeof(actorName) - 1);
            return;
        }

        if (strlcpy(actorName, name, sizeof(actorName)) > sizeof(actorName)) {
            spdlog::critical("Buffer overflow in actorName");
            return;
        }
    }

    void parse(const std::string& name) { parse(name.c_str()); }

    std::string getCommandHelpText(void) const { return "Usage: Sends a ping message to the server."; }

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