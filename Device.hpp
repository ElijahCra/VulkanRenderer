//
// Created by Elijah Crain on 11/30/24.
//

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Window.hpp"
#include "Utils.cpp"



class Instance;

class Device {
  public:
  Device(Instance* instance, Window* window);
  ~Device();

  VkDevice getLogical() const;
  VkPhysicalDevice getPhysical() const;
  VkQueue getGraphicsQueue() const;
  VkQueue getPresentQueue() const;
  const VkSurfaceKHR& getSurface() const;
  VkSampleCountFlagBits getSampleCount() const {return msaaSamples;}

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);




  private:
  VkDevice device;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  Instance* instance;
  VkSurfaceKHR surface;

  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

  void createSurface(GLFWwindow* window);
  void pickPhysicalDevice();
  void createLogicalDevice();
  VkSampleCountFlagBits getMaxUsableSampleCount();

  bool isDeviceSuitable(VkPhysicalDevice device);
  uint32_t findQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueFlags);


};

#endif // DEVICE_HPP

