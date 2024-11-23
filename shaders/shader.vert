#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 instanceOffset;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec3 pos = inPos * 0.1;
    pos.x += instanceOffset.x;
    pos.y += instanceOffset.y;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    fragColor = inColor;
}
