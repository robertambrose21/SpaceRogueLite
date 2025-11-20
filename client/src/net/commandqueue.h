#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <string>

namespace SpaceRogueLite {

/**
 * Thread-safe queue for passing commands from input thread to main thread
 */
class CommandQueue {
public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    // Non-copyable, non-movable
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;
    CommandQueue(CommandQueue&&) = delete;
    CommandQueue& operator=(CommandQueue&&) = delete;

    /**
     * Enqueue a command (called from input thread)
     */
    void enqueue(const std::string& command) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(command);
    }

    /**
     * Try to dequeue a command (non-blocking, called from main thread)
     * @return The next command if available, std::nullopt otherwise
     */
    std::optional<std::string> tryDequeue() {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return std::nullopt;
        }
        std::string command = queue.front();
        queue.pop();
        return command;
    }

    /**
     * Check if queue is empty
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    /**
     * Get current queue size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

private:
    std::queue<std::string> queue;
    mutable std::mutex mutex;
};

}  // namespace SpaceRogueLite
