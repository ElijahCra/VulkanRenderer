//
// Created by Elijah Crain on 11/30/24.
//

#include "Synchronization.hpp"
#include "Device.hpp"
#include <stdexcept>

Synchronization::Synchronization(Device* device) : device(device) {
  createSyncObjects();
}

Synchronization::~Synchronization() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device->getLogical(), imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device->getLogical(), renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(device->getLogical(), inFlightFences[i], nullptr);
  }
}

VkSemaphore Synchronization::getImageAvailableSemaphore(size_t index) const {
  return imageAvailableSemaphores[index];
}

VkSemaphore Synchronization::getRenderFinishedSemaphore(size_t index) const {
  return renderFinishedSemaphores[index];
}

VkFence Synchronization::getInFlightFence(size_t index) const {
  return inFlightFences[index];
}

void Synchronization::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
  }
}

