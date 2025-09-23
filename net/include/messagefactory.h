#pragma once

#include <yojimbo.h>

#include "connectionconfig.h"
#include "message.h"

namespace SpaceRogueLite {

enum class MessageType { PING, COUNT };

class PingMessage : public Message {
public:
    PingMessage() : Message("Ping", MessageChannel::UNRELIABLE) {}

    std::string toString(void) const { return getName(); }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int) MessageType::COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE((int) MessageType::PING, PingMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

}  // namespace SpaceRogueLite