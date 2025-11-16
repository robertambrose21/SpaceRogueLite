#pragma once

#include <spdlog/spdlog.h>
#include <yojimbo.h>
#include <map>
#include <string>

#include "connectionconfig.h"

namespace SpaceRogueLite {

class Message : public yojimbo::Message {
public:
    Message(MessageChannel channel) : channel(channel) {}
    virtual ~Message() = default;

    virtual constexpr const char* getName() const = 0;
    virtual std::string toString(void) const = 0;

    /**
     * @brief Parse arguments to populate this message's data fields.
     *
     * Message implementations should override this method to extract and validate
     * arguments from the provided vector.
     *
     * Each message class is responsible for:
     *   - Validating the correct number of arguments
     *   - Converting string arguments to appropriate types
     *   - Performing data validation (null checks, size limits, etc.)
     *   - Logging appropriate warnings/errors for invalid input
     *
     * Examples:
     *   - No-argument messages (like PingMessage): Check args.empty()
     *   - Single argument: Check args.size() == 1, then use args[0]
     *   - Multiple arguments: Check args.size(), then use args[0], args[1], etc.
     *
     * @param args Vector of string arguments parsed from the command line
     */
    virtual void parse(const std::vector<std::string>& args) = 0;

    virtual std::string getCommandHelpText(void) const = 0;

    MessageChannel getMessageChannel() const noexcept { return channel; }

private:
    MessageChannel channel;
};

}  // namespace SpaceRogueLite