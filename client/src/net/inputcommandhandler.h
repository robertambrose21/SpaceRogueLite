#pragma once

#include <atomic>
#include <thread>

#include "commandqueue.h"

namespace SpaceRogueLite {

class ClientMessageTransmitter;

/**
 * Handles input commands from stdin in a separate thread.
 * Owns the command queue and input thread, managing their lifecycle.
 * Processes commands and routes them to the appropriate message transmitter.
 */
class InputCommandHandler {
public:
    /**
     * Constructor - starts the input thread
     * @param transmitter Reference to the client message transmitter
     */
    explicit InputCommandHandler(ClientMessageTransmitter& transmitter);

    /**
     * Destructor - cleanly shuts down the input thread
     */
    ~InputCommandHandler();

    // Delete copy and move operations (not safe with thread management)
    InputCommandHandler(const InputCommandHandler&) = delete;
    InputCommandHandler& operator=(const InputCommandHandler&) = delete;
    InputCommandHandler(InputCommandHandler&&) = delete;
    InputCommandHandler& operator=(InputCommandHandler&&) = delete;

    /**
     * Process all available commands from the queue.
     * Should be called regularly from the game loop worker.
     * @param timeSinceLastFrame Time elapsed since last frame (unused but required by worker signature)
     */
    void processCommands(int64_t timeSinceLastFrame);

private:
    /**
     * Input thread function - reads from stdin and enqueues commands
     */
    void inputThreadFunction();

    CommandQueue commandQueue;
    ClientMessageTransmitter& transmitter;
    std::atomic<bool> shouldQuit;
    std::thread inputThread;
};

}  // namespace SpaceRogueLite
