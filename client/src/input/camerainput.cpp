#include "camerainput.h"

using namespace SpaceRogueLite;

CameraInput::CameraInput(Camera* camera, uint32_t workerId)
    : camera(camera), worker(workerId, state) {}

CameraInputWorker& CameraInput::getWorker() { return worker; }

void CameraInput::update(int64_t timeSinceLastFrame, bool& quit) {
    float deltaSeconds = timeSinceLastFrame / 1000.0f;
    float moveAmount = CAMERA_SPEED * deltaSeconds;

    float dx = 0.0f, dy = 0.0f;

    if (state.up) dy -= moveAmount;
    if (state.down) dy += moveAmount;
    if (state.left) dx -= moveAmount;
    if (state.right) dx += moveAmount;

    if (dx != 0.0f || dy != 0.0f) {
        camera->move(dx, dy);
    }
}