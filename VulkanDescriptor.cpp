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

    void updateUniformBuffer(size_t currentFrame) {
      UniformBufferObject ubo{};
      ubo.model = glm::mat4(1.0f); // No rotation

      float theta = windowPtr->cameraAngleX; // Azimuthal angle
      float phi = windowPtr->cameraAngleY; // Polar angle

      glm::vec3 cameraPos;
      cameraPos.x = windowPtr->radius * sin(phi) * sin(theta);
      cameraPos.y = windowPtr->radius * cos(phi);
      cameraPos.z = windowPtr->radius * sin(phi) * cos(theta);

      ubo.view = glm::lookAt(
        cameraPos,
        // viewpoint
        glm::vec3(0.0f, 0.0f, 0.0f),
        //origin
        glm::vec3(0.0f, 1.0f, 0.0f)); //Up vector

      ubo.proj = glm::perspective(glm::radians(windowPtr->fov),
                                  swapChainPtr->getExtent().width / (float) swapChainPtr->getExtent().height,
                                  0.1f,
                                  500.0f);
      //std::cout << "\n Camera Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")\n";
      //std::cout << "FOV: " << fov << "\n";

      void *data;
      vkMapMemory(devicePtr->getDevice(), uniformBuffersMemory[currentFrame], 0, sizeof(ubo), 0, &data);
      memcpy(data, &ubo, sizeof(ubo));
      vkUnmapMemory(devicePtr->getDevice(), uniformBuffersMemory[currentFrame]);
    }
  void updateTextureDescriptor(VkImageView imageView, VkSampler sampler) {
      for (size_t i = 0; i < descriptorSets.size(); i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
      }
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

  void createDescriptorSetLayout() {
      VkDescriptorSetLayoutBinding uboLayoutBinding{};
      uboLayoutBinding.binding = 0;
      uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uboLayoutBinding.descriptorCount = 1;
      uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      uboLayoutBinding.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutBinding samplerLayoutBinding{};
      samplerLayoutBinding.binding = 1;
      samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      samplerLayoutBinding.descriptorCount = 1;
      samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      samplerLayoutBinding.pImmutableSamplers = nullptr;

      std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

      VkDescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
      layoutInfo.pBindings = bindings.data();

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
      std::array<VkDescriptorPoolSize, 2> poolSizes{};
      poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      poolSizes[0].descriptorCount = maxFramesInFlight;
      poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[1].descriptorCount = maxFramesInFlight;

      VkDescriptorPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      poolInfo.pPoolSizes = poolSizes.data();
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
