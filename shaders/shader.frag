#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int fragHovered;
layout(location = 3) in float faceFlag;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform sampler2D atlasTexture;

void main() {
    // Determine default tile: top face uses grass (tile index 0), bottom uses dirt (tile index 1)
    int tileIndex = (faceFlag > 0.0) ? 0 : 1;
    // Override with star tile if hovered.
    if (fragHovered == 1) {
        tileIndex = 2;
    }
    // Our atlas is assumed to be arranged horizontally in 3 equal columns.
    float atlasTileWidth = 1.0 / 3.0;
    // Remap the fragment UV into the appropriate sub‑region.
    vec2 atlasUV;
    atlasUV.x = fragTexCoord.x * atlasTileWidth + tileIndex * atlasTileWidth;
    atlasUV.y = fragTexCoord.y;
    vec4 sampled = texture(atlasTexture, atlasUV);
    outColor = sampled * vec4(fragColor, 1.0);
}
