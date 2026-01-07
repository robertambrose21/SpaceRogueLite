#pragma once

#include <yojimbo.h>

#include "tileid.h"

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
    char actorName[256];

    SpawnActorMessage() : Message(MessageChannel::RELIABLE) {}

    constexpr const char* getName() const override { return "SpawnActor"; }

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

class LoadMapChunkMessage : public Message {
public:
    struct ChunkTile {
        TileId id;
        uint8_t orientation;
        char type[64];
        uint8_t walkability;
    };

    struct MapChunk {
        static constexpr uint16_t MAX_WIDTH = 16;
        static constexpr uint16_t MAX_HEIGHT = 16;

        uint16_t width;
        uint16_t height;
        uint16_t posX;
        uint16_t posY;

        ChunkTile tiles[MAX_WIDTH * MAX_HEIGHT];
    };

    MapChunk chunk;

    LoadMapChunkMessage() : Message(MessageChannel::RELIABLE) {}

    constexpr const char* getName() const override { return "LoadMapChunk"; }

    bool parseFromCommand(const std::vector<std::string>& args) override {
        spdlog::warn("LoadMapChunkMessage cannot be parsed from command");
        return false;
    }

    std::string toString(void) const {
        std::string data = "";

        for (uint16_t x = 0; x < chunk.width; x++) {
            for (uint16_t y = 0; y < chunk.height; y++) {
                data += std::to_string(chunk.tiles[y * chunk.width + x].id) + " ";
            }
            data += "\n";
        }

        return std::string(getName()) + ": (" + std::to_string(chunk.posX) + ", " +
               std::to_string(chunk.posY) + ")\n" + data;
    }

    std::string getCommandHelpText(void) const override { return "Loads a map chunk"; }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_bits(stream, chunk.width, sizeof(uint16_t));
        serialize_bits(stream, chunk.height, sizeof(uint16_t));
        serialize_bits(stream, chunk.posX, sizeof(uint16_t));
        serialize_bits(stream, chunk.posY, sizeof(uint16_t));

        for (uint16_t x = 0; x < chunk.width; x++) {
            for (uint16_t y = 0; y < chunk.height; y++) {
                ChunkTile& tile = chunk.tiles[y * chunk.width + x];

                serialize_bits(stream, tile.id, sizeof(TileId));
                serialize_bits(stream, tile.orientation, sizeof(uint8_t));
                serialize_string(stream, tile.type, 64);
                serialize_bits(stream, tile.walkability, sizeof(uint8_t));
            }
        }

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