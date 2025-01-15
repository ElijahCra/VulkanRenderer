//
// Created by Elijah Crain on 11/30/24.
//

#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include <vulkan/vulkan.h>

#include "SwapChain.hpp"

class Device;

class CommandPool {
  public:
  CommandPool(Device* device, SwapChain* swapChain);
  ~CommandPool();

  VkCommandPool getCommandPool() const;

  private:
  Device* device;
  VkCommandPool commandPool;
  SwapChain* swapChain;

  void createCommandPool();
};

#endif // COMMANDPOOL_HPP
