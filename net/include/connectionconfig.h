#pragma once

#include <yojimbo.h>

namespace SpaceRogueLite {

enum class MessageChannel { RELIABLE, UNRELIABLE, COUNT };

struct ConnectionConfig : yojimbo::ClientServerConfig {
    ConnectionConfig() {
        numChannels = 2;
        channel[(int)MessageChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int)MessageChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

}  // namespace SpaceRogueLite