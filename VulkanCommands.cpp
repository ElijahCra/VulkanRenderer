//
// Created by Elijah Crain on 2/10/25.
//
#pragma once

#include "VulkanDevice.cpp"
#include <vector>

class VulkanCommands {
  public:
    VulkanCommands(std::shared_ptr<VulkanDevice> device, uint32_t maxFramesInFlight)
      : devicePtr(device), maxFramesInFlight(maxFramesInFlight) {
      createCommandPool();
      allocateCommandBuffers();
    }

    ~VulkanCommands() {
      if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device(), commandPool, nullptr);
      }
    }

    VkCommandPool getCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer> &getCommandBuffers() const { return commandBuffers; }

  private:
    std::shared_ptr<VulkanDevice> devicePtr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    uint32_t maxFramesInFlight;

    VkDevice device() const { return devicePtr->getDevice(); }

    void createCommandPool() {
      QueueFamilyIndices queueFamilyIndices = devicePtr->getQueueFamilyIndices();

      VkCommandPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

      if (vkCreateCommandPool(device(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
      }
    }

    void allocateCommandBuffers() {
      commandBuffers.resize(maxFramesInFlight);

      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

      if (vkAllocateCommandBuffers(device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
      }
    }
};
