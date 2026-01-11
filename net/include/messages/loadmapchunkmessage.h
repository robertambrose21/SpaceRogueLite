#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include <format>
#include <string>
#include <vector>

#include "../connectionconfig.h"
#include "../message.h"
#include "tileid.h"

namespace SpaceRogueLite {

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
    uint16_t sequenceNumber;
    uint16_t numSequences;

    LoadMapChunkMessage() : Message(MessageChannel::RELIABLE) {}

    constexpr const char* getName() const override { return "LoadMapChunk"; }

    bool parseFromCommand(const std::vector<std::string>& args) override {
        spdlog::warn("LoadMapChunkMessage cannot be parsed from command");
        return false;
    }

    std::string toString(void) const override {
        std::string data = "";

        for (uint16_t x = 0; x < chunk.width; x++) {
            for (uint16_t y = 0; y < chunk.height; y++) {
                data += std::to_string(chunk.tiles[y * chunk.width + x].id) + " ";
            }
            data += "\n";
        }

        return std::format("{}: ({}, {}) - {}/{}", getName(), chunk.posX, chunk.posY,
                           sequenceNumber, numSequences);
    }

    std::string getCommandHelpText(void) const override { return "Loads a map chunk"; }

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_bits(stream, chunk.width, 16);
        serialize_bits(stream, chunk.height, 16);
        serialize_bits(stream, chunk.posX, 16);
        serialize_bits(stream, chunk.posY, 16);
        serialize_bits(stream, sequenceNumber, 16);
        serialize_bits(stream, numSequences, 16);

        for (uint16_t x = 0; x < chunk.width; x++) {
            for (uint16_t y = 0; y < chunk.height; y++) {
                ChunkTile& tile = chunk.tiles[y * chunk.width + x];

                serialize_bits(stream, tile.id, 16);
                serialize_bits(stream, tile.orientation, 8);
                serialize_string(stream, tile.type, 64);
                serialize_bits(stream, tile.walkability, 8);
            }
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

}  // namespace SpaceRogueLite
