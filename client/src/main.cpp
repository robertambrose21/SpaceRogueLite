#include <spdlog/spdlog.h>
#include <yojimbo.h>

#include "net/client.h"

int main() {
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_DEBUG);

    if (!InitializeYojimbo()) {
        spdlog::error("Failed to initialize Yojimbo!");
        return 1;
    }

    spdlog::info("Yojimbo initialized successfully.");

    SpaceRogueLite::Client client(1, yojimbo::Address("127.0.0.1", 8081));
    client.connect();
    client.disconnect();

    ShutdownYojimbo();

    return 0;
}
