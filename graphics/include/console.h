#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <spdlog/sinks/base_sink.h>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include "inputhandler.h"

namespace SpaceRogueLite {

enum class LogLevel { Debug, Info, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
};

struct ConsoleInputWorker : InputHandler::InputWorker {
    explicit ConsoleInputWorker(uint32_t workerId, std::function<void()> toggleCallback) {
        id = workerId;
        name = "ConsoleToggle";
        function = [toggleCallback](const SDL_Event& event) {
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_GRAVE &&
                !event.key.repeat) {
                toggleCallback();
            }
        };
    }
};

class Console {
public:
    static constexpr size_t MAX_LOG_ENTRIES = 1000;
    static constexpr uint32_t CONSOLE_WORKER_ID = 100;

    explicit Console();
    ~Console() = default;

    void log(LogLevel level, const std::string& message);
    void logDebug(const std::string& message);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);

    void render();

    void toggle();
    bool isVisible() const;

    ConsoleInputWorker& getWorker();

    void setLevelFilter(LogLevel minLevel);
    LogLevel getLevelFilter() const;

    void clear();

    void setCommandCallback(std::function<void(const std::string&)> callback);

private:
    bool visible = false;
    bool justOpened = true;
    std::deque<LogEntry> entries;
    mutable std::mutex entriesMutex;
    LogLevel filterLevel = LogLevel::Debug;
    bool autoScroll = true;
    bool scrollToBottom = false;

    char inputBuffer[256] = {};
    std::function<void(const std::string&)> onCommand;

    ConsoleInputWorker worker;

    bool shouldCloseConsole() const;
    ImVec4 getLevelColor(LogLevel level) const;
    const char* getLevelName(LogLevel level) const;
    std::string getCurrentTimestamp() const;
};

template <typename Mutex>
class ConsoleSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit ConsoleSink(Console* console) : console(console) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!console) return;

        LogLevel level;
        switch (msg.level) {
            case spdlog::level::trace:
            case spdlog::level::debug:
                level = LogLevel::Debug;
                break;
            case spdlog::level::info:
                level = LogLevel::Info;
                break;
            case spdlog::level::warn:
                level = LogLevel::Warning;
                break;
            case spdlog::level::err:
            case spdlog::level::critical:
                level = LogLevel::Error;
                break;
            default:
                level = LogLevel::Info;
        }

        console->log(level, std::string(msg.payload.begin(), msg.payload.end()));
    }

    void flush_() override {}

private:
    Console* console;
};

using ConsoleSinkMt = ConsoleSink<std::mutex>;
using ConsoleSinkSt = ConsoleSink<spdlog::details::null_mutex>;

}  // namespace SpaceRogueLite
