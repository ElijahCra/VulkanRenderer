//
// Created by Elijah Crain on 11/30/24.
//

#ifndef SYNCHRONIZATION_HPP
#define SYNCHRONIZATION_HPP

#include <vulkan/vulkan.h>
#include <vector>

class Device;

class Synchronization {
  public:
  Synchronization(Device* device);
  ~Synchronization();

  VkSemaphore getImageAvailableSemaphore(size_t index) const;
  VkSemaphore getRenderFinishedSemaphore(size_t index) const;
  VkFence getInFlightFence(size_t index) const;

  private:
  Device* device;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  const size_t MAX_FRAMES_IN_FLIGHT = 2;

  void createSyncObjects();
};

#endif // SYNCHRONIZATION_HPP
