//
// Created by Elijah Crain on 2/9/25.
//
#pragma once

#include "VulkanDevice.cpp"
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanSwapChain.cpp"
#include "VulkanDevice.cpp"
#include "VulkanWindow.cpp"

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class VulkanDescriptors {
  public:
    VulkanDescriptors(std::shared_ptr<VulkanDevice> device,
                      std::shared_ptr<VulkanSwapChain> swapchain,
                      std::shared_ptr<VulkanWindow> window,
                      uint32_t maxFramesInFlight)
      : devicePtr(std::move(device)), swapChainPtr(std::move(swapchain)), windowPtr(std::move(window)),
        maxFramesInFlight(maxFramesInFlight) {
      createDescriptorSetLayout();
      createUniformBuffers();
      createDescriptorPool();
      createDescriptorSets();
    }

    ~VulkanDescriptors() {
      vkDestroyDescriptorPool(device(), descriptorPool, nullptr);
      for (size_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroyBuffer(device(), uniformBuffers[i], nullptr);
        vkFreeMemory(device(), uniformBuffersMemory[i], nullptr);
      }

      vkDestroyDescriptorSetLayout(device(), descriptorSetLayout, nullptr);
    }

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    const std::vector<VkDescriptorSet> &getDescriptorSets() const { return descriptorSets; }

    // Original updateUniformBuffer method - retained for backward compatibility
    void updateUniformBuffer(size_t currentFrame) {
      // Default implementation uses spherical coordinates
      glm::vec3 cameraPos;
      cameraPos.x = windowPtr->radius * sin(windowPtr->cameraAngleY) * sin(windowPtr->cameraAngleX);
      cameraPos.y = windowPtr->radius * cos(windowPtr->cameraAngleY);
      cameraPos.z = windowPtr->radius * sin(windowPtr->cameraAngleY) * cos(windowPtr->cameraAngleX);

      updateUniformBufferWithPosition(currentFrame, cameraPos);
    }

    // New method that takes an explicit camera position
    void updateUniformBuffer(size_t currentFrame, const glm::vec3& cameraPos) {
      updateUniformBufferWithPosition(currentFrame, cameraPos);
    }

  private:
    std::shared_ptr<VulkanDevice> devicePtr;
    std::shared_ptr<VulkanSwapChain> swapChainPtr;
    std::shared_ptr<VulkanWindow> windowPtr;
    uint32_t maxFramesInFlight;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkDevice device() const { return devicePtr->getDevice(); }

void updateUniformBufferWithPosition(size_t currentFrame, const glm::vec3& cameraPos) {
  UniformBufferObject ubo{};

  // No model rotation needed when using Z as up vector consistently
  ubo.model = glm::mat4(1.0f);

  // Use the camera angles from the window, which include rotation offsets
  float theta = windowPtr->cameraAngleX; // Azimuthal angle
  float phi = windowPtr->cameraAngleY;   // Polar angle

  // Clamp phi to prevent the flip at poles
  // Allow looking almost straight up/down but not completely
  const float epsilon = 0.001f;
  phi = glm::clamp(phi, epsilon, glm::pi<float>() - epsilon);

  // Calculate look direction for Z-up coordinate system
  glm::vec3 lookDirection;
  lookDirection.x = sin(phi) * sin(theta);
  lookDirection.y = sin(phi) * cos(theta);
  lookDirection.z = cos(phi);  // Z is up, so cosine of polar angle gives Z component

  // Calculate look-at point by adding the direction vector to camera position
  glm::vec3 lookAtPoint = cameraPos + lookDirection;

  // For up vector, create a stable up vector based on the camera's local coordinate system
  // This prevents flipping when crossing poles
  glm::vec3 worldUp(0.0f, 0.0f, 1.0f);  // Z is world up

  // Using a stable reference frame for lookAt
  glm::vec3 forward = glm::normalize(lookAtPoint - cameraPos);
  glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

  // If we're looking almost straight up or down, right might be near-zero
  // In that case, use a fixed reference direction
  if (glm::length(right) < 0.01f) {
    right = glm::vec3(sin(theta + glm::half_pi<float>()), cos(theta + glm::half_pi<float>()), 0.0f);
  }

  // Recalculate up based on right and forward to ensure orthogonality
  glm::vec3 up = glm::normalize(glm::cross(right, forward));

  // Build the view matrix using our computed vectors
  ubo.view = glm::lookAt(cameraPos, lookAtPoint, up);

  // Perspective projection matrix
  ubo.proj = glm::perspective(
    glm::radians(windowPtr->fov),
    swapChainPtr->getExtent().width / (float)swapChainPtr->getExtent().height,
    0.1f,
    500.0f
  );

  // Flip Y coordinate for Vulkan coordinate system
  ubo.proj[1][1] *= -1;

  // Update the uniform buffer
  void *data;
  vkMapMemory(devicePtr->getDevice(), uniformBuffersMemory[currentFrame], 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(devicePtr->getDevice(), uniformBuffersMemory[currentFrame]);
}

    void createDescriptorSetLayout() {
      VkDescriptorSetLayoutBinding uboLayoutBinding{};
      uboLayoutBinding.binding = 0;
      uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uboLayoutBinding.descriptorCount = 1;
      uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      uboLayoutBinding.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = 1;
      layoutInfo.pBindings = &uboLayoutBinding;

      if (vkCreateDescriptorSetLayout(device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
      }
    }

    void createUniformBuffers() {
      VkDeviceSize bufferSize = sizeof(UniformBufferObject);

      uniformBuffers.resize(maxFramesInFlight);
      uniformBuffersMemory.resize(maxFramesInFlight);

      for (size_t i = 0; i < maxFramesInFlight; i++) {
        devicePtr->createBuffer(
          bufferSize,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          uniformBuffers[i],
          uniformBuffersMemory[i]
        );
      }
    }

    void createDescriptorPool() {
      VkDescriptorPoolSize poolSize{};
      poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      poolSize.descriptorCount = maxFramesInFlight;

      VkDescriptorPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = 1;
      poolInfo.pPoolSizes = &poolSize;
      poolInfo.maxSets = maxFramesInFlight;

      if (vkCreateDescriptorPool(device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
      }
    }

    void createDescriptorSets() {
      std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);

      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = maxFramesInFlight;
      allocInfo.pSetLayouts = layouts.data();

      descriptorSets.resize(maxFramesInFlight);
      if (vkAllocateDescriptorSets(device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
      }

      for (size_t i = 0; i < maxFramesInFlight; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
      }
    }
};