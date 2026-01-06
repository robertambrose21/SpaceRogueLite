#pragma once

#include "camera.h"
#include "inputhandler.h"

namespace SpaceRogueLite {

struct CameraInputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

struct CameraInputWorker : InputHandler::InputWorker {
    explicit CameraInputWorker(uint32_t workerId, CameraInputState& state) {
        id = workerId;
        name = "CameraInput";
        function = [&state](const SDL_Event& event) {
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.scancode) {
                    case SDL_SCANCODE_W:
                        state.up = pressed;
                        break;
                    case SDL_SCANCODE_S:
                        state.down = pressed;
                        break;
                    case SDL_SCANCODE_A:
                        state.left = pressed;
                        break;
                    case SDL_SCANCODE_D:
                        state.right = pressed;
                        break;
                    default:
                        break;
                }
            }
        };
    }
};

class CameraInput {
public:
    static constexpr float CAMERA_SPEED = 2000.0f;

    explicit CameraInput(Camera* camera, uint32_t workerId);
    ~CameraInput() = default;

    CameraInputWorker& getWorker();
    void update(int64_t timeSinceLastFrame, bool& quit);

private:
    Camera* camera;
    CameraInputWorker worker;
    CameraInputState state;
};

}  // namespace SpaceRogueLite