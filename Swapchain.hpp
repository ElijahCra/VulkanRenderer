//
// Created by Elijah Crain on 11/30/24.
//

#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vulkan/vulkan.h>
#include <vector>

#include "RenderPass.hpp"

class Device;
class Window;

class SwapChain {
  public:
  SwapChain(Device* device, Window* window);
  ~SwapChain();

  VkExtent2D getExtent() const;
  VkFormat getImageFormat() const;
  VkSwapchainKHR getSwapChain() const;
  const std::vector<VkImageView>& getImageViews() const;

  private:
  Device* device;
  Window* window;
  VkSwapchainKHR swapChain;
  VkExtent2D extent;
  RenderPass* renderPass;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  void createSwapChain();
  void createImageViews();
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

  void createFramebuffers();
  void cleanupSwapChain();

  void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

};

#endif // SWAPCHAIN_HPP

