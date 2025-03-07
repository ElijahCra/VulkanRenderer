//
// Created by Elijah Crain on 2/9/25.
//
#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// We'll keep this struct but won't use it
struct InstanceData {
  glm::vec2 offset;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;

  bool operator==(const Vertex& other) const {
    return pos.x == other.pos.x &&
           pos.y == other.pos.y &&
           pos.z == other.pos.z &&
           color.x == other.color.x &&
           color.y == other.color.y &&
           color.z == other.color.z;
  }

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  // Modified to only include vertex attributes, no instance attributes
  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    // Position attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // Color attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};

namespace std {
template<>
struct hash<Vertex> {
  size_t operator()(const Vertex& vertex) const noexcept {
    // Combine the hash of position and color components
    // This is a simple hash combination function
    auto hashX = hash<float>()(vertex.pos.x);
    auto hashY = hash<float>()(vertex.pos.y);
    auto hashZ = hash<float>()(vertex.pos.z);

    auto hashR = hash<float>()(vertex.color.x);
    auto hashG = hash<float>()(vertex.color.y);
    auto hashB = hash<float>()(vertex.color.z);

    // Combine hashes - a simple but effective approach
    size_t result = hashX;
    result = (result << 5) + result + hashY;
    result = (result << 5) + result + hashZ;
    result = (result << 5) + result + hashR;
    result = (result << 5) + result + hashG;
    result = (result << 5) + result + hashB;

    return result;
  }
};
}