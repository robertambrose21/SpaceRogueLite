#pragma once

#include <yojimbo.h>

#include "connectionconfig.h"
#include "message.h"
#include "messages/loadmapchunkmessage.h"
#include "messages/pingmessage.h"
#include "messages/spawnactormessage.h"

namespace SpaceRogueLite {

// ============================================================================
// MESSAGE REGISTRY - Single source of truth for all message types
// ============================================================================
// Add new messages here
// Format: X(ENUM_NAME, MessageClass, Direction)
#define MESSAGE_LIST(X)                              \
    X(PING, PingMessage, BIDIRECTIONAL)              \
    X(SPAWN_ACTOR, SpawnActorMessage, BIDIRECTIONAL) \
    X(LOAD_MAP_CHUNK, LoadMapChunkMessage, SERVER_TO_CLIENT)

enum class MessageType {
#define MESSAGE_ENUM(name, messageClass, direction) name,
    MESSAGE_LIST(MESSAGE_ENUM)
#undef MESSAGE_ENUM
        COUNT
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int) MessageType::COUNT);
#define MESSAGE_FACTORY_REGISTER(name, messageClass, direction) \
    YOJIMBO_DECLARE_MESSAGE_TYPE((int) MessageType::name, messageClass);
MESSAGE_LIST(MESSAGE_FACTORY_REGISTER)
#undef MESSAGE_FACTORY_REGISTER
YOJIMBO_MESSAGE_FACTORY_FINISH();

}  // namespace SpaceRogueLite
