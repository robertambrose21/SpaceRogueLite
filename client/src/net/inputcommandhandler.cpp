#include "inputcommandhandler.h"

#include <spdlog/spdlog.h>

#include <iostream>
#include <string>

#include "clientmessagetransmitter.h"
#include "commandparser.h"
#include "message.h"

namespace SpaceRogueLite {

InputCommandHandler::InputCommandHandler(ClientMessageTransmitter& transmitter)
    : transmitter(transmitter), shouldQuit(false) {
    inputThread = std::thread(&InputCommandHandler::inputThreadFunction, this);
}

InputCommandHandler::~InputCommandHandler() {
    shouldQuit.store(true);

    if (inputThread.joinable()) {
        inputThread.join();
    }
}

void InputCommandHandler::inputThreadFunction() {
    spdlog::info("Input thread started. Type '/send <MessageType> [args...]' to send messages.");
    spdlog::info("Example: /send SpawnActor 'Enemy5'");

    std::string line;
    while (!shouldQuit.load()) {
        if (std::getline(std::cin, line)) {
            if (!line.empty()) {
                commandQueue.enqueue(line);
            }
        }

        if (std::cin.eof() || std::cin.bad()) {
            spdlog::info("Input thread detected EOF or error state, exiting...");
            break;
        }

        std::cin.clear();
    }

    spdlog::info("Input thread shutting down.");
}

void InputCommandHandler::processCommands(int64_t timeSinceLastFrame) {
    while (auto commandOpt = commandQueue.tryDequeue()) {
        std::string command = commandOpt.value();

        auto parsedCommand = CommandParser::parse(command);
        if (!parsedCommand.has_value()) {
            continue;
        }

        const auto& cmd = parsedCommand.value();

        transmitter.sendMessage(cmd.messageType, cmd.arguments);
    }
}

}  // namespace SpaceRogueLite
