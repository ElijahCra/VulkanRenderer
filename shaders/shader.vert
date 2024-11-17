#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 10;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    int gridX = gl_InstanceIndex % GRID_WIDTH;
    int gridY = gl_InstanceIndex / GRID_WIDTH;

    float xOffset = (float(gridX) - float(GRID_WIDTH - 1) / 2.0) * 1.5;
    float yOffset = (float(gridY) - float(GRID_HEIGHT - 1) / 2.0) * sqrt(3.0);

    if (gridY % 2 == 1) {
        xOffset += 0.75;
    }

    vec3 pos = inPos * 0.1;  // Scale the position
    pos.x += xOffset * 0.108; // Adjust x position
    pos.y += yOffset * 0.084; // Adjust z position

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    fragColor = inColor;
}
