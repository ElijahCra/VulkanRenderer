#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 instanceOffset;
layout(location = 4) in int instanceHovered;  // per‑instance “hover” flag

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int fragHovered;
layout(location = 3) out float faceFlag; // we'll use the original z value

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Transform the vertex position and add instance offset.
    vec3 pos = inPos * 0.1;
    pos.xy += instanceOffset;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragHovered = instanceHovered;
    faceFlag = inPos.z;  // positive z = top face, negative z = bottom face
}
