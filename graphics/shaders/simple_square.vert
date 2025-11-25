#version 450

layout(location = 0) in vec2 inPosition;

layout(std140, set = 1, binding = 0) uniform TransformUBO {
    mat4 projection;
} ubo;

void main() {
    // Apply orthographic projection matrix
    gl_Position = ubo.projection * vec4(inPosition, 0.0, 1.0);
}
