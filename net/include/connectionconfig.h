#pragma once

#include <yojimbo.h>

namespace SpaceRogueLite {

enum class MessageChannel { RELIABLE, UNRELIABLE, COUNT };

enum class MessageDirection {
    CLIENT_TO_SERVER,  // Only sent by client, handled by server
    SERVER_TO_CLIENT,  // Only sent by server, handled by client
    BIDIRECTIONAL      // Can be sent/received by both
};

constexpr const char* MessageChannelToString(MessageChannel channel) {
    switch (channel) {
        case MessageChannel::RELIABLE:
            return "RELIABLE";
        case MessageChannel::UNRELIABLE:
            return "UNRELIABLE";
        default:
            return "UNKNOWN";
    }
}

struct ConnectionConfig : yojimbo::ClientServerConfig {
    ConnectionConfig() {
        numChannels = 2;
        channel[(int) MessageChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int) MessageChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

}  // namespace SpaceRogueLite