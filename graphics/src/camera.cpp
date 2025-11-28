#include "camera.h"

using namespace SpaceRogueLite;

Camera::Camera(size_t screenWidth, size_t screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight) {}

void Camera::setPosition(const glm::vec2& pos) {
    if (position != pos) {
        position = pos;
        viewDirty = true;
        viewProjectionDirty = true;
    }
}

void Camera::setPosition(float x, float y) { setPosition(glm::vec2(x, y)); }

glm::vec2 Camera::getPosition() const { return position; }

void Camera::move(const glm::vec2& delta) { setPosition(position + delta); }

void Camera::move(float dx, float dy) { move(glm::vec2(dx, dy)); }

void Camera::setScreenSize(size_t width, size_t height) {
    if (screenWidth != width || screenHeight != height) {
        screenWidth = width;
        screenHeight = height;
        projectionDirty = true;
        viewProjectionDirty = true;
    }
}

size_t Camera::getScreenWidth() const { return screenWidth; }

size_t Camera::getScreenHeight() const { return screenHeight; }

const glm::mat4& Camera::getProjectionMatrix() const {
    if (projectionDirty) {
        recalculateProjectionMatrix();
    }
    return projectionMatrix;
}

const glm::mat4& Camera::getViewMatrix() const {
    if (viewDirty) {
        recalculateViewMatrix();
    }
    return viewMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix() const {
    if (viewProjectionDirty) {
        recalculateViewProjectionMatrix();
    }
    return viewProjectionMatrix;
}

ViewportData Camera::getViewport() const {
    return ViewportData{
        0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f, 1.0f};
}

ScissorData Camera::getScissor() const {
    return ScissorData{0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight)};
}

glm::vec2 Camera::screenToWorld(const glm::vec2& screenPos) const { return screenPos + position; }

glm::vec2 Camera::worldToScreen(const glm::vec2& worldPos) const { return worldPos - position; }

void Camera::recalculateProjectionMatrix() const {
    // Orthographic projection: screen space (pixels) to NDC
    // Origin at top-left, Y increases downward
    projectionMatrix = glm::ortho(0.0f, static_cast<float>(screenWidth),
                                  static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);
    projectionDirty = false;
}

void Camera::recalculateViewMatrix() const {
    // View matrix is the inverse of the camera's transform
    // For position-only: translate by negative position
    viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-position, 0.0f));
    viewDirty = false;
}

void Camera::recalculateViewProjectionMatrix() const {
    if (projectionDirty) {
        recalculateProjectionMatrix();
    }
    if (viewDirty) {
        recalculateViewMatrix();
    }
    viewProjectionMatrix = projectionMatrix * viewMatrix;
    viewProjectionDirty = false;
}
