#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Function to map a value from one range to another
float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

void main() {
    vec3 pos = inPos * 0.1; // Scale the model down

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);

    // Calculate color based on height (y-coordinate)
    // Assume terrain might range from -1.0 to 1.0 in height after scaling
    float height = pos.z*0.05f;

    // Clamp height to a reasonable range
    float heightNormalized = clamp(map(height, -0.1, 0.1, 0.0, 1.0), 0.0, 1.0);

    // Create a color gradient based on elevation:
    // Deep blue (water/low) -> green (medium) -> brown -> white (high peaks)
    vec3 color;
    if (heightNormalized < 0.3) {
        // Deep blue to light blue (water)
        color = mix(vec3(0.0, 0.0, 0.5), vec3(0.0, 0.5, 1.0), heightNormalized / 0.3);
    } else if (heightNormalized < 0.5) {
        // Light blue to green (shoreline to grass)
        color = mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 0.7, 0.0), (heightNormalized - 0.3) / 0.2);
    } else if (heightNormalized < 0.7) {
        // Green to brown (grass to mountain)
        color = mix(vec3(0.0, 0.7, 0.0), vec3(0.5, 0.3, 0.0), (heightNormalized - 0.5) / 0.2);
    } else if (heightNormalized < 0.9) {
        // Brown to gray (mountain to rock)
        color = mix(vec3(0.5, 0.3, 0.0), vec3(0.5, 0.5, 0.5), (heightNormalized - 0.7) / 0.2);
    } else {
        // Gray to white (rock to snow peaks)
        color = mix(vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 1.0), (heightNormalized - 0.9) / 0.1);
    }

    fragColor = color;
}