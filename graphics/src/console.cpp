#include "console.h"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace SpaceRogueLite;

Console::Console() : worker(CONSOLE_WORKER_ID, visible) {}

void Console::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(entriesMutex);

    LogEntry entry{level, message, getCurrentTimestamp()};
    entries.push_back(std::move(entry));

    while (entries.size() > MAX_LOG_ENTRIES) {
        entries.pop_front();
    }

    if (autoScroll) {
        scrollToBottom = true;
    }
}

void Console::logDebug(const std::string& message) { log(LogLevel::Debug, message); }

void Console::logInfo(const std::string& message) { log(LogLevel::Info, message); }

void Console::logWarning(const std::string& message) { log(LogLevel::Warning, message); }

void Console::logError(const std::string& message) { log(LogLevel::Error, message); }

void Console::render() {
    if (!visible) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    float consoleHeight = io.DisplaySize.y * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, consoleHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Console", &visible, flags)) {
        const char* levels[] = {"Debug", "Info", "Warning", "Error"};
        int currentLevel = static_cast<int>(filterLevel);

        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##LogLevel", &currentLevel, levels, IM_ARRAYSIZE(levels))) {
            filterLevel = static_cast<LogLevel>(currentLevel);
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll);

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            clear();
        }

        ImGui::Separator();

        float footerHeight =
            ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() * 2;
        ImGui::BeginChild("LogScrollRegion", ImVec2(0, -footerHeight), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        {
            std::lock_guard<std::mutex> lock(entriesMutex);
            for (const auto& entry : entries) {
                if (static_cast<int>(entry.level) < static_cast<int>(filterLevel)) continue;

                ImVec4 color = getLevelColor(entry.level);
                ImGui::PushStyleColor(ImGuiCol_Text, color);

                ImGui::TextUnformatted(("[" + entry.timestamp + "] [" + getLevelName(entry.level) +
                                        "] " + entry.message)
                                           .c_str());

                ImGui::PopStyleColor();
            }
        }

        if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
            ImGui::SetScrollHereY(1.0f);
        }
        scrollToBottom = false;

        ImGui::EndChild();

        {
            std::lock_guard<std::mutex> lock(entriesMutex);
            ImGui::Text("%zu entries", entries.size());
        }

        ImGui::SetNextItemWidth(-1);
        ImGuiInputTextFlags inputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
        if (ImGui::InputText("##ConsoleInput", inputBuffer, sizeof(inputBuffer), inputFlags)) {
            if (inputBuffer[0] != '\0') {
                std::string command(inputBuffer);
                logInfo("> " + command);
                if (onCommand) {
                    onCommand(command);
                }
                inputBuffer[0] = '\0';
                scrollToBottom = true;
            }
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();
}

void Console::toggle() { visible = !visible; }

bool Console::isVisible() const { return visible; }

ConsoleInputWorker& Console::getWorker() { return worker; }

void Console::setLevelFilter(LogLevel minLevel) { filterLevel = minLevel; }

LogLevel Console::getLevelFilter() const { return filterLevel; }

void Console::clear() {
    std::lock_guard<std::mutex> lock(entriesMutex);
    entries.clear();
}

void Console::setCommandCallback(std::function<void(const std::string&)> callback) {
    onCommand = std::move(callback);
}

ImVec4 Console::getLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:
            return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        case LogLevel::Info:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        case LogLevel::Warning:
            return ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
        case LogLevel::Error:
            return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        default:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

const char* Console::getLevelName(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string Console::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S") << '.' << std::setfill('0')
        << std::setw(3) << ms.count();
    return oss.str();
}
