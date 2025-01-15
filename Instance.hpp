//
// Created by Elijah Crain on 11/30/24.
//

#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#define GLFW_INCLUDE_VULKAN
#include <optional>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class Instance {
  public:
  Instance();
  ~Instance();

  VkInstance getVkInstance() const;

  private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  void createInstance();
  void setupDebugMessenger();
  std::vector<const char*> getRequiredExtensions();
  bool checkValidationLayerSupport();
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
  void Instance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);



};

#endif // INSTANCE_HPP
