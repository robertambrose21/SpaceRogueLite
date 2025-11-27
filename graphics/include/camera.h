#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SpaceRogueLite {

struct ViewportData {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct ScissorData {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class Camera {
public:
    explicit Camera(size_t screenWidth, size_t screenHeight);

    // Position control
    void setPosition(const glm::vec2& position);
    void setPosition(float x, float y);
    glm::vec2 getPosition() const;
    void move(const glm::vec2& delta);
    void move(float dx, float dy);

    // Screen size (for resize handling)
    void setScreenSize(size_t width, size_t height);
    size_t getScreenWidth() const;
    size_t getScreenHeight() const;

    // Matrix accessors (cached with dirty flags)
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getViewProjectionMatrix() const;

    // Viewport/scissor data
    ViewportData getViewport() const;
    ScissorData getScissor() const;

    // Coordinate conversion
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;

private:
    void recalculateProjectionMatrix() const;
    void recalculateViewMatrix() const;
    void recalculateViewProjectionMatrix() const;

    size_t screenWidth;
    size_t screenHeight;
    glm::vec2 position{0.0f, 0.0f};

    // Cached matrices (mutable for lazy evaluation)
    mutable glm::mat4 projectionMatrix{1.0f};
    mutable glm::mat4 viewMatrix{1.0f};
    mutable glm::mat4 viewProjectionMatrix{1.0f};

    mutable bool projectionDirty{true};
    mutable bool viewDirty{true};
    mutable bool viewProjectionDirty{true};
};

}  // namespace SpaceRogueLite
