//
// Created by Elijah Crain on 2/10/25.
//
#pragma once

#include "VulkanDevice.cpp"
#include <vector>

class VulkanSync {
  public:
    VulkanSync(std::shared_ptr<VulkanDevice> device, uint32_t maxFramesInFlight)
      : devicePtr(device), maxFramesInFlight(maxFramesInFlight) {
      createSyncObjects();
    }

    ~VulkanSync() {
      for (size_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(device(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device(), inFlightFences[i], nullptr);
      }
    }

    VkSemaphore getImageAvailableSemaphore(size_t frameIndex) const {
      return imageAvailableSemaphores[frameIndex];
    }

    VkSemaphore getRenderFinishedSemaphore(size_t frameIndex) const {
      return renderFinishedSemaphores[frameIndex];
    }

    VkFence *getInFlightFence(size_t frameIndex) {
      return &inFlightFences[frameIndex];
    }

    uint32_t getMaxFramesInFlight() const { return maxFramesInFlight; }

  private:
    std::shared_ptr<VulkanDevice> devicePtr;
    uint32_t maxFramesInFlight;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkDevice device() const { return devicePtr->getDevice(); }

    void createSyncObjects() {
      imageAvailableSemaphores.resize(maxFramesInFlight);
      renderFinishedSemaphores.resize(maxFramesInFlight);
      inFlightFences.resize(maxFramesInFlight);

      VkSemaphoreCreateInfo semaphoreInfo{};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceInfo{};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create sync objects for a frame!");
        }
      }
    }
};
